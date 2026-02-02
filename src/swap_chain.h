#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#include "frame_buffers.h"

typedef struct swap_chain_t_ swap_chain_t;

typedef void (*draw_pass_fn)(surface_t* color, frame_buffers_t* fb, void* user);

swap_chain_t* swapchain_get_instance(frame_buffers_t* fb);
void swapchain_next_frame(swap_chain_t* sc);
void swapchain_set_draw_pass(swap_chain_t* sc, draw_pass_fn fn);
void swapchain_set_user_data(swap_chain_t* sc, void* user);
void swapchain_start(swap_chain_t* sc);
void swapchain_drain(swap_chain_t* sc);

#endif /* SWAP_CHAIN_H */
