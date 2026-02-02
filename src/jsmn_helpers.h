#ifndef JSMN_HELPERS_H
#define JSMN_HELPERS_H

#define JSMN_HEADER
#include "jsmn.h"

/* strcmp for jsmn token */
int jsmn_cmp(const char* json, const jsmntok_t* tok, const char* str);

/* count the size of jsmn objects */
int jsmn_size(const char* js, const jsmntok_t* t, size_t remaining);

/* return index of value into tokens that has given key. -1 if not found*/
int jsmn_find_key(const char* key, const char* js, jsmntok_t* tokens, size_t remaining);

/* function object for jsmn_iteritem */
typedef void(*item_f)(const char* js, const jsmntok_t* key, const jsmntok_t* value, size_t remaining, void* user);

/* run function against all key/value pairs in json object. returns size of object traversed*/
int jsmn_iteritem(const char* js, const jsmntok_t* object, size_t remaining, item_f f, void* user);


/* function object for jsmn_itervalue */
typedef void(*value_f)(const char* js, const jsmntok_t* value, size_t remaining, void* user);

/* run function against all values pairs in json array. returns size of object traversed*/
int jsmn_itervalue(const char* js, const jsmntok_t* object, size_t remaining, value_f f, void* user);


#endif /* JSMN_HELPERS_H */
