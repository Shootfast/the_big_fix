#include "jsmn.h" /* put this first to get implementation*/
#include "jsmn_helpers.h"
#include <string.h>

int jsmn_cmp(const char* json, const jsmntok_t* tok, const char* str){
	if ((int)strlen(str) == tok->end - tok->start &&
		strncmp(json+tok->start, str, tok->end - tok->start) == 0){
		return 0;
	}
	return -1;
}

int jsmn_size(const char* js, const jsmntok_t* t, size_t remaining){
	if (remaining == 0){
		return 0;
	}
	int size = 0;
	if (t->type == JSMN_PRIMITIVE || t->type == JSMN_STRING){
		return size + 1;
	} else if (t->type == JSMN_OBJECT){
		for(size_t i=0; i < t->size; ++i){
			const jsmntok_t* key = t + 1 + size;
			size += jsmn_size(js, key, remaining - size);
			if (key->size > 0){
				size += jsmn_size(js, t + 1 + size, remaining - size);
			}
		}
		return size +1;
	} else if (t->type == JSMN_ARRAY){
		for (size_t i=0; i < t->size; ++i){
			size += jsmn_size(js, t + 1 + size, remaining - size);
		}
		return size + 1;
	}
	return size;
}

int jsmn_find_key(const char* key, const char* js, jsmntok_t* tokens, size_t remaining){
	if (remaining == 0){
		return -1;
	}
	int idx = 0;
	while (idx < remaining){
		jsmntok_t* t = &tokens[idx];
		size_t left = remaining - idx;
		if (t->type != JSMN_OBJECT){
			idx += jsmn_size(js, t, left);
			continue;
		}
		size_t size = 0;
		for(size_t i=0; i < t->size; ++i){
			jsmntok_t* k = t + 1 + size;
			if (jsmn_cmp(js, k, key) == 0){
				return idx + size + 2;
			}
			size += jsmn_size(js, k, left - size);
			if (k->size > 0){
				size += jsmn_size(js, t+1+size, left - size);
			}
		}
		idx += size+1;
	}
	return -1;
}

int jsmn_iteritem(const char* js, const jsmntok_t* object, size_t remaining, item_f f, void* user){
	if (object->type != JSMN_OBJECT){
		return 0;
	}
	size_t size = 0;
	for(size_t i=0; i < object->size; ++i){
		const jsmntok_t* key = object + 1 + size;
		const jsmntok_t* value = NULL;
		size += jsmn_size(js, key, remaining - size);
		if (key->size > 0){
			value = object + 1 + size;
			size += jsmn_size(js, value, remaining - size);
		}
		f(js, key, value, remaining-size, user); 
	}
	return size + 1;
}

int jsmn_itervalue(const char* js, const jsmntok_t* object, size_t remaining, value_f f, void* user){
	if (object->type != JSMN_ARRAY){
		return 0;
	}
	size_t size = 0;
	for(size_t i=0; i < object->size; ++i){
		const jsmntok_t* value = object + 1 + size;
		size += jsmn_size(js, value, remaining - size);
		f(js, value, remaining-size, user); 
	}
	return size + 1;
}
