#ifndef FRAME_BUFFERS_H
#define FRAME_BUFFERS_H

#include <libdragon.h>
#define FB_COUNT 3

typedef struct frame_buffers_t {
	surface_t color[FB_COUNT];
	surface_t uv[FB_COUNT];
	surface_t shade[FB_COUNT];
	surface_t depth;
} frame_buffers_t;


frame_buffers_t alloc_frame_buffers();

#endif /* FRAME_BUFFERS_H */
