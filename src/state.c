#include "state.h"
#include "level_pvt.h"
#include "collision.h"

int state_fire_named_trigger(state_t* state, const char* trigger_name){
	for(size_t i=0; i < state->level->triggers_len; ++i){
		trigger_t* trigger = &state->level->triggers[i];
		if (strcmp(trigger->name, trigger_name) == 0){
			return trigger_fire(trigger, state);
		}
	}
	debugf("Error: state_fire_named_trigger could not find a trigger named '%s'\n", trigger_name); 
	return 1;
}

static void check_zone_triggers(state_t* state){
	lvl_iter_t it = lvl_iter_create(state->level->data, LVL_CHUNK_TYPE_ZONE_TRIGGER);
	while(lvl_iter_next(&it)){
		lvl_chunk_zone_trigger_t* z = it.zone;
		aabb_t bounds;
		memcpy(&bounds.min.v[0], &z->aabb_min[0], sizeof(float)*3);
		memcpy(&bounds.max.v[0], &z->aabb_max[0], sizeof(float)*3);
		bool should_fire = false;
		if (point_inside_aabb(&state->player->position, &bounds)){
			should_fire = true;
		} else {
			/* if we are outside the trigger, reset the fired_previously state */
			z->fired_previously = 0;
		}
		if (should_fire && z->fire_once && z->fired_previously){
			should_fire = false;
		}
		if (should_fire){
			int ret = state_fire_named_trigger(state, z->name);
			if (ret == 0){
				z->fired_previously = 1;
			} else {
				debugf("Error whilst firing trigger '%s'\n", z->name);
			}
		}
	}
}

static void point_camera_to_player(state_t* state){
	T3DVec3 camera_target = state->player->position;
	camera_target.v[1] += 1.5f;
	state->level->camera.target = camera_target;
}

void state_update(state_t* state, float delta_time){
	joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);

	/* Player rotation and position */
	T3DVec3 orig_pos=state->player->position;
	T3DVec3 pos=state->player->position;
	T3DVec3 rot=state->player->rotation;

	if (state->controls_enabled){
		rot.v[1] += joypad.stick_x * delta_time * 0.1;
	}
	T3DVec3 dir = {{ -fm_sinf(rot.v[1]), 0, fm_cosf(rot.v[1])}};
	t3d_vec3_norm(&dir);

	if (state->controls_enabled){
		for (size_t i=0; i<3; ++i){
			pos.v[i] += dir.v[i] * joypad.stick_y * delta_time * 0.1;
		}
	}


	/* Check movement and activate zone triggers */
	bool in_bounds = level_check_bounds(state->level, &pos);
	if (in_bounds){
		state->player->position = pos;
	}
	state->player->rotation = rot;
	if (state->zone_triggers_active){
		check_zone_triggers(state);
	}
	if (state->camera_follows_player){
		point_camera_to_player(state);
	}
	camera_update(&state->level->camera);

	/* Animation walking speed */
	T3DVec3 diff;
	t3d_vec3_diff(&diff, &orig_pos, &state->player->position);
	float speed = sqrtf(t3d_vec3_len2(&diff));
	state->player->walking_speed = speed;
	player_update(state->player, delta_time);

	/* create ray to check for interaction */
	ray_t ray = ray_create(&state->player->position, &dir);
	level_objects_check_interactive(state->level, &ray);

	/* cooldown to only allow interactions once per second */
	state->interact_cooldown = fmaxf(0, state->interact_cooldown - delta_time);
	if (joypad.btn.a && state->controls_enabled){
		if (state->interact_cooldown == 0){
			action_create_and_run(state, "examine");
			state->interact_cooldown = 1.0;
		}
	}
	if (joypad.btn.b && state->controls_enabled){
		if (state->interact_cooldown == 0){
			action_create_and_run(state, "take");
			state->interact_cooldown = 1.0;
		}
	}

	/* first person when z button held */
	state->first_person = joypad.btn.z && state->controls_enabled;

	animation_manager_update(state->am, state);
}
