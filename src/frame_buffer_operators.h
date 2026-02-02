#ifndef FRAME_BUFFER_OPERATORS_H
#define FRAME_BUFFER_OPERATORS_H

#include <stddef.h>

#define FBOP_BLEND_SLICES 16
#define RDPQ_SHADE_BUFFER_IDX 3
#define RDPQ_COLOR_BUFFER_IDX 4
#define FBOP_SWIZZLE_SIZE 4

typedef struct fbop_blend_t_ fbop_blend_t;
typedef struct fbop_uvgen_t_ fbop_uvgen_t;

fbop_blend_t* fbop_blend_alloc();
void fbop_blend_free(fbop_blend_t* op);
void fbop_blend(fbop_blend_t* op, size_t idx);

fbop_uvgen_t* fbop_uvgen_alloc();
void fbop_uvgen_free(fbop_uvgen_t* op);
void fbop_uvgen(fbop_uvgen_t* op);

#endif /* FRAME_BUFFER_OPERATORS_H */
