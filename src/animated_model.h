#ifndef ANIMATED_MODEL_H
#define ANIMATED_MODEL_H

#include "model.h"
#include "texture_manager.h"

typedef struct anim_model_t_ anim_model_t;

anim_model_t* anim_model_alloc(const char* path, texture_manager_t* tm);
void anim_model_free(anim_model_t* amodel);

model_t* anim_model_get_model(anim_model_t* amodel);

void anim_model_update(anim_model_t* amodel, float delta_time, float anim_blend);
void anim_model_draw_uvs(anim_model_t* amodel);
void anim_model_draw_shading(anim_model_t* amodel);

#endif /* ANIMATED_MODEL_H */
