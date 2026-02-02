#include "player.h"

player_t* player_alloc(texture_manager_t* tm){
	player_t* player = malloc(sizeof(player_t));
	if (!player){
		return NULL;
	}
	player->position = (T3DVec3){{0,0,0}};
	player->rotation = (T3DVec3){{0,0,0}};
	player->first_person_camera = camera_create();
	player->first_person_camera.near = 0.25f;
	player->amodel = anim_model_alloc("rom:/hero_nocoat.t3dm", tm);
	player->walking_speed = 0.0f;

	return player;
}

void player_free(player_t* player){
	anim_model_free(player->amodel);
	camera_destroy(&player->first_person_camera);
	free(player);
}

void player_update(player_t* player, float delta_time){
	T3DVec3 camera_pos = player->position;
	camera_pos.v[1] += 1.5; /*Hero's head pos */
	player->first_person_camera.position = camera_pos;
	T3DVec3 dir;
	dir.v[0] = -fm_sinf(player->rotation.v[1]);
	dir.v[1] = 0;
	dir.v[2] = fm_cosf(player->rotation.v[1]);
	t3d_vec3_norm(&dir);
	T3DVec3 tgt = camera_pos;
	tgt.v[0] += dir.v[0] * 0.1;
	tgt.v[1] += dir.v[1] * 0.1;
	tgt.v[2] += dir.v[2] * 0.1;
	player->first_person_camera.target = tgt;
	camera_update(&player->first_person_camera);


	model_t* model = anim_model_get_model(player->amodel);
	model_set_rotation(model, &player->rotation);
	model_set_position(model, &player->position);
	float anim_blend = player->walking_speed / 0.2f;
	if (anim_blend > 1.0f){
		anim_blend = 1.0f;
	}
	anim_model_update(player->amodel, delta_time, anim_blend);
}

void player_draw_uvs(player_t* player){
	anim_model_draw_uvs(player->amodel);
}

void player_draw_shading(player_t* player){
	anim_model_draw_shading(player->amodel);
}
