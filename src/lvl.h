#ifndef LVL_H
#define LVL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* lvl file format */
#define LVL_VERSION 1

typedef enum lvl_chunk_type {
	LVL_CHUNK_TYPE_COLLIDER_AABB = 'A',
	LVL_CHUNK_TYPE_BOUNDS_AABB   = 'B',
	LVL_CHUNK_TYPE_CAMERA        = 'C',
	LVL_CHUNK_TYPE_JSON          = 'J',
	LVL_CHUNK_TYPE_LIGHT         = 'L',
	LVL_CHUNK_TYPE_OBJECT        = 'O',
	LVL_CHUNK_TYPE_ZONE_TRIGGER  = 'Z'
} lvl_chunk_type;

typedef struct lvl_chunk_collider_aabb_t {
	char* name;
	float aabb_min[3];
	float aabb_max[3];
} lvl_chunk_collider_aabb_t;

typedef struct lvl_chunk_bounds_aabb_t {
	float aabb_min[3];
	float aabb_max[3];
} lvl_chunk_bounds_aabb_t;

typedef struct lvl_chunk_camera_t {
	char* name;
	float position[3];
	float target[3];
} lvl_chunk_camera_t;

typedef struct lvl_chunk_object_t {
	char* name;
	float position[3];
	uint32_t is_decal;
} lvl_chunk_object_t;

typedef struct lvl_chunk_json_t {
	char* json;
} lvl_chunk_json_t;

typedef struct lvl_chunk_light_t {
	char* name;
	float position[3];
	float color[3];
	float intensity;
	uint32_t is_directional;
	uint32_t enabled;
} lvl_chunk_light_t;

typedef struct lvl_chunk_zone_trigger_t {
	char* name;
	float aabb_min[3];
	float aabb_max[3];
	uint32_t fire_once;
	uint32_t fired_previously;
} lvl_chunk_zone_trigger_t;


typedef union lvl_chunk_offset_t {
	char type;
	uint32_t offset;
} lvl_chunk_offset_t;

typedef struct lvl_t {
	char magic[4];
	uint32_t chunk_count;
	uint32_t string_table_offset;
	lvl_chunk_offset_t chunk_offsets[];
} lvl_t;

typedef struct lvl_iter_t {
	union {
		void* chunk;
		lvl_chunk_collider_aabb_t* collider;
		lvl_chunk_camera_t* camera;
		lvl_chunk_bounds_aabb_t* bounds;
		lvl_chunk_object_t* object;
		lvl_chunk_json_t* json;
		lvl_chunk_light_t* light;
		lvl_chunk_zone_trigger_t* zone;
	};
	const lvl_t* lvl;
	size_t idx;
	lvl_chunk_type type;
} lvl_iter_t;


lvl_t* load_lvl(const char* lvl_file);

static inline lvl_iter_t lvl_iter_create(lvl_t* lvl, lvl_chunk_type type){
	return (lvl_iter_t){
		.chunk=NULL,
		.lvl=lvl,
		.idx=0,
		.type=type};
}
bool lvl_iter_next(lvl_iter_t* iter);

#endif /* LVL_H */
