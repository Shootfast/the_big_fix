#include "fx.h"
#include <libdragon.h>
#include <stdarg.h>

DEFINE_RSP_UCODE(rsp_fx);

#define unlikely(x) __builtin_expect(x, 0)

extern volatile uint32_t* rspq_cur_pointer;
extern volatile uint32_t* rspq_cur_sentinel;
extern void rspq_next_buffer(void);

static uint32_t g_rsp_id_fx = 0;

void rsp_fx_init(){
	if (g_rsp_id_fx == 0){
		g_rsp_id_fx = rspq_overlay_register(&rsp_fx);
	}
}

static void write(uint32_t overlay_id, uint32_t command_id, uint32_t arg0, size_t count, ...){
	va_list args;
	va_start(args, count);

	volatile uint32_t* ptr = rspq_cur_pointer +1;
	for(size_t i=0; i<count; ++i){
		*ptr = va_arg(args, uint32_t);
		++ptr;
	}
	va_end(args);

	*rspq_cur_pointer = arg0 | overlay_id | ((command_id) << 24);
	rspq_cur_pointer += count +1;

	if (unlikely(rspq_cur_pointer > rspq_cur_sentinel)){
		rspq_next_buffer();
	}
}

void rsp_fx_fill_textures(uint32_t fb_tex, uint32_t fb_tex_end, uint32_t fb_out){
	write(g_rsp_id_fx, 0,
	    (uint32_t)fb_tex     & 0xFFFFFF,
		2,
	    (uint32_t)fb_tex_end & 0xFFFFFF,
	    (uint32_t)fb_out     & 0xFFFFFF
	);
}
