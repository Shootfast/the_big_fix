#ifndef LEVEL_PVT_H
#define LEVEL_PVT_H

#include "level.h"
#include "model.h"
#include "camera.h"
#include "selectable_object.h"
#include "trigger.h"
#include "cutscene.h"
#include "collision.h"
#include "lvl.h"


#define MAX_OBJECT_NAME 256

typedef struct level_t_ {
	model_t* skybox;
	model_t* model;
	camera_t camera;

	aabb_t bounds;

	selectable_object_t* objects;
	size_t objects_len;

	trigger_t* triggers;
	size_t triggers_len;

	cutscene_t* cutscenes;
	size_t cutscenes_len;

	lvl_t* data;
} level_t_;


/* manually load triggers for an object that wasn't available at level start */
void level_load_object_triggers(level_t* level, selectable_object_t* sobj);

#endif /* LEVEL_PVT_H */
