#ifndef CAMERA_H
#define CAMERA_H

#include <t3d/t3d.h>

typedef struct camera_t {
	T3DViewport viewport;
	T3DVec3 position;
	T3DVec3 target;
	float fov;
	float near;
	float far;
} camera_t;

camera_t camera_create();
void camera_destroy(camera_t* camera);

void camera_update(camera_t* camera);
void camera_attach(camera_t* camera);

#endif /* CAMERA_H */
