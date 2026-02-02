#ifndef TEX_H
#define TEX_H

#include <stdint.h>

/* see ./tex.S */
uint32_t apply_texture(uint32_t fb_tex_in, uint32_t fb_tex_in_end, uint32_t fb_out_64);

#endif /* TEX_H */
