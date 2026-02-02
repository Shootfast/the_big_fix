#ifndef SCENE_H
#define SCENE_H

#include "ui.h"
#include "swap_chain.h"
#include "scene_manager.h"

typedef struct scene_t_ scene_t;

scene_t* scene_alloc(scene_manager_t* sm, const char* level_name);
void scene_free(scene_t* scene);
void scene_update(scene_t* scene, float delta_time);
void scene_use_swapchain(scene_t* scene, swap_chain_t* sc);

#endif /* SCENE_H */
