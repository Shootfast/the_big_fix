#include "level.h"
#include "level_pvt.h" /* level_t */

#include "lvl.h"
#include "action.h"
#include "model.h"
#include "collision.h"

#include "jsmn_helpers.h"

#define MAX_LEVEL_NAME 256
#define MAX_JSON_TOKENS 512

static const char* get_json(level_t* level){
	const char* js = NULL;
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_JSON);
	while(lvl_iter_next(&it)){
		lvl_chunk_json_t* j = it.json;
		if (js){
			debugf("Warning: Multiple json chunks in lvl\n");
		}
		js = j->json;
	}
	return js;
}

static int action_init_jsmn(action_t* action, const char* js, const jsmntok_t* argc_tok){
	size_t offset = 0;
	size_t argc = argc_tok->size;
	char** argv = malloc(sizeof(char*)* argc);
	for(size_t i=0; i < argc; ++i){
		const jsmntok_t* v = argc_tok + 1 + i;
		if (v->type != JSMN_STRING){
			debugf("Error: action has non string argument\n");
			debugf("Relevant json: %.*s\n", v->end - v->start, js+v->start);
			return -1;
		} else {
			argv[i] = strndup(js+v->start, v->end - v->start);
		}
		offset += 1;
	}
	offset += 1;
	action->argc = argc;
	action->argv = argv;
	return offset;
}

static int trigger_init_jsmn(trigger_t* trigger, const char* js, const jsmntok_t* value){
	if (value->type != JSMN_ARRAY){
		debugf("Error: value of trigger '%s' is not an array\n", trigger->name);
		return 1;
	}

	trigger->actions = NULL;
trigger->actions_len = 0;

	if (value->size <= 0){
		debugf("Error: no value for trigger '%s'\n", trigger->name);
		return 1;
	}
	/* Check that trigger is array of actions */
	if ((value+1)->type != JSMN_ARRAY){
		debugf("Error: value of trigger '%s' must be array of array\n", trigger->name);
		return 1;
	}
	trigger->actions_len = value->size;	
	trigger->actions = malloc(sizeof(action_t)*trigger->actions_len);
	if (!trigger->actions){
		debugf("Error: could not allocate actions for trigger %s\n", trigger->name);
		return 1;
	}

	size_t offset =0;
	for (size_t action_idx=0; action_idx < trigger->actions_len; ++action_idx){
		const jsmntok_t* argc_tok = value + 1 + offset;
		action_t* action = &trigger->actions[action_idx];
		int ret = action_init_jsmn(action, js, argc_tok);
		if (ret < 0){
			debugf("Error whilst setting action in trigger %s\n", trigger->name);
			return 1;
		}
		offset += ret;
	}
	return 0;
}

typedef struct trigger_helper_t {
	level_t* level;
	trigger_t* triggers;
	size_t idx;
} trigger_helper_t;

/* called to load each trigger of an individual object */
static void trigger_loader(const char* js, const jsmntok_t* key, const jsmntok_t* value, size_t remaining, void* user){
	trigger_helper_t* help = (trigger_helper_t*)user;
	trigger_t* trigger = &help->triggers[help->idx++];
	trigger->name = strndup(js+key->start, key->end - key->start);
	if (trigger_init_jsmn(trigger, js, value) != 0){
		return;
	}
}

static void level_load_object_triggers_jsmn(level_t* level, selectable_object_t* sobj, const char* js, const jsmntok_t* value, size_t remaining){
	/*allocate sobj->triggers to value->size */
	if (sobj->triggers){
		debugf("Error: Selectable object '%s' already has triggers allocated\n", sobj->name);
		return;
	}
	sobj->triggers_len = value->size;
	sobj->triggers = malloc(sizeof(trigger_t) * sobj->triggers_len);
	if (!sobj->triggers){
		debugf("Error: Could not allocate triggers for Selectable object '%s'\n", sobj->name);
		return;
	}

	/* run the individual trigger loaders */
	trigger_helper_t help = {
		.level = level,
		.triggers = sobj->triggers,
		.idx = 0
	};
	jsmn_iteritem(js, value, remaining, trigger_loader, &help);
}

/* called over all objects */
static void objects_loader(const char* js, const jsmntok_t* key, const jsmntok_t* value, size_t remaining, void* user){
	trigger_helper_t* help = (trigger_helper_t*)user;

	if (value->type != JSMN_OBJECT){
		debugf("Error: value of objects key '%.*s' must be an object\n", key->end - key->start, js+key->start);
		return;
	}

	/*find selectable_object_t by key name */
	selectable_object_t* sobj = NULL;
	for(size_t i=0; i < help->level->objects_len; ++i){
		sobj = &help->level->objects[i];
		if (strncmp(js+key->start, sobj->name, key->end - key->start) == 0){
			break; 
		}
		sobj = NULL;
	}
	if (!sobj){
		debugf("Warning: Could not find object '%.*s' to load actions\n", key->end - key->start, js+key->start);
		return;
	}

	level_load_object_triggers_jsmn(help->level, sobj, js, value, remaining);
}

typedef struct cutscene_helper_t {
	level_t* level;
	cutscene_t* cutscene;
	size_t idx;
} cutscene_helper_t;

static void cutscene_event_loader(const char* js, const jsmntok_t* value, size_t remaining, void* user){
	cutscene_helper_t* help = (cutscene_helper_t*)user;
	cutscene_t* cutscene = help->cutscene;
	if (value->type != JSMN_ARRAY){
		debugf("Error: event item %d in cutscene '%s' is not an array\n", help->idx, cutscene->name);
		return;
	}
	if (value->size != 2){
		debugf("Error: event item %d in cutscene '%s' does not have 2 elements [timestamp, [actions]] (got %d)\n",
				help->idx, cutscene->name, value->size);
		return;
	}

	const jsmntok_t* ts = value+1;
	if ((ts)->type != JSMN_PRIMITIVE){
		debugf("Error: element 0 of event item %d in cutscene '%s' is not a timestamp integer\n",
			help->idx, cutscene->name);
		debugf("Relevant json: %.*s\n", ts->end - ts->start, js+ts->start);
		debugf("%.*s\n", value->end - value->start, js+value->start);
		return;
	}
	const jsmntok_t* actions = value+2;
	if ((actions)->type != JSMN_ARRAY){
		debugf("Error: element 1 of event item %d in cutscene '%s' is not an array\n",
			help->idx, cutscene->name);
		return;
	}
	cutscene_event_t* event = &cutscene->events[help->idx++];
	char* val = strndup(js+ts->start, ts->end - ts->start);
	char* end;
	event->timestamp = strtoul(val, &end, 10);
	free(val);
	
	event->actions_len = actions->size;
	event->actions = malloc(sizeof(action_t)*event->actions_len);
	if (!event->actions){
		debugf("Error: could not allocate actions for cutscene '%s'\n", cutscene->name);
		return;
	}
	size_t offset = 0;
	for (size_t action_idx=0; action_idx < event->actions_len; ++action_idx){
		const jsmntok_t* argc_tok = actions + 1 + offset;
		action_t* action = &event->actions[action_idx];
		int ret = action_init_jsmn(action, js, argc_tok);
		if (ret < 0){
			debugf("Error whilst setting action in cutscene '%s'\n", cutscene->name);
			return;
		}
		offset += ret;
	}
	return;
}

static void cutscenes_loader(const char* js, const jsmntok_t* key, const jsmntok_t* value, size_t remaining, void* user){
	cutscene_helper_t* help = (cutscene_helper_t*)user;
	if (value->type != JSMN_ARRAY){
		debugf("Error: value of cutscenes key '%.*s' must be an array\n", key->end - key->start, js+key->start);
		return;
	}
	cutscene_t* cutscene = &help->level->cutscenes[help->idx++];
	size_t n_events = value->size;
	char* name = strndup(js+key->start, key->end - key->start);
	cutscene_init(cutscene, name, n_events);
	free(name);
	
	cutscene_helper_t event_help = (cutscene_helper_t){
		.level = help->level,
		.cutscene = cutscene,
		.idx=0
	};

	jsmn_itervalue(js, value, remaining+1, cutscene_event_loader, &event_help);
}

static void load_json(level_t* level){
	const char* js = get_json(level);
	if (!js){
		return;
	}

	jsmn_parser p;
	jsmn_init(&p);
	jsmntok_t tokens[MAX_JSON_TOKENS];
	int r = jsmn_parse(&p, js, strlen(js), &tokens[0], MAX_JSON_TOKENS);
	if (r < 0){
		debugf("Warning: Insufficient memory for JSON parsing. Raise MAX_JSON_TOKENS\n");
	}
	size_t remaining = p.toknext;

	int value_idx = jsmn_find_key("objects", js, &tokens[0], remaining);
	if (value_idx > 0){
		remaining -= value_idx;
		jsmntok_t* objects = &tokens[value_idx];
		trigger_helper_t help = {
			.level = level,
			.triggers = NULL,
			.idx = 0
		};
		jsmn_iteritem(js, objects, remaining, objects_loader, &help);
	}
	
	remaining = p.toknext;
	value_idx = jsmn_find_key("triggers", js, &tokens[0], remaining);
	if (value_idx > 0){
		remaining -= value_idx;
		jsmntok_t* triggers = &tokens[value_idx];
		level->triggers_len = triggers->size;
		if (level->triggers_len){
			level->triggers = malloc(sizeof(trigger_t) * level->triggers_len);
		}
		if (!level->triggers){
			debugf("Error: Could not allocate triggers\n");
			return;
		}
		trigger_helper_t help = {
			.level = level,
			.triggers = level->triggers,
			.idx = 0
		};
		jsmn_iteritem(js, triggers, remaining, trigger_loader, &help);
	}

	remaining = p.toknext;
	value_idx = jsmn_find_key("cutscenes", js, &tokens[0], remaining);
	if (value_idx > 0){
		remaining -= value_idx;
		jsmntok_t* cutscenes = &tokens[value_idx];
		level->cutscenes_len = cutscenes->size;
		if (level->cutscenes_len){
			level->cutscenes = malloc(sizeof(cutscene_t) * level->cutscenes_len);
		}
		if (!level->cutscenes){
			debugf("Error: Could not allocate cutscenes\n");
			return;
		}
		cutscene_helper_t help = {
			.level = level,
			.cutscene = NULL,
			.idx = 0
		};
		jsmn_iteritem(js, cutscenes, remaining, cutscenes_loader, &help);
	}
}

void level_load_object_triggers(level_t* level, selectable_object_t* sobj){
	const char* js = get_json(level);
	if (!js){
		return;
	}
	jsmn_parser p;
	jsmn_init(&p);
	jsmntok_t tokens[MAX_JSON_TOKENS];
	int r = jsmn_parse(&p, js, strlen(js), &tokens[0], MAX_JSON_TOKENS);
	if (r < 0){
		debugf("Warning: Insufficient memory for JSON parsing. Raise MAX_JSON_TOKENS\n");
	}
	size_t remaining = p.toknext;

	int value_idx = jsmn_find_key("objects", js, &tokens[0], remaining);
	if (value_idx > 0){
		remaining -= value_idx;
		value_idx += jsmn_find_key(sobj->name, js, &tokens[value_idx], remaining);
		if (value_idx > 0){
			remaining -= value_idx;
			level_load_object_triggers_jsmn(level, sobj, js, &tokens[value_idx], remaining);
		}
	}
}

static void load_camera(level_t* level){
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_CAMERA);
	while(lvl_iter_next(&it)){
		lvl_chunk_camera_t* c = it.camera;
		memcpy(&level->camera.position.v[0], &c->position[0], sizeof(float)*3);
		memcpy(&level->camera.target.v[0], &c->target[0], sizeof(float)*3);
		break; /*load the first camera*/
	}
}


static bool load_objects(level_t* level, texture_manager_t* tm){
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_OBJECT);
	size_t n_objects = 0;
	while(lvl_iter_next(&it)){
		++n_objects;
	}
	level->objects = malloc(sizeof(selectable_object_t) * n_objects);
	if (!level->objects){
		return false;
	}
	level->objects_len = n_objects;

	it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_OBJECT);
	size_t i=0;

	bool success = true;
	while(lvl_iter_next(&it)){
		lvl_chunk_object_t* obj = it.object;
		char object_path[MAX_OBJECT_NAME];
		int s = snprintf(&object_path[0], MAX_OBJECT_NAME, "rom:/object_%s.t3dm", obj->name);
		if (s < 0){
			success = false;
			break;
		}
		object_path[s] = '\0';
		selectable_object_t* sobj = &level->objects[i];
		T3DVec3 object_position = (T3DVec3){{obj->position[0], obj->position[1], obj->position[2]}};
		selectable_object_init(sobj, obj->name, object_path, &object_position, tm);
		sobj->is_decal = obj->is_decal;
		++i;
	}
	if (!success){
		for (size_t idx=0; idx != i; ++idx){
			selectable_object_t* sobj = &level->objects[idx];
			selectable_object_deinit(sobj);
		}
		free(level->objects);
		level->objects_len = 0;
	}
	return success;
}

static void set_bounds(level_t* level){
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_BOUNDS_AABB);
	while(lvl_iter_next(&it)){
		lvl_chunk_bounds_aabb_t* bounds = it.bounds;
		memcpy(&level->bounds.min.v[0], &bounds->aabb_min[0], sizeof(float)*3);
		memcpy(&level->bounds.max.v[0], &bounds->aabb_max[0], sizeof(float)*3);
	}
}


level_t* level_alloc(const char* name, texture_manager_t* tm){
	level_t* level = malloc(sizeof(level_t));
	if (!level){
		return NULL;
	}
	level->skybox = model_alloc("rom:/skybox.t3dm", tm);

	char model_path[MAX_LEVEL_NAME];
	int s = snprintf(&model_path[0], MAX_LEVEL_NAME, "rom:/%s.lvl.t3dm", name);
	if (s < 0){
		free(level);
		return NULL;
	}
	model_path[s] = '\0';
	level->model = model_alloc(model_path, tm);

	/* Remove the ".t3dm" suffix from model_path to get lvl data file name */
	char* last_dot = strrchr(model_path, '.');
	if (last_dot){
		*last_dot = '\0';
	}
	level->data = load_lvl(model_path);
	load_objects(level, tm);
	set_bounds(level);

	level->camera = camera_create();
	load_camera(level);

	level->triggers = NULL;
	level->triggers_len = 0;

	level->cutscenes = NULL;
	level->cutscenes_len = 0;
	load_json(level);

	return level;
}

void level_free(level_t* level){
	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		selectable_object_deinit(sobj);
	}
	free(level->objects);
	free(level->data);
	camera_destroy(&level->camera);
	model_free(level->model);
	model_free(level->skybox);
	free(level);
}


void level_draw_uvs(level_t* level){
	model_draw_uvs(level->skybox);
	model_draw_uvs(level->model);

	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		if (sobj->is_decal){
			t3d_state_set_depth_offset(-0x40);
		}
		model_draw_uvs(sobj->model);
		t3d_state_set_depth_offset(0);
	}
}

void level_draw_shading(level_t* level, uint8_t (*ambient_light_color)[4], uint8_t (*selected_light_color)[4]){
	t3d_light_set_ambient(*ambient_light_color);
	model_draw_shading(level->model);

	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		if (sobj->selected){
			t3d_light_set_ambient(*selected_light_color);
		} else {
			t3d_light_set_ambient(*ambient_light_color);
		}
		model_draw_shading(sobj->model);
		/* reset lighting after drawing selected */
		t3d_light_set_ambient(*ambient_light_color);
	}
}

camera_t* level_get_active_camera(level_t* level){
	return &level->camera;
}

void level_set_lights(level_t* level){
	size_t n_lights = 0;
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_LIGHT);
	while(lvl_iter_next(&it)){
		if (it.light->enabled == 1){
			++n_lights;
		}
	}
	if (n_lights > 7){
		debugf("WARNING: Too many lights set. Only the first 7 lights will render\n");
		n_lights = 7;
	}
	t3d_light_set_count(n_lights);

	it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_LIGHT);
	size_t idx = 0;
	while(lvl_iter_next(&it)){
		if (idx > n_lights -1){
			break;
		}
		if (!it.light->enabled){
			continue;
		}
		lvl_chunk_light_t* light = it.light;
		uint8_t color[4] = {
			255 * fminf(light->color[0], 1.0),
			255 * fminf(light->color[1], 1.0),
			255 * fminf(light->color[2], 1.0),
			0xFF
		};
		T3DVec3 pos = {{
			light->position[0],
			light->position[1],
			light->position[2]
		}};
		if (light->is_directional){
			t3d_light_set_directional(idx, &color[0], &pos);
		} else {
			/* convert from gltf lm/r^2 to Blender Watts and clamp at 10000W */
			const float WATTS_TO_LUMENS = 683;
			const float MAX_WATTS = 10000;
			float watts = (light->intensity / WATTS_TO_LUMENS) * (4*M_PI);
			watts = fminf(watts, MAX_WATTS);
			float size = watts / MAX_WATTS;
			t3d_light_set_point(idx, &color[0], &pos, size, /*ignoreNormals=*/true);
		}
		idx++;
	}
}

bool level_check_bounds(level_t* level, T3DVec3* pos){
	if (point_inside_aabb(pos, &level->bounds)){
		/* also check colliders */
		lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_COLLIDER_AABB);
		while(lvl_iter_next(&it)){
			lvl_chunk_collider_aabb_t* col = it.collider;
			aabb_t collider;
			memcpy(&collider.min.v[0], &col->aabb_min[0], sizeof(float)*3);
			memcpy(&collider.max.v[0], &col->aabb_max[0], sizeof(float)*3);
			if (point_inside_aabb(pos, &collider)){
				return false;
			}
		}
		return true;
	}
	return false;
}




void level_objects_check_interactive(level_t* level, ray_t* ray){
	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		bool selected = false;
		aabb_t aabb = model_get_world_aabb(sobj->model);
		if (ray_intersects_aabb(ray, &aabb, /*ignore_y=*/true)){
			selected = true;
		}
		T3DVec3 obj_pos = model_get_position(sobj->model);
		T3DVec3 player_pos = ray->origin;
		/* ignore height for distance comparison */
		obj_pos.v[1] = 0;
		player_pos.v[1] = 0;

		T3DVec3 diff;
		t3d_vec3_diff(&diff, &player_pos, &obj_pos);
		float distance = sqrtf(t3d_vec3_len2(&diff));

		if (distance > 1.5f){
			selected = false;
		}
		sobj->selected = selected;
	}
}


