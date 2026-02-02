#ifndef SELECTABLE_OBJECT_H
#define SELECTABLE_OBJECT_H

#include <t3d/t3d.h>
#include "model.h"
#include "trigger.h"

typedef struct selectable_object_t{
	const char* name;
	model_t* model;
	bool selected;
	bool is_decal;

	trigger_t* triggers;
	size_t triggers_len;

} selectable_object_t;

void selectable_object_init(selectable_object_t* sobj,
                            const char* name,
                            const char* model_path,
                            T3DVec3* position,
                            texture_manager_t* tm);

void selectable_object_deinit(selectable_object_t* sobj);

#endif /* SELECTABLE_OBJECT_H */
