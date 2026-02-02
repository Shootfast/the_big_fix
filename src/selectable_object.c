#include "selectable_object.h"

void selectable_object_init(selectable_object_t* sobj,
	                        const char* name,
	                        const char* model_path,
	                        T3DVec3* position,
	                        texture_manager_t* tm){
	sobj->name = name;
	sobj->model = model_alloc(model_path, tm);
	model_set_position(sobj->model, position);
	sobj->selected = false;
	sobj->is_decal = false;
	sobj->triggers = NULL;
	sobj->triggers_len = 0;
}

void selectable_object_deinit(selectable_object_t* sobj){
	model_free(sobj->model);
	sobj->model = NULL;
	if (sobj->triggers_len != 0){
		for(size_t i=0; i < sobj->triggers_len; ++i){
			trigger_t* trigger = &sobj->triggers[i];
			free(trigger->name);
			for(size_t j=0; j<trigger->actions_len; ++j){
				action_t* action = &trigger->actions[j];
				for (size_t k=0; k<action->argc; ++k){
					char* arg = action->argv[i];
					free(arg);
				}
				free(action->argv);
			}
			free(trigger->actions);
		}
		free(sobj->triggers);
	}
	sobj->triggers=NULL;
	sobj->triggers_len = 0;
}
