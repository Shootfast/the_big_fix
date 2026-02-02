#ifndef TRIGGER_H
#define TRIGGER_H

#include <stddef.h>
#include "action.h"

typedef struct trigger_t {
	char* name;
	action_t* actions;
	size_t actions_len;
} trigger_t;

int trigger_fire(trigger_t* trigger, void* state);

#endif /* TRIGGER_H */
