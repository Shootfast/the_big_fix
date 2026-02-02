#include "scene.h"
#include <stdlib.h>
#include <t3d/t3d.h>
#include <inttypes.h>

#include "main.h"
#include "tex.h"
#include "rsp/fx.h"
#include "texture_manager.h"
#include "frame_buffer_operators.h"
#include "camera.h"
#include "state.h"
#include "level.h"
#include "player.h"
#include "action.h"

typedef struct scene_t_ {
	state_t state;
	fbop_blend_t* blend_op;
	fbop_uvgen_t* uvgen_op;
	uint32_t frame_idx;
} scene_t_;


static void clear_screen(uint16_t color){
	rdpq_mode_push();
	{
		rdpq_set_mode_fill(color_from_packed16(color));
		rdpq_fill_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
	rdpq_mode_pop();
}


static void draw_scene(surface_t* color, frame_buffers_t* fb, void* user){
	scene_t* scene = (scene_t*)user;
	state_t* state = &scene->state;

	uint32_t prev_frame_idx = (scene->frame_idx + 2 ) % 3;

	t3d_frame_start();
	rdpq_mode_antialias(AA_NONE);
	rdpq_mode_dithering(DITHER_NONE_NONE);

	fbop_uvgen(scene->uvgen_op);
	if (state->first_person){
		camera_attach(&state->player->first_person_camera);
	} else {
		camera_attach(level_get_active_camera(state->level));
	}

	rdpq_set_color_image(&fb->depth);
	clear_screen(ZBUF_MAX);

	uint8_t ambient_light_color[4]={0x30, 0x30, 0x30, 0xFF};
	//uint8_t ambient_light_color[4]={0x90, 0x90, 0x90, 0xFF};
	uint8_t selected_light_color[4]={0xFF, 0xFF, 0xFF, 0xFF};
	t3d_light_set_ambient(ambient_light_color);
	t3d_light_set_count(0);
	level_set_lights(state->level);

	rdpq_set_color_image(&fb->uv[scene->frame_idx]);
	rdpq_set_z_image(&fb->depth);



	/* 1st draw pass - Write UVs to offscreen-buffer */
	level_draw_uvs(state->level);
	if (!state->first_person){
		player_draw_uvs(state->player);
	}
	
	rdpq_sync_pipe();
	rdpq_mode_zbuf(false, false);

	/* 2nd draw pass - Shading into different buffer */
	rdpq_set_color_image(&fb->shade[scene->frame_idx]);
	clear_screen(0);
	level_draw_shading(state->level, &ambient_light_color, &selected_light_color);
	if (!state->first_person){
		player_draw_shading(state->player);
	}
	
	rdpq_sync_pipe();


	rdpq_sync_tile();
	rdpq_sync_load();
	rdpq_set_color_image(color);
	rdpq_set_mode_standard();

	rdpq_set_lookup_address(RDPQ_SHADE_BUFFER_IDX, fb->shade[prev_frame_idx].buffer);
	rdpq_set_lookup_address(RDPQ_COLOR_BUFFER_IDX, color->buffer);
	rspq_flush();


	uint64_t* tex_in = (uint64_t*)CachedAddr(fb->uv[prev_frame_idx].buffer);
	uint32_t fb_size_in = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint8_t) * 4;

	uint32_t quarter_slice = fb_size_in / FBOP_BLEND_SLICES / 3;
	uint32_t step_size_tex_in = quarter_slice * 2;
	uint32_t step_size_tex_in_rsp = quarter_slice * 1;

	uint32_t ptr_in_pos = (uint32_t)tex_in;
	uint32_t ptr_out_pos = (uint32_t)CachedAddr(color->buffer);

	for(size_t slice=0; slice<FBOP_BLEND_SLICES; ++slice){
		if (slice % 4 == 0){
			rsp_fx_fill_textures(
				ptr_in_pos, 
				ptr_in_pos + step_size_tex_in_rsp,
				ptr_out_pos
			);
			
			rspq_flush(); /* <- ensure RSP is busy */
			ptr_in_pos += step_size_tex_in_rsp;
			ptr_out_pos += step_size_tex_in_rsp / 2;
			apply_texture(
				ptr_in_pos,
				ptr_in_pos + step_size_tex_in,
				ptr_out_pos
			);
			ptr_in_pos += step_size_tex_in;
			ptr_out_pos += step_size_tex_in / 2;
		} else {
			apply_texture(
				ptr_in_pos,
				ptr_in_pos + quarter_slice * 3,
				ptr_out_pos
			);
			ptr_in_pos += quarter_slice * 3;
			ptr_out_pos += quarter_slice * 3 / 2;
		}
		data_cache_hit_writeback_invalidate(
			(char*)CachedAddr(ptr_out_pos) - 0x1000,
			0x1000
		);
		fbop_blend(scene->blend_op, slice);

		rspq_flush();
	}

	ui_draw(state->ui);
	scene->frame_idx = (scene->frame_idx + 1) % 3;
}

scene_t* scene_alloc(scene_manager_t* sm, const char* level_name){
	scene_t* scene = malloc(sizeof(scene_t));
	if (!scene){
		return NULL;
	}

	state_t* state = &scene->state;
	state->sm = sm;

	state->ui = ui_alloc();
	if (!state->ui){
		goto err;
	}
	state->tm = texture_manager_alloc(MAX_TEXTURES);
	if (!state->tm){
		goto err_ui;
	}
	state->am = animation_manager_alloc();
	if (!state->am){
		goto err_tm;
	}
	state->aum = audio_manager_alloc();
	if (!state->aum){
		goto err_am;
	}
	state->level = level_alloc(level_name, state->tm);
	if (!state->level){
		goto err_aum;
	}
	state->player = player_alloc(state->tm);
	if (!state->player){
		goto err_level;
	}
	state->controls_enabled = true;
	state->zone_triggers_active = true;
	state->camera_follows_player = true;
	state->interact_cooldown = 0;
	state->first_person = false;
	state->has_coat = false;

	scene->blend_op = fbop_blend_alloc();
	if (!scene->blend_op){
		goto err_player;
	}
	scene->uvgen_op = fbop_uvgen_alloc();
	if (!scene->uvgen_op){
		goto err_blendop;
	}
	
	scene->frame_idx = 0;

	/* run any post load triggers */
	state_fire_named_trigger(state, "on_load");

	return scene;

	fbop_uvgen_free(scene->uvgen_op);
err_blendop:
	fbop_blend_free(scene->blend_op);
err_player:
	player_free(state->player);
err_level:
	level_free(state->level);
err_aum:
	audio_manager_free(state->aum);
err_am:
	animation_manager_free(state->am);
err_tm:
	texture_manager_free(state->tm);
err_ui:
	ui_free(state->ui);
err:
	free(scene);
	return NULL;
}

void scene_free(scene_t* scene){
	fbop_uvgen_free(scene->uvgen_op);
	fbop_blend_free(scene->blend_op);
	state_t* state = &scene->state;
	player_free(state->player);
	level_free(state->level);
	audio_manager_free(state->aum);
	animation_manager_free(state->am);
	texture_manager_free(state->tm);
	ui_free(state->ui);
	free(scene);
}

void scene_update(scene_t* scene, float delta_time){
	state_update(&scene->state, delta_time);
}

void scene_use_swapchain(scene_t* scene, swap_chain_t* sc){
	swapchain_set_user_data(sc, (void*)scene);
	swapchain_set_draw_pass(sc, &draw_scene);
}
