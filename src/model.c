#include <stdlib.h>
#include <float.h>
#include <t3d/t3dmodel.h>
#include <t3d/t3d.h>

#include "model.h"

// MODEL_SCALE = 1 / base-scale given to gltf_to_t3d conversion tool (default value is 64)
#define MODEL_SCALE (1.0f / 64) 

typedef struct model_t_ {
	T3DModel* model;
	size_t objects_len;
	T3DObject** objects;
	T3DMat4FP matrix;
	rspq_block_t* draw_uvs;
	rspq_block_t* draw_shading;
	aabb_t aabb;
} model_t_;

static size_t objects_count(T3DModel* model){
	T3DModelIter it = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
	size_t n_objects = 0;
	while(t3d_model_iter_next(&it)){
		++n_objects;
	}
	return n_objects;
}

static int compare_material_name(const void* lhs, const void* rhs){
	const T3DObject* a = *(const T3DObject**) lhs;
	const T3DObject* b = *(const T3DObject**) rhs;
	if (a->material->name && b->material->name){
		return strcmp(a->material->name, b->material->name);
	}
	return 1;
}

static T3DObject** extract_objects(T3DModel* model, size_t* objects_len){
	*objects_len = objects_count(model);
	T3DObject** objects = malloc(sizeof(T3DObject*) * *objects_len);
	if (!objects){
		return NULL;
	}
	T3DModelIter it = t3d_model_iter_create(model, T3D_CHUNK_TYPE_OBJECT);
	size_t i=0;
	while(t3d_model_iter_next(&it)){
		objects[i] = it.object;
		++i;
	}
	/* Sort objects by material name */
	qsort(&objects[0], *objects_len, sizeof(T3DObject*), compare_material_name);
	return objects;
}

static aabb_t extract_aabb(T3DObject** objects, size_t objects_len){
	aabb_t aabb = {
		.min={{FLT_MAX, FLT_MAX, FLT_MAX}},
		.max={{-FLT_MAX, -FLT_MAX, -FLT_MAX}}
	};
	for (size_t i=0; i<objects_len; ++i){
		T3DObject* object = objects[i];
		for (size_t v=0; v<3; ++v){
			aabb.min.v[v] = fminf(aabb.min.v[v], object->aabbMin[v]);
			aabb.max.v[v] = fmaxf(aabb.max.v[v], object->aabbMax[v]);
		}
	}
	return aabb;
}


static void load_material(T3DObject* obj, texture_manager_t* tm){
	T3DMaterial* mat = obj->material;
	if (mat->textureA.texReference == 0xFF){
		return;
	}
	uint8_t mat_idx = 0;
	if (mat->textureA.texReference){
		mat_idx = texture_manager_reserve_texture(tm);
	} else if (mat->textureA.texPath){
		char* path = strdup(mat->textureA.texPath);
		/* ignore the .sprite suffix and just look for the .bci */
		char* last_dot = strrchr(path, '.');
		if (last_dot){
			*last_dot = '\0';
		}
		mat_idx = texture_manager_add_texture(tm, path); 
		free(path);
	} else {
	}

	mat->otherModeMask |= SOM_Z_COMPARE | SOM_Z_WRITE;
	if (mat->name[0] == '_'){
		mat->otherModeValue &= ~(SOM_Z_COMPARE | SOM_Z_WRITE);
	} else if (mat->name[0] == '#'){
		mat->otherModeValue &= ~(SOM_Z_COMPARE);
		mat->otherModeValue |= SOM_Z_WRITE;
	} else {
		mat->otherModeValue |= SOM_Z_COMPARE | SOM_Z_WRITE;
	}

	mat->otherModeMask |= SOM_SAMPLE_MASK;
	mat->otherModeValue |= SOM_SAMPLE_POINT;

	mat->renderFlags &= ~T3D_FLAG_SHADED;
	mat->textureA.texPath = NULL;
	mat->textureB.texPath = NULL;
	mat->textureA.texReference = 0xFF;
	mat->textureB.texReference = 0xFF;

	mat->colorCombiner = RDPQ_COMBINER2(
		(1, 0, TEX0, TEX1),     (0, 0, 0, 1), /* aka (1-0) * TEX0 + TEX1 */
		(1, 0, PRIM, COMBINED), (0, 0, 0, 1)  /* aka (1-0) * PRIM + COMBINED */
	);
	uint8_t base_addr_mat = (TEX_BASE_ADDR >> 16) & 0xFF;
	mat->primColor = (color_t){base_addr_mat + mat_idx, 0, 0, 0xFF};
}


static rspq_block_t* uv_calls(T3DObject** objects, size_t objects_len){
	rspq_block_begin();
		T3DModelState model_state = t3d_model_state_create();
		for(size_t i=0; i<objects_len; ++i){
			T3DObject* obj = objects[i];
			t3d_model_draw_material(obj->material, &model_state);
			t3d_model_draw_object(obj, NULL);
		}
		t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
	return rspq_block_end();
}

static rspq_block_t* shading_calls(T3DObject** objects, size_t objects_len){
	rspq_block_begin();
		rdpq_sync_pipe();
		rdpq_mode_combiner(RDPQ_COMBINER1((1, SHADE, PRIM, 0), (0, 0, 0, 1)));
		rdpq_mode_blender(0);
		rdpq_mode_alphacompare(0);
		rdpq_set_prim_color((color_t){0xFF, 0xFF, 0xFF, 0xFF});
		t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_CULL_BACK);

		int last_no_depth = -1;
		for(size_t i=0; i<objects_len; ++i){
			T3DObject* obj = objects[i];
			int no_depth = obj->material->name[0] == '_' ? 1 : 0;
			if (no_depth != last_no_depth){
				rdpq_sync_pipe();
				uint64_t mask = SOM_ZMODE_MASK | SOM_Z_COMPARE | SOM_Z_WRITE;
				uint64_t val = 0;
				if (!no_depth){
					val = SOM_ZMODE_DECAL | SOM_Z_COMPARE;
				} 
				rdpq_change_other_modes_raw(mask, val);
				last_no_depth = no_depth;
			}
			t3d_model_draw_object(obj, NULL);
		}
	return rspq_block_end();
}

model_t* model_alloc(const char* path, texture_manager_t* tm){
	model_t* model = malloc(sizeof(model_t));
	if (!model){
		return NULL;
	}
	model->model = t3d_model_load(path);
	if (!model->model){
		free(model);
		return NULL;
	}
	model->objects = extract_objects(model->model, &model->objects_len);

	model->aabb = extract_aabb(model->objects, model->objects_len);

	for (size_t i=0; i<model->objects_len; ++i){
		T3DObject* obj = model->objects[i];
		load_material(obj, tm);
	}
	
	t3d_mat4fp_from_srt_euler(&model->matrix, 
		(float[3]){MODEL_SCALE, MODEL_SCALE, MODEL_SCALE},
		(float[3]){0, 0, 0},
		(float[3]){0, 0, 0}
	);

	model->draw_uvs = uv_calls(model->objects, model->objects_len);
	model->draw_shading = shading_calls(model->objects, model->objects_len);

	return model;
}

void model_free(model_t* model){
	rspq_block_free(model->draw_uvs);
	rspq_block_free(model->draw_shading);
	free(model->objects);
	t3d_model_free(model->model);
	free(model);
}

T3DVec3 model_get_position(model_t* model){
	T3DVec3 pos;
	pos.v[0] = t3d_mat4fp_get_float(&model->matrix, 3, 0);
	pos.v[1] = t3d_mat4fp_get_float(&model->matrix, 3, 1);
	pos.v[2] = t3d_mat4fp_get_float(&model->matrix, 3, 2);
	return pos;
}
void model_set_position(model_t* model, const T3DVec3* pos){
	t3d_mat4fp_set_pos(&model->matrix, pos->v);
}
void model_set_rotation(model_t* model, const T3DVec3* rot){
	T3DVec3 pos = model_get_position(model);
	t3d_mat4fp_from_srt_euler(&model->matrix, 
		(float[3]){MODEL_SCALE, MODEL_SCALE, MODEL_SCALE},
		&rot->v[0],
		&pos.v[0]
	);
}


void model_draw_uvs(model_t* model){
	t3d_matrix_push(&model->matrix);
	rspq_block_run(model->draw_uvs);
	t3d_matrix_pop(1);
}

void model_draw_shading(model_t* model){
	t3d_matrix_push(&model->matrix);
	rspq_block_run(model->draw_shading);
	t3d_matrix_pop(1);
}

T3DModel* model_get_t3d_model(model_t* model){
	return model->model;
}

T3DObject** model_get_t3d_objects(model_t* model, size_t* objects_len){
	*objects_len = model->objects_len;
	return model->objects;
}

T3DMat4FP* model_get_t3d_matrix(model_t* model){
	return &model->matrix;
}

static aabb_t transform_aabb(const T3DMat4* matrix, const aabb_t* aabb){
	T3DVec3 corners[8] = {
		{{aabb->min.v[0], aabb->min.v[1], aabb->min.v[2]}}, /* min */
		{{aabb->max.v[0], aabb->min.v[1], aabb->min.v[2]}},
		{{aabb->min.v[0], aabb->min.v[1], aabb->max.v[2]}},
		{{aabb->max.v[0], aabb->min.v[1], aabb->max.v[2]}},

		{{aabb->min.v[0], aabb->max.v[1], aabb->min.v[2]}},
		{{aabb->max.v[0], aabb->max.v[1], aabb->min.v[2]}},
		{{aabb->min.v[0], aabb->max.v[1], aabb->max.v[2]}},
		{{aabb->max.v[0], aabb->max.v[1], aabb->max.v[2]}} /* max */
	};

	aabb_t result = {
		.min={{FLT_MAX, FLT_MAX, FLT_MAX}},
		.max={{-FLT_MAX, -FLT_MAX, -FLT_MAX}}
	};
	for (size_t i=0; i<8; ++i){
		T3DVec4 v = {{0,0,0,1}};
		t3d_mat4_mul_vec3(&v, matrix, &corners[i]);
		for(size_t c=0; c<3; ++c){
			result.min.v[c] = fminf(result.min.v[c], v.v[c]);
			result.max.v[c] = fmaxf(result.max.v[c], v.v[c]);
		}
	}
	return result;
}

aabb_t model_get_world_aabb(model_t* model){
	aabb_t aabb = model->aabb;
	/* matrix is stored in fixed point, so convert to float here */
	T3DMat4 mat;
	for(size_t y=0; y<4; ++y){
		for (size_t x=0; x<4; ++x){
			float v = t3d_mat4fp_get_float(&model->matrix, y, x);
			mat.m[y][x] = v;
		}
	}
	aabb = transform_aabb(&mat, &aabb);
	return aabb;
}

