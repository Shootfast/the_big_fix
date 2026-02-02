#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/tpx.h>
#include <stddef.h>

#include "main.h"
#include "ui.h"
#include "frame_buffers.h"
#include "rsp/fx.h"
#include "swap_chain.h"
#include "scene.h"
#include "scene_manager.h"

int main(){

	//debug_init_isviewer();
	//debug_init_usblog();
	
	/* Asset loader */
	asset_init_compression(2);

	/* Dragon FileSystem for access to ROM*/
	dfs_init(DFS_DEFAULT_LOCATION);

	/* Reality Display Processor Queue */
	rdpq_init();
	//rdpq_debug_start();

	/* Controllers */
	joypad_init();

	/* Audio */
	audio_init(/*freq=*/48000, /*n_buffers=*/4);
	mixer_init(/*n_channels=*/16);

	/* Timer */
	timer_init();
	uint64_t last_ticks = get_ticks() - 10000;

	/* Tiny3d */
	t3d_init((T3DInitParams){});

	/*TinyPX*/
	tpx_init((TPXInitParams){});

	/* RSP FX */
	rsp_fx_init();

	/* FrameBuffers */
	frame_buffers_t fb = alloc_frame_buffers();

	/* Video Interface */
	vi_init();
	vi_set_dedither(false);
	vi_set_aa_mode(VI_AA_MODE_RESAMPLE);
	vi_set_interlaced(false);
	vi_set_divot(false);
	vi_set_gamma(VI_GAMMA_DISABLE);

	/* SwapChain */
	swap_chain_t* sc = swapchain_get_instance(&fb);

	/* Scene Manager */
	scene_manager_t* sm = scene_manager_alloc();
	scene_manager_set_next_scene(sm, "city");

	/* Main loop */
	while(1){

		scene_t* scene = scene_alloc(sm, sm->next_scene);
		sm->swap = false;
		scene_use_swapchain(scene, sc);

		swapchain_start(sc);

		while(!sm->swap/*rendering this scene*/){
			uint64_t ticks = get_ticks();
			float time_delta = TICKS_TO_US(ticks - last_ticks) / 1000000.0f;
			last_ticks = ticks;

			joypad_poll();
			scene_update(scene, time_delta);

			swapchain_next_frame(sc);
			mixer_try_play();
		}

		scene_free(scene);
		swapchain_drain(sc);
	}
}
