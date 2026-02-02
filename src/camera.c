#include "camera.h"
#include "main.h"
#include "frame_buffers.h"

camera_t camera_create(){
	camera_t cam;
	cam.viewport = t3d_viewport_create_buffered(FB_COUNT);
	cam.viewport.size[0] = SCREEN_WIDTH;
	cam.viewport.size[1] = SCREEN_HEIGHT;

	cam.position = (T3DVec3){{0,0,0}};
	cam.target = (T3DVec3){{0,0,1}};

	cam.fov = 50;
	cam.near = 4.0f;
	cam.far = 80.0f;
	return cam;
}

void camera_destroy(camera_t* camera){
	t3d_viewport_destroy(&camera->viewport);
}

void camera_update(camera_t* camera){
	t3d_viewport_set_projection(
		&camera->viewport,
		/*fov=*/T3D_DEG_TO_RAD(camera->fov),
		camera->near,
		camera->far
	);
	T3DVec3 up = {{0,1,0}};
	t3d_viewport_look_at(&camera->viewport, &camera->position, &camera->target, &up);
}

void camera_attach(camera_t* camera){
	t3d_viewport_attach(&camera->viewport);
}
