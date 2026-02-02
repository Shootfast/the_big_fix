#ifndef ACTION_H
#define ACTION_H

#include "vargs.h"
#include <stddef.h>

typedef struct action_t {
	int argc;
	char** argv;
} action_t;

int action_run(action_t* action, void* state);

int action_create_and_run_argc(void* state, size_t argc, ...);
#define action_create_and_run(state, ...) action_create_and_run_argc(state, PP_NARG(__VA_ARGS__), __VA_ARGS__)

#endif /* ACTION_H */
