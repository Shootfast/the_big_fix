#include "mini_math.h"
#include <stddef.h>

m44 m44_identity(){
	return (m44){{
		{1, 0, 0, 0},
		{0, 1, 0, 0},
		{0, 0, 1, 0},
		{0, 0, 0, 1}
	}};
}

void m44_set_position(m44* m, const vec3* pos){
	m->data[3][0] = pos->data[0];
	m->data[3][1] = pos->data[1];
	m->data[3][2] = pos->data[2];
}

void m44_set_scale(m44* m, const vec3* scale){
	m->data[0][0] = scale->data[0];
	m->data[1][1] = scale->data[1];
	m->data[2][2] = scale->data[2];
}

void m44_set_rotation(m44* m, const quat* rot){
	float qxx = rot->data[0] * rot->data[0];
	float qyy = rot->data[1] * rot->data[1];
	float qzz = rot->data[2] * rot->data[2];
	float qxz = rot->data[0] * rot->data[2];
	float qxy = rot->data[0] * rot->data[1];
	float qyz = rot->data[1] * rot->data[2];
	float qwx = rot->data[3] * rot->data[0];
	float qwy = rot->data[3] * rot->data[1];
	float qwz = rot->data[3] * rot->data[2];

	m->data[0][0] = 1.0f - 2.0f * (qyy + qzz);
	m->data[0][1] =        2.0f * (qxy + qwz);
	m->data[0][2] =        2.0f * (qxz - qwy);

	m->data[1][0] =        2.0f * (qxy - qwz);
	m->data[1][1] = 1.0f - 2.0f * (qxx + qzz);
	m->data[1][2] =        2.0f * (qyz + qwx);

	m->data[2][0] =        2.0f * (qxz + qwy);
	m->data[2][1] =        2.0f * (qyz - qwx);
	m->data[2][2] = 1.0f - 2.0f * (qxx + qyy);
}

m44 m44_mul_m44(const m44* lhs, const m44* rhs){
	m44 res;
	for (size_t i=0; i<4; ++i){
		for (size_t j=0; j<4; ++j){
			res.data[j][i] = lhs->data[0][i] * rhs->data[j][0] +
			                 lhs->data[1][i] * rhs->data[j][1] +
			                 lhs->data[2][i] * rhs->data[j][2] +
			                 lhs->data[3][i] * rhs->data[j][3];
		}
	}
	return res;
}

vec3 m44_mul_vec3(const m44* m, const vec3* rhs){
	vec3 res;
	for (size_t i=0; i<3; ++i){
		res.data[i] = m->data[0][i] * rhs->data[0] +
		              m->data[1][i] * rhs->data[1] +
		              m->data[2][i] * rhs->data[2] +
		              m->data[3][i] * 1.0f;
	}
	return res;
}

vec3 vec3_mul_float(const vec3* v, float f){
	return (vec3){{
		v->data[0] * f, v->data[1] * f, v->data[2] * f
	}};
}
