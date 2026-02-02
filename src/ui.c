#include "ui.h"
#include "main.h"
#include <libdragon.h>
#include <t3d/t3d.h>

#define FONT_MAIN 2
#define MAX_MESSAGES 10

typedef struct ui_msg_t {
	uint64_t start_time;
	size_t duration;
	char* text;
	ui_text_style_t style;
} ui_msg_t;

typedef struct ui_t_ {
	rdpq_font_t* font;
	size_t font_height;
	ui_msg_t messages[MAX_MESSAGES];
	size_t messages_len;

	rspq_block_t* draw_black_bars;
	T3DVertPacked* black_bars;
	bool cinematic_mode;
} ui_t_;

static int compare_ui_msg_start_time(const void* lhs, const void* rhs){
	const ui_msg_t* a = (const ui_msg_t*) lhs;
	const ui_msg_t* b = (const ui_msg_t*) rhs;
	if (a->start_time > b->start_time){
		return -1;
	}
	return 1;
}

static void sort_and_expire_msgs(ui_t* ui){
	uint64_t time = get_ticks();
	/* Expire old messages */
	for (size_t i=0; i < ui->messages_len; ++i){
		ui_msg_t* msg = &ui->messages[i];
		if (msg->start_time + TICKS_FROM_MS(msg->duration) < time){
			free(msg->text);
			*msg = (ui_msg_t){
				.start_time=0,
				.duration=0,
				.text=NULL,
				.style=UI_TEXT_LETTER_BY_LETTER
			};
		}
	}
	/* order messages so newest is first */
	qsort(&ui->messages[0], ui->messages_len, sizeof(ui_msg_t), compare_ui_msg_start_time);

	/* remove expired messages */
	for (size_t i=0; i < ui->messages_len; ++i){
		ui_msg_t* msg = &ui->messages[i];
		if ((msg->start_time == msg->duration) && (msg->text == NULL)){
			ui->messages_len = i;
			break;
		}
	}
}

static T3DVertPacked* black_bars_alloc(){
	T3DVertPacked* black_bars = malloc_uncached(sizeof(T3DVertPacked) * 4);
	uint16_t norm = t3d_vert_pack_normal(&(T3DVec3){{0,0,-1}});

	int w = SCREEN_WIDTH/2;
	int h = SCREEN_HEIGHT/2;
	black_bars[0] = (T3DVertPacked){
		.posA = {-w,-h,0}, .rgbaA=0x000000FF, .normA=norm,
		.posB = {w,-h}, .rgbaB=0x000000FF, .normB=norm,
	};
	black_bars[1] = (T3DVertPacked){
		.posA = {w,-h+30,0}, .rgbaA=0x000000FF, .normA=norm,
		.posB = {-w,-h+30,0}, .rgbaB=0x000000FF, .normB=norm,
	};
	black_bars[2] = (T3DVertPacked){
		.posA = {-w,h-30,0}, .rgbaA=0x000000FF, .normA=norm,
		.posB = {w,h-30}, .rgbaB=0x000000FF, .normB=norm,
	};
	black_bars[3] = (T3DVertPacked){
		.posA = {w,h,0}, .rgbaA=0x000000FF, .normA=norm,
		.posB = {-w,h,0}, .rgbaB=0x000000FF, .normB=norm,
	};
	return black_bars;
}

static rspq_block_t* draw_black_bars_calls(ui_t* ui){
	rspq_block_begin();
	
	t3d_vert_load(ui->black_bars, 0, 8);
	t3d_tri_draw(0,1,2);
	t3d_tri_draw(2,3,0);
	t3d_tri_draw(4,5,6);
	t3d_tri_draw(6,7,4);
	t3d_tri_sync();
	return rspq_block_end();
}

void find_font_glyph_sizes(rdpq_font_t* fnt, size_t* width, size_t* height){
	int idx=0;
	uint32_t start, end;
	bool sparse;
	while(rdpq_font_get_glyph_ranges(fnt, idx++, &start, &end, &sparse)){
		rdpq_font_gmetrics_t gmetrics;
		do {
			rdpq_font_get_glyph_metrics(fnt, start, &gmetrics);
			size_t w = gmetrics.x1 - gmetrics.x0;
			size_t h = gmetrics.y1 - gmetrics.y0;
			*width = w > *width ? w : *width;
			*height = h > *height ? h : *height;
		} while (gmetrics.x1-gmetrics.x0 == 0 && ++start <= end);
		if (start > end){
			continue;
		}
	}
}

ui_t* ui_alloc(){
	ui_t* ui = malloc(sizeof(ui_t));
	ui->font = rdpq_font_load("rom://Limelight-Regular.font64");
	rdpq_font_style(ui->font, 0, &(rdpq_fontstyle_t){
		.color = (color_t){0xFF, 0xFF, 0xFF, 0xFF}
	});
	rdpq_text_register_font(FONT_MAIN, ui->font);

	/* this doesn't seem to work when ran twice, so pre-setting
	 * font_height to 8!*/
	ui->font_height = 8;
	size_t _;
	find_font_glyph_sizes(ui->font, &_, &ui->font_height);

	ui->messages_len = 0;
	ui->cinematic_mode = false;
	ui->black_bars = black_bars_alloc();
	ui->draw_black_bars = draw_black_bars_calls(ui);

	return ui;
}

void ui_free(ui_t* ui){
	rdpq_text_unregister_font(FONT_MAIN);
	rdpq_font_free(ui->font);
	rspq_block_free(ui->draw_black_bars);
	free_uncached(ui->black_bars);
	free(ui);
}

void ui_draw(ui_t* ui){

	if (ui->cinematic_mode){
		T3DViewport* vp = t3d_viewport_get();
		t3d_viewport_set_ortho(vp,
			-SCREEN_WIDTH/2, SCREEN_WIDTH/2,
			-SCREEN_HEIGHT/2, SCREEN_HEIGHT/2,
			4.0, 80);
		t3d_viewport_look_at(vp, &(T3DVec3){{0,0,-10}}, &(T3DVec3){{0,0,0}}, &(T3DVec3){{0,1,0}});
		t3d_viewport_attach(vp);
		
		rdpq_mode_combiner(RDPQ_COMBINER_SHADE);
		t3d_state_set_drawflags(T3D_FLAG_SHADED | T3D_FLAG_DEPTH);
		rspq_block_run(ui->draw_black_bars);
	}



	/* draw messages */
	float x = 5;
	float y = SCREEN_HEIGHT - ui->font_height;

	sort_and_expire_msgs(ui);
	for (size_t i=0; i < ui->messages_len; ++i){
		ui_msg_t* msg = &ui->messages[i];
		
		/*TODO style */
		rdpq_textmetrics_t metrics = rdpq_text_printf(
			&(rdpq_textparms_t){.align=ALIGN_LEFT, .width=SCREEN_WIDTH, .wrap=WRAP_WORD},
			FONT_MAIN,
			x, y,
			msg->text
		);
		/* move the pen up by the number of lines written */
		y -= (ui->font_height +1) * metrics.nlines;
	}
}

void ui_write_message(ui_t* ui, const char* text, ui_text_style_t style, size_t onscreen_duration_ms){
	if (ui->messages_len < MAX_MESSAGES){
		ui_msg_t* msg = &ui->messages[ui->messages_len++];
		msg->start_time = get_ticks();
		msg->duration = onscreen_duration_ms;
		msg->text = strdup(text);
		msg->style = style;
	}
}

void ui_cinematic_mode(ui_t* ui, bool enabled){
	ui->cinematic_mode = enabled;
}
