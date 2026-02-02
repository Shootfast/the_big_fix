#ifndef UI_H
#define UI_H

#include <stddef.h>
#include <stdbool.h>

typedef struct ui_t_ ui_t;

typedef enum ui_text_style_t {
	UI_TEXT_IMMEDIATE,
	UI_TEXT_LETTER_BY_LETTER,
	UI_TEXT_WORD_BY_WORD,
	UI_TEXT_MAX
} ui_text_style_t;

ui_t* ui_alloc();
void ui_free(ui_t* ui);

void ui_draw(ui_t* ui);

void ui_write_message(ui_t* ui, const char* text, ui_text_style_t style, size_t onscreen_duration_ms);
void ui_cinematic_mode(ui_t* ui, bool enabled);

#endif /* UI_H */
