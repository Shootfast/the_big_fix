#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <libdragon.h>

#define TEX_BASE_ADDR 0x80400000
#define MAX_TEXTURES 18

typedef struct texture_manager_t_ texture_manager_t;

texture_manager_t* texture_manager_alloc(size_t max_textures);
void texture_manager_free(texture_manager_t* tm);

uint8_t texture_manager_add_texture(texture_manager_t* tm, const char* path);
uint8_t texture_manager_reserve_texture(texture_manager_t* tm);
uint8_t texture_manager_set_texture(texture_manager_t* tm, uint8_t idx, const char* path);

#endif /* TEXTURE_MANAGER_H */
