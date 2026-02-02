#include "audio_manager.h"

#define MAX_AUDIO_CLIPS 5
#define MAX_FILEPATH 256

typedef struct audio_manager_t_ {
	audio_clip_t clips[MAX_AUDIO_CLIPS];
	size_t clips_len;
} audio_manager_t_;


audio_manager_t* audio_manager_alloc(){
	audio_manager_t* aum = malloc(sizeof(audio_manager_t));
	aum->clips_len = 0;
	return aum;
}

void audio_manager_free(audio_manager_t* aum){
	for (size_t i=0; i< aum->clips_len; ++i){
		audio_clip_t* clip = &aum->clips[i];
		free(clip->name);
		wav64_close(&clip->clip);
	}
	free(aum);
}

audio_clip_t* audio_manager_get_clip(audio_manager_t* aum, const char* name){
	for (size_t i=0; i < aum->clips_len; ++i){
		audio_clip_t* clip = &aum->clips[i];
		if (strcmp(clip->name, name) == 0){
			return clip;
		}
	}

	/* No clip found*/
	if (aum->clips_len >= MAX_AUDIO_CLIPS){
		return NULL;
	}

	char file_path[MAX_FILEPATH];
	int s = snprintf(&file_path[0], MAX_FILEPATH, "rom:/%s.wav64", name);

	audio_clip_t* clip = &aum->clips[aum->clips_len];
	clip->name = strdup(name);
	wav64_open(&clip->clip, file_path);
	return clip;
}


