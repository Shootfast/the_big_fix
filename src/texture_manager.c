#include "texture_manager.h"

#define TEX_WIDTH 256
#define TEX_HEIGHT 256
#define TEX_SIZE_BYTES TEX_WIDTH * TEX_HEIGHT

typedef struct item_t{
	const char* key;
	uint8_t value;
} item_t;

typedef struct texture_manager_t_ {
	uint8_t* buffer;
	size_t max_textures;
	size_t count;
	item_t* items;
} texture_manager_t_;

static item_t* search(item_t* items, size_t size, const char* key){
	for (size_t i=0; i < size; ++i){
		if (!items[i].key){
			continue; /* reserved texture but not set */
		}
		if (strcmp(items[i].key, key) == 0){
			return &items[i];
		}
	}
	return NULL;
}

static void load(const char* path, uint8_t* tex){
	FILE* fh = asset_fopen(path, NULL);
	fread(tex, 1, TEX_SIZE_BYTES, fh);
	fclose(fh);
}

texture_manager_t* texture_manager_alloc(size_t max_textures){
	texture_manager_t* tm = malloc(sizeof(texture_manager_t_));
	if (tm){
		tm->buffer = (uint8_t*)TEX_BASE_ADDR;
		tm->max_textures = max_textures;
		tm->count = 0;
		tm->items = malloc(sizeof(item_t) * max_textures);
		if (!tm->items){
			free(tm);
			tm = NULL;
		}
	}
	return tm;
}

void texture_manager_free(texture_manager_t* tm){
	free(tm->items);
	free(tm);
}

uint8_t texture_manager_add_texture(texture_manager_t* tm, const char* path){
	item_t* found = search(tm->items, tm->count, path);
	if (!found){
		uint8_t idx = texture_manager_reserve_texture(tm);
		return texture_manager_set_texture(tm, idx, path);
	}
	return found->value;
}

uint8_t texture_manager_reserve_texture(texture_manager_t* tm){
	uint8_t idx = tm->count++;
	assertf(idx < tm->max_textures, "Texture buffer full: %d/%zu", idx, tm->max_textures);
	tm->items[idx].key = NULL;
	tm->items[idx].value = idx;
	return idx;
}

uint8_t texture_manager_set_texture(texture_manager_t* tm, uint8_t idx, const char* path){
	uint8_t* tex = tm->buffer + TEX_SIZE_BYTES * idx;
	debugf("Reserving texture %d (%s) at address: %p\n", idx, path, tex);
	load(path, tex);
	tm->items[idx].key = strdup(path);
	tm->items[idx].value = idx;
	return idx;
}
