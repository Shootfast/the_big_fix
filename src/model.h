#ifndef MODEL_H
#define MODEL_H

#include "texture_manager.h"
#include "collision.h"
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

typedef struct model_t_ model_t;

model_t* model_alloc(const char* path, texture_manager_t* tm);
void model_free(model_t* model);

T3DVec3 model_get_position(model_t* model);
void model_set_position(model_t* model, const T3DVec3* pos);
void model_set_rotation(model_t* model, const T3DVec3* rot);
void model_draw_uvs(model_t* model);
void model_draw_shading(model_t* model);

T3DModel* model_get_t3d_model(model_t* model);
T3DObject** model_get_t3d_objects(model_t* model, size_t* objects_len);
T3DMat4FP* model_get_t3d_matrix(model_t* model);
aabb_t model_get_world_aabb(model_t* model);

#endif /* MODEL_H */
