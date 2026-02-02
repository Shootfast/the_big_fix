#include "cutscene.h"
#include <stdlib.h>
#include <string.h>

void cutscene_init(cutscene_t* cutscene, const char* name, size_t n_events){
	cutscene->name = strdup(name);
	cutscene->events = malloc(sizeof(cutscene_event_t) * n_events);
	cutscene->events_len = n_events;
	for (size_t i=0; i<n_events; ++i){
		cutscene_event_t* event = &cutscene->events[i];
		event->timestamp = 0;
		event->actions = NULL;
		event->actions_len = 0;
	}
}
