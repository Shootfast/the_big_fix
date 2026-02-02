#ifndef CUTSCENE_H
#define CUTSCENE_H

#include "action.h"

typedef struct cutscene_event_t {
	size_t timestamp;
	action_t* actions;
	size_t actions_len;
} cutscene_event_t;

typedef struct cutscene_t {
	char* name;
	cutscene_event_t* events;
	size_t events_len;
} cutscene_t;

void cutscene_init(cutscene_t* cutscene, const char* name, size_t n_events);


#endif /* CUTSCENE_H */
