#include "scene_manager.h"

#include <stdlib.h>
#include <string.h>

scene_manager_t* scene_manager_alloc(){
	scene_manager_t* sm = malloc(sizeof(scene_manager_t));
	sm->next_scene = NULL;
	sm->swap = false;
	return sm;
}
void scene_manager_free(scene_manager_t* sm){
	free(sm->next_scene);
	free(sm);
}

void scene_manager_set_next_scene(scene_manager_t* sm, const char* next_scene){
	if (sm->next_scene){
		free(sm->next_scene);
	}
	sm->next_scene = strdup(next_scene);
}

