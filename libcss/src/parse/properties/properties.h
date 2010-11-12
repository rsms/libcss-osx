/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_parse_properties_properties_h_
#define css_parse_properties_properties_h_

#include "stylesheet.h"
#include "lex/lex.h"
#include "parse/language.h"
#include "parse/propstrings.h"

/**
 * Type of property handler function
 */
typedef css_error (*css_prop_handler)(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result);

extern const css_prop_handler property_handlers[LAST_PROP + 1 - FIRST_PROP];

css_error parse_azimuth(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_background(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_background_attachment(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_background_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_background_image(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_background_position(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_background_repeat(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_bottom(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_bottom_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_bottom_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_bottom_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_color(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_collapse(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_left(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_left_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_left_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_left_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_right(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_right_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_right_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_right_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_spacing(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_top(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_border_top_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_top_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_top_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_border_width(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_bottom(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_caption_side(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_clear(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_clip(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_content(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_counter_increment(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_counter_reset(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_cue(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_cue_after(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_cue_before(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_cursor(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_direction(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_display(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_elevation(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_empty_cells(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_float(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_font(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_font_family(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_font_size(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_font_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_font_variant(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_font_weight(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_height(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_left(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_letter_spacing(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_line_height(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_list_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_list_style_image(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_list_style_position(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_list_style_type(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_margin(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_margin_bottom(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_margin_left(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_margin_right(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_margin_top(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_max_height(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_max_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_min_height(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_min_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_orphans(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_outline(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_outline_color(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_outline_style(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_outline_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_overflow(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_padding(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_padding_bottom(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_padding_left(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_padding_right(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_padding_top(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_page_break_after(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_page_break_before(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_page_break_inside(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_pause(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result);
css_error parse_pause_after(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_pause_before(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_pitch_range(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_pitch(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_play_during(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_position(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_quotes(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_richness(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_right(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_speak_header(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_speak_numeral(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_speak_punctuation(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_speak(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_speech_rate(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_stress(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_table_layout(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_text_align(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_text_decoration(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_text_indent(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_text_transform(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_top(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_unicode_bidi(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_vertical_align(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_visibility(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_voice_family(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_volume(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_white_space(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_widows(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_width(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_word_spacing(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);
css_error parse_z_index(css_language *c,
		const parserutils_vector *vector, int *ctx, 
		css_style **result);

#endif

