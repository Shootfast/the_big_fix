#include "state.h"
#include "level_pvt.h"
#include "main.h"
#include "trigger.h"

#include <alloca.h>

static int add_to_inventory(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	state->has_coat = true;
	return 0;
}

static int camera_follows_player(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	bool enabled = (strcmp(argv[1], "enable") == 0) ? true : false;
	state->camera_follows_player = enabled;
	return 0;
}
static int camera_near_far(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 3){
		return 1;
	}
	float near;
	sscanf(argv[1], "%f", &near);
	float far;
	sscanf(argv[2], "%f", &far);
	level->camera.near = near;
	level->camera.far = far;
	return 0;
}

static int camera_position(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	T3DVec3 pos;
	sscanf(argv[1], "%f,%f,%f", &pos.v[0], &pos.v[1], &pos.v[2]);
	level->camera.position = pos;
	return 0;
}
static int camera_target(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	T3DVec3 tgt;
	sscanf(argv[1], "%f,%f,%f", &tgt.v[0], &tgt.v[1], &tgt.v[2]);
	level->camera.target = tgt;
	return 0;
}

static int change_scene(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	scene_manager_set_next_scene(state->sm, argv[1]);
	state->sm->swap = true;
	return 0;
}

static int cinematic_mode(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	bool enabled = (strcmp(argv[1], "enable") == 0) ? true : false;
	ui_cinematic_mode(state->ui, enabled);
	return 0;
}

static int condition_check(int argc, char** argv, state_t* state){
	if (!state->has_coat){
		return action_create_and_run(state, "say", "I can't go out without my hat and coat!");
	}
	action_create_and_run(state, "say", "Thanks for playing!");
	return 0;
}


static int examine(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		if (sobj->selected){
			return action_create_and_run(state, "examine_object", sobj->name);
		}
	}
	return 0;
}

static int exit_game(int argc, char** argv, state_t* state){
	abort();
	return 0;
}
static int examine_object(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	selectable_object_t* sobj = NULL;
	for (size_t i=0; i < level->objects_len; ++i){
		sobj = &level->objects[i];
		if (strcmp(sobj->name, argv[1]) == 0){
			break;
		}
		sobj = NULL;
	}
	if (!sobj){
		debugf("examine_object could not find object called '%s'", argv[1]);
		return 1;
	}
	bool found = false;
	for (size_t i=0; i < sobj->triggers_len; ++i){
		trigger_t* trigger = &sobj->triggers[i];
		if (strcmp(trigger->name, "examine") == 0){
			found = true;
			trigger_fire(trigger, state);
			break;
		}
	}

	if (!found){
		/* No special case for this object*/
		return action_create_and_run(state, "say", "I don't see anything special");
	}
	return 0;
}

static int move_player(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	T3DVec3 pos;
	sscanf(argv[1], "%f,%f,%f", &pos.v[0], &pos.v[1], &pos.v[2]);
	state->player->position = pos;
	return 0;
}

static int player_controls(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	bool enabled = (strcmp(argv[1], "enable") == 0) ? true : false;
	state->controls_enabled = enabled;
	return 0;
}

static int play_music(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	audio_clip_t* clip = audio_manager_get_clip(state->aum, argv[1]);
	wav64_set_loop(&clip->clip, true);
	wav64_play(&clip->clip, CHANNEL_MUSIC);
	return 0;
}

static int replace_object(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 3){
		return 1;
	}
	const char* orig_object_name = argv[1];
	const char* new_object_name = argv[2];

	selectable_object_t* orig_object = NULL;
	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		if (strcmp(orig_object_name, sobj->name) == 0){
			orig_object = sobj;
			break;
		}
	}
	if (!orig_object){
		debugf("replace_object could not find original object named '%s'\n", orig_object_name);
		return 1;
	}
	/*TODO check if dst object exists */

	char object_path[MAX_OBJECT_NAME];
	int s = snprintf(&object_path[0], MAX_OBJECT_NAME, "rom:/object_%s.t3dm", new_object_name);

	selectable_object_t new_object;
	T3DVec3 pos = model_get_position(orig_object->model);
	selectable_object_init(&new_object,
	                       new_object_name,
	                       object_path,
	                       &pos,
	                       state->tm);
	new_object.is_decal = orig_object->is_decal;
	new_object.selected = orig_object->selected;
	level_load_object_triggers(level, &new_object);

	selectable_object_deinit(orig_object);
	*orig_object = new_object;
	return 0;
}

static int rotate_player(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	T3DVec3 rot;
	sscanf(argv[1], "%f,%f,%f", &rot.v[0], &rot.v[1], &rot.v[2]);
	state->player->rotation = rot;
	return 0;
}

static int replace_character_model(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	const char* new_model_name = argv[1];

	char model_path[MAX_OBJECT_NAME];
	int s = snprintf(&model_path[0], MAX_OBJECT_NAME, "rom:/%s.t3dm", new_model_name);

	anim_model_t* orig_model = state->player->amodel;
	anim_model_t* new_model = anim_model_alloc(model_path, state->tm);
	state->player->amodel = new_model;
	anim_model_free(orig_model);
	return 0;
}

static int say(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	
	char* argv2[] = {"show_message", argv[1]};
	action_t show = (action_t){
		.argc=2,
		.argv=argv2
	};
	int ret = action_run(&show, state);
	/*TODO look for a voice clip */
	return ret;
}

static int set_camera(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_CAMERA);
	while(lvl_iter_next(&it)){
		lvl_chunk_camera_t* c = it.camera;
		if (strcmp(c->name, argv[1]) == 0){
			memcpy(&level->camera.position.v[0], &c->position[0], sizeof(float)*3);
			memcpy(&level->camera.target.v[0], &c->target[0], sizeof(float)*3);
			return 0;
		}
	}
	debugf("set_camera called with unknown camera '%s'\n", argv[1]);
	return 1;
}

static int show_message(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	ui_write_message(state->ui, argv[1], UI_TEXT_IMMEDIATE, 3000);
	return 0;
}

static int start_cutscene(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	const char* cutscene_name = argv[1];
	for (size_t i=0; i < level->cutscenes_len; ++i){
		cutscene_t* cutscene = &level->cutscenes[i];
		if (strcmp(cutscene->name, cutscene_name) == 0){
			animation_manager_start_cutscene(state->am, cutscene);
			return 0;
		}
	}
	debugf("start_cutscene could not find cutscene named '%s'\n", cutscene_name);
	return 0;
}

static int take(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	for (size_t i=0; i < level->objects_len; ++i){
		selectable_object_t* sobj = &level->objects[i];
		if (sobj->selected){
			return action_create_and_run(state, "take_object", sobj->name);
		}
	}
	return 0;
}

static int take_object(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	selectable_object_t* sobj = NULL;
	for (size_t i=0; i < level->objects_len; ++i){
		sobj = &level->objects[i];
		if (strcmp(sobj->name, argv[1]) == 0){
			break;
		}
		sobj = NULL;
	}
	if (!sobj){
		debugf("take_object could not find object called '%s'", argv[1]);
		return 1;
	}
	bool found = false;
	for (size_t i=0; i < sobj->triggers_len; ++i){
		trigger_t* trigger = &sobj->triggers[i];
		if (strcmp(trigger->name, "take") == 0){
			found = true;
			trigger_fire(trigger, state);
			break;
		}
	}

	if (!found){
		/* No special case for this object*/
		return action_create_and_run(state, "say", "I don't want to take that");
	}
	return 0;
}

static int toggle_light(int argc, char** argv, state_t* state){
	level_t* level = state->level;
	if (argc != 2){
		return 1;
	}
	lvl_iter_t it = lvl_iter_create(level->data, LVL_CHUNK_TYPE_LIGHT);	
	while(lvl_iter_next(&it)){
		if (strcmp(it.light->name, argv[1]) == 0){
			it.light->enabled = !it.light->enabled;
			return 0;
		}
	}
	debugf("toggle_light called with unknown light '%s'\n", argv[1]);
	return 1;
}

static int zone_triggers(int argc, char** argv, state_t* state){
	if (argc != 2){
		return 1;
	}
	bool enabled = (strcmp(argv[1], "enable") == 0) ? true : false;
	state->zone_triggers_active = enabled;
	return 0;
}

int action_run(action_t* action, void* state){
	state_t* s = (state_t*) state;
	if (strcmp(action->argv[0], "add_to_inventory") == 0){
		return add_to_inventory(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "camera_follows_player") == 0){
		return camera_follows_player(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "camera_near_far") == 0){
		return camera_near_far(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "camera_position") == 0){
		return camera_position(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "camera_target") == 0){
		return camera_target(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "change_scene") == 0){
		return change_scene(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "cinematic_mode") == 0){
		return cinematic_mode(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "condition_check") == 0){
		return condition_check(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "examine") == 0){
		return examine(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "exit_game") == 0){
		return exit_game(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "examine_object") == 0){
		return examine_object(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "move_player") == 0){
		return move_player(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "player_controls") == 0){
		return player_controls(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "play_music") == 0){
		return play_music(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "replace_character_model") == 0){
		return replace_character_model(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "replace_object") == 0){
		return replace_object(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "rotate_player") == 0){
		return rotate_player(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "say") == 0){
		return say(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "set_camera") == 0){
		return set_camera(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "show_message") == 0){
		return show_message(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "start_cutscene") == 0){
		return start_cutscene(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "take") == 0){
		return take(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "take_object") == 0){
		return take_object(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "toggle_light") == 0){
		return toggle_light(action->argc, action->argv, s);
	} else if (strcmp(action->argv[0], "zone_triggers") == 0){
		return zone_triggers(action->argc, action->argv, s);
	}
	debugf("Error: Unknown action '%s'\n", action->argv[0]);
	return 1;
}

int action_create_and_run_argc(void* state, size_t argc, ...){
	state_t* s = (state_t*) state;
	char** argv = alloca(sizeof(char*) * argc);
	va_list args;
	va_start(args, argc);
	for (size_t i=0; i < argc; ++i){
		const char* arg = va_arg(args, const char*);
		size_t arg_len = strlen(arg);
		argv[i] = alloca(arg_len+1);
		memcpy(argv[i], arg, arg_len+1);
	}
	va_end(args);
	action_t action = (action_t){
		.argc=argc,
		.argv=argv
	};
	int ret = action_run(&action, s);
	return ret;
}
