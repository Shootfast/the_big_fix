#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>
#include <t3d/t3d.h>

typedef struct aabb_t {
	T3DVec3 min;
	T3DVec3 max;
} aabb_t;

typedef struct ray_t {
	T3DVec3 origin;
	T3DVec3 inv_dir;
} ray_t;

static inline ray_t ray_create(T3DVec3* origin, T3DVec3* dir){
	return (ray_t){
		{{ origin->v[0], origin->v[1], origin->v[2] }},
		{{ dir->v[0] != 0 ? 1.0 / dir->v[0] : 0, dir->v[1] != 0 ? 1.0 / dir->v[1] : 0, dir->v[2] != 0 ? 1.0 / dir->v[2] : 0 }}
	};
}

bool point_inside_aabb(T3DVec3* point, aabb_t* aabb);

bool aabb_intersects_aabb(aabb_t* a, aabb_t* b);

bool ray_intersects_aabb(ray_t* ray, aabb_t* aabb, bool ignore_y);

#endif /* COLLISION_H */
