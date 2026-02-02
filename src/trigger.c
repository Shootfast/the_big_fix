#include "trigger.h"
#include "state.h"

int trigger_fire(trigger_t* trigger, void* state){
	state_t* s = (state_t*)state;
	int ret = 0;
	for (size_t i=0; i < trigger->actions_len; ++i){
		action_t* action = &trigger->actions[i];
		ret += action_run(action, s);
	}
	return ret;
}

