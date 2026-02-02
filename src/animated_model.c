#include "animated_model.h"
#include "frame_buffers.h"
#include <t3d/t3dskeleton.h>
#include <t3d/t3danim.h>

typedef struct anim_model_t_ {
	model_t* model;
	T3DSkeleton skel;
	T3DSkeleton skel_blend;

	rspq_block_t* draw_uvs;
	rspq_block_t* draw_shading;

	T3DAnim anim_idle;
	T3DAnim anim_walk;
} anim_model_t_;

static inline T3DMat4FP* get_bone_matrices(T3DSkeleton* skel){
	if (skel){
		if (skel->bufferCount == 1){
			return skel->boneMatricesFP;
		} else {
			return t3d_segment_placeholder(T3D_SEGMENT_SKELETON);
		}
	}
	return NULL;
}

static rspq_block_t* uv_calls(T3DObject** objects, size_t objects_len, T3DSkeleton* skel){
	rspq_block_begin();
		T3DModelState model_state = t3d_model_state_create();
		T3DModelDrawConf conf = (T3DModelDrawConf){
			.matrices = get_bone_matrices(skel)
		};
		model_state.drawConf = &conf;
		for(size_t i=0; i<objects_len; ++i){
			T3DObject* obj = objects[i];
			t3d_model_draw_material(obj->material, &model_state);
			t3d_model_draw_object(obj, conf.matrices);
		}
		t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
	return rspq_block_end();
}

static rspq_block_t* shading_calls(T3DObject** objects, size_t objects_len, T3DSkeleton* skel){
	rspq_block_begin();
		rdpq_sync_pipe();
		rdpq_mode_combiner(RDPQ_COMBINER1((1, SHADE, PRIM, 0), (0, 0, 0, 1)));
		rdpq_mode_blender(0);
		rdpq_mode_alphacompare(0);
		rdpq_set_prim_color((color_t){0xFF, 0xFF, 0xFF, 0xFF});
		t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_CULL_BACK);

		T3DMat4FP* bone_matrices = get_bone_matrices(skel);
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
			t3d_model_draw_object(obj, bone_matrices);
		}
	return rspq_block_end();
}

anim_model_t* anim_model_alloc(const char* path, texture_manager_t* tm){
	anim_model_t* amodel = malloc(sizeof(anim_model_t_));
	if (!amodel){
		return NULL;
	}
	amodel->model = model_alloc(path, tm);
	if (!amodel->model){
		free(amodel);
		return NULL;
	}

	T3DModel* m = model_get_t3d_model(amodel->model);
	amodel->skel = t3d_skeleton_create_buffered(m, FB_COUNT);
	amodel->skel_blend = t3d_skeleton_clone(&amodel->skel, false);

	amodel->anim_idle = t3d_anim_create(m, "Idle");
	t3d_anim_attach(&amodel->anim_idle, &amodel->skel);

	amodel->anim_walk = t3d_anim_create(m, "Walk");
	t3d_anim_attach(&amodel->anim_walk, &amodel->skel_blend);

	size_t objects_len = 0;
	T3DObject** objects = model_get_t3d_objects(amodel->model, &objects_len);
	amodel->draw_uvs = uv_calls(objects, objects_len, &amodel->skel);
	amodel->draw_shading = shading_calls(objects, objects_len, &amodel->skel);

	return amodel;
}

void anim_model_free(anim_model_t* amodel){
	rspq_block_free(amodel->draw_uvs);
	rspq_block_free(amodel->draw_shading);

	t3d_skeleton_destroy(&amodel->skel);
	t3d_skeleton_destroy(&amodel->skel_blend);

	t3d_anim_destroy(&amodel->anim_idle);
	t3d_anim_destroy(&amodel->anim_walk);
	model_free(amodel->model);
	free(amodel);
}

model_t* anim_model_get_model(anim_model_t* amodel){
	return amodel->model;
}

void anim_model_update(anim_model_t* amodel, float delta_time, float anim_blend){
	t3d_anim_update(&amodel->anim_idle, delta_time);
	t3d_anim_update(&amodel->anim_walk, delta_time);

	t3d_skeleton_blend(&amodel->skel, &amodel->skel, &amodel->skel_blend, anim_blend);

	t3d_skeleton_update(&amodel->skel);
}

void anim_model_draw_uvs(anim_model_t* amodel){
	t3d_skeleton_use(&amodel->skel);
	t3d_matrix_push(model_get_t3d_matrix(amodel->model));
	rspq_block_run(amodel->draw_uvs);
	t3d_matrix_pop(1);
}
void anim_model_draw_shading(anim_model_t* amodel){
	t3d_skeleton_use(&amodel->skel);
	t3d_matrix_push(model_get_t3d_matrix(amodel->model));
	rspq_block_run(amodel->draw_shading);
	t3d_matrix_pop(1);
}
