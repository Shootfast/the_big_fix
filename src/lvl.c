#include "lvl.h"
#include <libdragon.h>

static void* patch_pointer(void* offset, void* root){
	return (void*)((uint8_t*)root + (uint32_t)offset);
}

lvl_t* load_lvl(const char* lvl_file){
	int size = 0;
	lvl_t* data = asset_load(lvl_file, &size);
	if (!data){
		return NULL;
	}
	for (size_t i=0; i < data->chunk_count; ++i){
		enum lvl_chunk_type type = data->chunk_offsets[i].type;
		uint32_t offset = data->chunk_offsets[i].offset & 0x00FFFFFF;
		if (type == LVL_CHUNK_TYPE_COLLIDER_AABB){
			lvl_chunk_collider_aabb_t* col = (lvl_chunk_collider_aabb_t*)((uint8_t*)data + offset);
			col->name = patch_pointer(col->name, (uint8_t*)data + data->string_table_offset);
			debugf("loaded collider with name %s\n", col->name);
		} else if (type == LVL_CHUNK_TYPE_CAMERA){
			lvl_chunk_camera_t* cam = (lvl_chunk_camera_t*)((uint8_t*)data + offset);
			cam->name = patch_pointer(cam->name, (uint8_t*)data + data->string_table_offset);
			debugf("loaded camera with name %s\n", cam->name);
		} else if (type == LVL_CHUNK_TYPE_OBJECT){
			lvl_chunk_object_t* obj = (lvl_chunk_object_t*)((uint8_t*)data + offset);
			obj->name = patch_pointer(obj->name, (uint8_t*)data + data->string_table_offset);
			debugf("loaded object with name %s\n", obj->name);
		} else if (type == LVL_CHUNK_TYPE_JSON){
			lvl_chunk_json_t* j = (lvl_chunk_json_t*)((uint8_t*)data + offset);
			j->json = patch_pointer(j->json, (uint8_t*)data + data->string_table_offset);
			debugf("loaded json\n");
		} else if (type == LVL_CHUNK_TYPE_LIGHT){
			lvl_chunk_light_t* light = (lvl_chunk_light_t*)((uint8_t*)data + offset);
			light->name = patch_pointer(light->name, (uint8_t*)data + data->string_table_offset);
			debugf("loaded light with name %s\n", light->name);
		} else if (type == LVL_CHUNK_TYPE_ZONE_TRIGGER){
			lvl_chunk_zone_trigger_t* z = (lvl_chunk_zone_trigger_t*)((uint8_t*)data + offset);
			z->name = patch_pointer(z->name, (uint8_t*)data + data->string_table_offset);
			debugf("loaded zone trigger with name %s\n", z->name);
		}

	}
	return data;
}

bool lvl_iter_next(lvl_iter_t* iter){
	for(; iter->idx < iter->lvl->chunk_count; ++iter->idx){
		if (iter->lvl->chunk_offsets[iter->idx].type == iter->type){
			uint32_t offset = iter->lvl->chunk_offsets[iter->idx].offset & 0x00FFFFFF;
			iter->chunk = (uint8_t*)iter->lvl + offset;
			++iter->idx;
			return true;
		}
	}
	iter->chunk = NULL;
	return false;
}
