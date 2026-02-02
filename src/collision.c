#include "collision.h"

bool point_inside_aabb(T3DVec3* point, aabb_t* aabb){
	return point->v[0] >= aabb->min.v[0] &&
	       point->v[0] <= aabb->max.v[0] &&
	       point->v[1] >= aabb->min.v[1] &&
	       point->v[1] <= aabb->max.v[1] &&
	       point->v[2] >= aabb->min.v[2] &&
	       point->v[2] <= aabb->max.v[2];
}

bool aabb_intersects_aabb(aabb_t* a, aabb_t* b){
	return a->min.v[0] <= b->max.v[0] &&
	       a->max.v[0] >= b->min.v[0] &&
	       a->min.v[1] <= b->max.v[1] &&
	       a->max.v[1] >= b->min.v[1] &&
	       a->min.v[2] <= b->max.v[2] &&
	       a->max.v[2] >= b->min.v[2];
}

bool ray_intersects_aabb(ray_t* ray, aabb_t* aabb, bool ignore_y){
	float t1  = (aabb->min.v[0] - ray->origin.v[0]) * ray->inv_dir.v[0];
	float t2  = (aabb->max.v[0] - ray->origin.v[0]) * ray->inv_dir.v[0];

	float tmin = fminf(t1, t2);
	float tmax = fmaxf(t1, t2);

	for (size_t i= ignore_y ? 2 : 1; i < 3; ++i){
		t1 = (aabb->min.v[i] - ray->origin.v[i]) * ray->inv_dir.v[i];
		t2 = (aabb->max.v[i] - ray->origin.v[i]) * ray->inv_dir.v[i];

		tmin = fmaxf(tmin, fminf(t1, t2));
		tmax = fminf(tmax, fmaxf(t1, t2));
	}

	return tmax >= fmaxf(tmin, 0.0);
}
