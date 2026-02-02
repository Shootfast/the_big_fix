#include "animation_manager.h"
#include "state.h"
#include <stdlib.h>

typedef struct animation_manager_t_ {
	cutscene_t* current_cutscene;
	uint64_t start_time;
	size_t next_event_idx;
} animation_manager_t_;


animation_manager_t* animation_manager_alloc(){
	animation_manager_t* am = malloc(sizeof(animation_manager_t));
	am->current_cutscene = NULL;
	am->start_time = 0;
	am->next_event_idx = 0;
	return am;
}

void animation_manager_free(animation_manager_t* am){
	free(am);
}

void animation_manager_start_cutscene(animation_manager_t* am, cutscene_t* cutscene){
	am->current_cutscene = cutscene;
	am->start_time = get_ticks();
	am->next_event_idx = 0;
}

void animation_manager_stop_cutscene(animation_manager_t* am){
	am->current_cutscene = NULL;
	am->start_time = 0;
	am->next_event_idx = 0;
}

void animation_manager_update(animation_manager_t* am, void* state){
	state_t* s = (state_t*)state;
	if (!am->current_cutscene){
		return;
	}
	uint64_t now = get_ticks();

	for (; am->next_event_idx < am->current_cutscene->events_len; ++am->next_event_idx){
		cutscene_event_t* event = &am->current_cutscene->events[am->next_event_idx];

		if (TICKS_TO_MS(now - am->start_time) >= event->timestamp){
			for (size_t action_idx=0; action_idx < event->actions_len; ++action_idx){
				action_t* action = &event->actions[action_idx];
				action_run(action, s);
			}
		} else {
			break;
		}
	}
}

