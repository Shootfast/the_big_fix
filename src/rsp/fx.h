#ifndef RSP_FX_H
#define RSP_FX_H

#include <stdint.h>

void rsp_fx_init();

void rsp_fx_fill_textures(uint32_t fb_tex, uint32_t fb_tex_end, uint32_t fb_out);

#endif /* RSP_FX_H */
