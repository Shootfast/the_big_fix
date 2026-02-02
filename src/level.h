#ifndef LEVEL_H
#define LEVEL_H

#include <t3d/t3d.h>
#include "texture_manager.h"
#include "camera.h"
#include "collision.h"

typedef struct level_t_ level_t;

level_t* level_alloc(const char* name, texture_manager_t* tm);
void level_free(level_t* level);

void level_draw_uvs(level_t* level);
void level_draw_shading(level_t* level,
                        uint8_t (*ambient_light_color)[4],
                        uint8_t (*selected_light_color)[4]);

camera_t* level_get_active_camera(level_t* level);
void level_set_lights(level_t* level);
bool level_check_bounds(level_t* level, T3DVec3* pos);
void level_objects_check_interactive(level_t* level, ray_t* ray);


#endif /* LEVEL_H */
