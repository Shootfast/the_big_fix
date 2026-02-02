#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <stdbool.h>

typedef struct scene_manager_t {
	char* next_scene;
	bool swap;
} scene_manager_t;

scene_manager_t* scene_manager_alloc();
void scene_manager_free(scene_manager_t* sm);

void scene_manager_set_next_scene(scene_manager_t* sm, const char* next_scene);

#endif /* SCENE_MANAGER_H */
