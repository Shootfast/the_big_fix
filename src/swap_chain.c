#include "swap_chain.h"
#include "fifo.h"
#include <stdint.h>

typedef struct swap_chain_t_ {
	frame_buffers_t* fb;
	volatile uint8_t block_new_frame;
	
	uint8_t fb_state[FB_COUNT];
	fifo_t fb_idx_for_vi;
	volatile uint8_t cur_fb_idx;
	volatile uint32_t n_fb_free;

	draw_pass_fn draw_pass;
	void* user;
} swap_chain_t_;

static void null_draw_pass(surface_t* color, frame_buffers_t* fb, void* user){
	/* nothing */
}

static swap_chain_t g_sc = {
	.fb=NULL,
	.block_new_frame=false,
	.cur_fb_idx=0,
	.n_fb_free=0,
	.draw_pass=&null_draw_pass,
	.user=NULL
};


static void on_vi_frame_ready(){
	disable_interrupts();
	{
		FIFO_T next_fb_idx = fifo_pop(&g_sc.fb_idx_for_vi);
		if (next_fb_idx != FIFO_DEFAULT){
			vi_write_begin();
			vi_show(&g_sc.fb->color[next_fb_idx]);
			vi_write_end();

			++g_sc.fb_state[next_fb_idx];		
			g_sc.fb_state[g_sc.cur_fb_idx] = 0;
			++g_sc.n_fb_free;
			g_sc.cur_fb_idx = next_fb_idx;
		}
	}
	enable_interrupts();
}

static void render_pass_done(uint32_t fb_idx){
	disable_interrupts();
	{
		++g_sc.fb_state[fb_idx];
		fifo_push(&g_sc.fb_idx_for_vi, fb_idx);
		g_sc.block_new_frame = false;
	}
	enable_interrupts();
}

swap_chain_t* swapchain_get_instance(frame_buffers_t* fb){
	swap_chain_t* sc = &g_sc;
	if (sc->fb != NULL){
		return NULL;
	}

	sc->fb = fb;
	sc->block_new_frame = false;
	/* Block all buffers */
	for(size_t i=0; i<FB_COUNT; ++i){
		sc->fb_state[i] = 0xFF-1;
	}
	sc->fb_state[1] = 0; /* except the 2nd one */
	sc->n_fb_free = 1;
	/* Initial state pretends that the VI already has 1 frame rendering */
	sc->cur_fb_idx = FB_COUNT-1;
	fifo_fill(&sc->fb_idx_for_vi, FIFO_DEFAULT);
	fifo_push(&sc->fb_idx_for_vi, 0);
	disable_interrupts();
	{
		register_VI_handler(on_vi_frame_ready);
		set_VI_interrupt(1, VI_V_CURRENT_VBLANK);
	}
	enable_interrupts();
	rspq_wait();

	return sc;
}


void swapchain_next_frame(swap_chain_t* sc){
	/* Wait for up to 200ms to get a free FrameBuffer */
	for(uint32_t __t = TICKS_READ() + TICKS_FROM_MS(200);; __rsp_check_assert(__FILE__, __LINE__, __func__)){
		if (sc->n_fb_free && !sc->block_new_frame){
			break;
		}

		if (!TICKS_BEFORE(TICKS_READ(), __t)){
			debugf("[ERROR] RSP wait loop timed, force new buffer\n");
			sc->n_fb_free = 1;
			sc->block_new_frame = false;
		}
	}

	uint32_t idx = 0;
	while(sc->fb_state[idx]){
		++idx;
	}
	
	disable_interrupts();
	{
		sc->n_fb_free -= 1;
		sc->block_new_frame = true;
	}
	enable_interrupts();

	rdpq_attach(&sc->fb->color[idx], &sc->fb->depth);
	sc->draw_pass(&sc->fb->color[idx], sc->fb, sc->user);
	rdpq_detach_cb((void(*)(void*))render_pass_done, (void*)idx);
}

void swapchain_drain(swap_chain_t* sc){
	rspq_wait();
	RSP_WAIT_LOOP(200){
		if (sc->n_fb_free == (FB_COUNT - 1)){
			break;
		}
	}
	sc->block_new_frame = false;
}

void swapchain_set_draw_pass(swap_chain_t* sc, draw_pass_fn fn){
	sc->draw_pass = fn;
}

void swapchain_set_user_data(swap_chain_t* sc, void* user){
	sc->user = user;
}

void swapchain_start(swap_chain_t* sc){
	vi_write_begin();
	{
		vi_show(&sc->fb->color[sc->cur_fb_idx]);
	}
	vi_write_end();
	wait_ms(30);
}
