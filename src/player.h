#ifndef PLAYER_H
#define PLAYER_H

#include <t3d/t3d.h>
#include "texture_manager.h"
#include "camera.h"
#include "animated_model.h"

typedef struct player_t {
	T3DVec3 position;
	T3DVec3 rotation;
	camera_t first_person_camera;
	anim_model_t* amodel;
	float walking_speed;
} player_t;

player_t* player_alloc(texture_manager_t* tm);
void player_free(player_t* player);
void player_update(player_t* player, float delta_time);

void player_draw_uvs(player_t* player);
void player_draw_shading(player_t* player);

#endif /* PLAYER_H */
