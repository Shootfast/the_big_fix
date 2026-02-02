#include "frame_buffers.h"
#include "main.h"

// use internal function from
// libdragon/src/system_internal.h
extern void* sbrk_top(int incr);

#define FB_BYTE_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT*sizeof(uint16_t))
static uint32_t FB_BANK_ADDR[6] = {
    0x80500000 - FB_BYTE_SIZE, 0x80500000,
    0x80600000 - FB_BYTE_SIZE, 0x80600000,
    0x80700000 - FB_BYTE_SIZE, 0x80700000,
};

frame_buffers_t alloc_frame_buffers(){
	if (!is_memory_expanded()){
		assertf(false, "Expansion-Pack required!");
	}

	/* Reserve the upper 4MB (excl. the stack) for our frame buffers */
	void* buf = sbrk_top(16);
	uint32_t missing = (uint32_t)buf - FB_BANK_ADDR[0];
	buf = sbrk_top(missing);
	assert(FB_BANK_ADDR[0] == (uint32_t)buf);

	uint16_t stride = SCREEN_WIDTH * sizeof(uint16_t);

	return (frame_buffers_t){
		.color = {
			surface_make(UncachedAddr(FB_BANK_ADDR[2]), FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT, stride),
			surface_make(UncachedAddr(FB_BANK_ADDR[3]), FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT, stride),
			surface_make(UncachedAddr(FB_BANK_ADDR[4]), FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT, stride),
		},
		.uv = {
			surface_alloc(FMT_RGBA32, SCREEN_WIDTH, SCREEN_HEIGHT),
			surface_alloc(FMT_RGBA32, SCREEN_WIDTH, SCREEN_HEIGHT),
			surface_alloc(FMT_RGBA32, SCREEN_WIDTH, SCREEN_HEIGHT),
		},
		.shade = {
			surface_alloc(FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT),
			surface_alloc(FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT),
			surface_alloc(FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT),
		},
		.depth = surface_make(UncachedAddr(FB_BANK_ADDR[5]), FMT_RGBA16, SCREEN_WIDTH, SCREEN_HEIGHT, stride)
	};
}
