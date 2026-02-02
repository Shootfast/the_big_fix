#ifndef STATE_H
#define STATE_H

#include "level.h"
#include "player.h"
#include "ui.h"
#include "texture_manager.h"
#include "animation_manager.h"
#include "audio_manager.h"
#include "scene_manager.h"

typedef struct state_t {
	ui_t* ui;
	texture_manager_t* tm;
	animation_manager_t* am;
	audio_manager_t* aum;
	scene_manager_t* sm;
	level_t* level;
	player_t* player;
	bool controls_enabled;
	bool zone_triggers_active;
	bool camera_follows_player;
	float interact_cooldown;
	bool first_person;
	bool has_coat;
} state_t;

void state_update(state_t* state, float delta_time);
int state_fire_named_trigger(state_t* state, const char* trigger_name);


#endif /* STATE_H */
