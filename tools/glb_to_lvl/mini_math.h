#ifndef MINI_MATH_H
#define MINI_MATH_H

typedef struct m44 {
	float data[4][4];
} m44;

typedef struct vec3 {
	float data[3];
} vec3;

typedef struct quat {
	float data[4];
} quat;


m44 m44_identity();
void m44_set_position(m44* m, const vec3* pos);
void m44_set_scale(m44* m, const vec3* scale);
void m44_set_rotation(m44* m, const quat* rot);
m44 m44_mul_m44(const m44* lhs, const m44* rhs);
vec3 m44_mul_vec3(const m44* m, const vec3* rhs);

vec3 vec3_mul_float(const vec3* v, float f);

#endif /* MINI_MATH_H */
