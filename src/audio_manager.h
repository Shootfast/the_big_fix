#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H
#include <libdragon.h>
#include <stdlib.h>

typedef struct audio_clip_t {
	char* name;
	wav64_t clip;
} audio_clip_t;

typedef struct audio_manager_t_ audio_manager_t;

audio_manager_t* audio_manager_alloc();
void audio_manager_free(audio_manager_t* aum);

audio_clip_t* audio_manager_get_clip(audio_manager_t* aum, const char* name);

#endif /* AUDIO_MANAGER_H */
