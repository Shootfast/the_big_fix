#include "frame_buffer_operators.h"
#include <libdragon.h>
#include "main.h"

typedef struct fbop_blend_t_ {
	rspq_block_t* blend_blocks[FBOP_BLEND_SLICES];
	surface_t shade_placeholder;
	surface_t color_placeholder;
} fbop_blend_t_;

typedef struct fbop_uvgen_t_ {
	rspq_block_t* generator;
	surface_t tex_u;
	surface_t tex_v;
} fbop_uvgen_t_;

static rspq_block_t* create_blend_block(fbop_blend_t* op, size_t idx){
	rspq_block_begin();

	if (idx == 0){
		rdpq_mode_begin();
			rdpq_mode_blender(0);
			rdpq_mode_combiner(RDPQ_COMBINER2(
				(1, TEX0, TEX1, 0),     (0, 0, 0, TEX0),
				(0, 0, 0, COMBINED),    (0, 0, 0, COMBINED)
			));
			rdpq_mode_alphacompare(100);
			rdpq_mode_filter(FILTER_POINT);
		rdpq_mode_end();
	}
	int block_height = SCREEN_HEIGHT / FBOP_BLEND_SLICES; 
	int y = idx * (FBOP_BLEND_SLICES - 1);
	int y_end = y + block_height;

	for(; y < y_end; y+= 3){
		int next_y = y+3;
		rdpq_tex_multi_begin();
			rdpq_tex_upload_sub(TILE0, &op->shade_placeholder, NULL, 0, y, SCREEN_WIDTH, next_y);
			rdpq_tex_upload_sub(TILE1, &op->color_placeholder, NULL, 0, y, SCREEN_WIDTH, next_y);
		rdpq_tex_multi_end();
		rdpq_texture_rectangle(TILE0, 0, y, SCREEN_WIDTH, next_y, 0, y);
	}
	return rspq_block_end();
}

fbop_blend_t* fbop_blend_alloc(){
	fbop_blend_t* op = malloc(sizeof(fbop_blend_t_));
	if (!op){
		return NULL;
	}

	op->shade_placeholder = surface_make_placeholder(
			RDPQ_SHADE_BUFFER_IDX,
			FMT_RGBA16,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			SCREEN_WIDTH * 2
	);
	op->color_placeholder = surface_make_placeholder(
			RDPQ_COLOR_BUFFER_IDX,
			FMT_RGBA16,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			SCREEN_WIDTH * 2
	);

	for(size_t idx=0; idx < FBOP_BLEND_SLICES; ++idx){
		op->blend_blocks[idx] = create_blend_block(op, idx);
	}

	return op;
}

void fbop_blend_free(fbop_blend_t* op){
	for (size_t idx=0; idx < FBOP_BLEND_SLICES; ++idx){
		rspq_block_free(op->blend_blocks[idx]);
	}
	free(op);
}

void fbop_blend(fbop_blend_t* op, size_t idx){
	rspq_block_run(op->blend_blocks[idx]);
}



static rspq_block_t* create_uvgen_block(fbop_uvgen_t* op){
	rspq_block_begin();
	rdpq_texparms_t tex_params_u = {0};
	tex_params_u.s.repeats = REPEAT_INFINITE;
	tex_params_u.t.repeats = REPEAT_INFINITE;
	rdpq_texparms_t tex_params_v = tex_params_u;
	tex_params_v.s.scale_log = 6;
	tex_params_v.t.scale_log = 2;
	rdpq_tex_multi_begin();
		rdpq_tex_upload(TILE0, &op->tex_u, &tex_params_u);
		rdpq_tex_upload(TILE1, &op->tex_v, &tex_params_v);
	rdpq_tex_multi_end();
	return rspq_block_end();
}

static void generate_uv_texture(surface_t* tex_u, surface_t* tex_v){
	uint32_t* data = (uint32_t*)tex_u->buffer;
	for (uint32_t y=0; y < FBOP_SWIZZLE_SIZE; ++y){
		uint32_t val = (y % FBOP_SWIZZLE_SIZE) * FBOP_SWIZZLE_SIZE;
		for (uint32_t i=0; i < (256/FBOP_SWIZZLE_SIZE); i+=FBOP_SWIZZLE_SIZE){
			for(uint32_t sub=0; sub < FBOP_SWIZZLE_SIZE; ++sub){
				*(data++) = ((val+sub) << 8) & 0xFF00;
			}
			val += FBOP_SWIZZLE_SIZE * FBOP_SWIZZLE_SIZE;
		}
	}

	data = (uint32_t*)tex_v->buffer;
	uint32_t val=0;
	for (uint32_t i=0; i < (256/FBOP_SWIZZLE_SIZE); ++i){
		for (uint32_t sub=0; sub < FBOP_SWIZZLE_SIZE; ++sub){
			*(data++) = ((val+sub) << 16) & 0xFF0000;
		}
		val += FBOP_SWIZZLE_SIZE;
	}
}


fbop_uvgen_t* fbop_uvgen_alloc(){
	fbop_uvgen_t* op = malloc(sizeof(fbop_uvgen_t_));
	if (!op){
		return NULL;
	}
	op->tex_u = surface_alloc(
			FMT_RGBA32,
			256 / FBOP_SWIZZLE_SIZE,
			FBOP_SWIZZLE_SIZE
	);
	op->tex_v = surface_alloc(
			FMT_RGBA32,
			FBOP_SWIZZLE_SIZE,
			256 / FBOP_SWIZZLE_SIZE
	);
	generate_uv_texture(&op->tex_u, &op->tex_v);
	
	op->generator = create_uvgen_block(op);
	return op;
}

void fbop_uvgen_free(fbop_uvgen_t* op){
	surface_free(&op->tex_u);
	surface_free(&op->tex_v);
	rspq_block_free(op->generator);
	free(op);
}
void fbop_uvgen(fbop_uvgen_t* op){
	rspq_block_run(op->generator);
}
