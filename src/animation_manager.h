#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "cutscene.h"

typedef struct animation_manager_t_ animation_manager_t;

animation_manager_t* animation_manager_alloc();
void animation_manager_free(animation_manager_t* am);

void animation_manager_start_cutscene(animation_manager_t* am, cutscene_t* cutscene);
void animation_manager_stop_cutscene(animation_manager_t* am);

void animation_manager_update(animation_manager_t* am, void* state);


#endif /* ANIMATION_MANAGER_H */
