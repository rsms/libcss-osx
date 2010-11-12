/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <string.h>

#include "select/computed.h"
#include "select/dispatch.h"
#include "select/propget.h"
#include "select/propset.h"
#include "utils/utils.h"

static css_error compute_border_colors(css_computed_style *style);

static css_error compute_absolute_border_width(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_border_side_width(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit));
static css_error compute_absolute_sides(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_clip(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_line_height(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_margins(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_padding(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_vertical_align(css_computed_style *style,
		const css_hint_length *ex_size);
static css_error compute_absolute_length(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit));
static css_error compute_absolute_length_auto(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit));
static css_error compute_absolute_length_none(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit));
static css_error compute_absolute_length_normal(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit));
static css_error compute_absolute_length_pair(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len1, css_unit *unit1,
				css_fixed *len2, css_unit *unit2),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len1, css_unit unit1,
				css_fixed len2, css_unit unit2));


/**
 * Create a computed style
 *
 * \param alloc   Memory (de)allocation function
 * \param pw      Pointer to client-specific data
 * \param result  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_BADPARM on bad parameters.
 */
css_error css_computed_style_create(css_allocator_fn alloc, void *pw,
		css_computed_style **result)
{
	css_computed_style *s;

	if (alloc == NULL || result == NULL)
		return CSS_BADPARM;

	s = alloc(NULL, sizeof(css_computed_style), pw);
	if (s == NULL)
		return CSS_NOMEM;

	memset(s, 0, sizeof(css_computed_style));

	s->alloc = alloc;
	s->pw = pw;

	*result = s;

	return CSS_OK;
}

/**
 * Destroy a computed style
 *
 * \param style  Style to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_computed_style_destroy(css_computed_style *style)
{
	if (style == NULL)
		return CSS_BADPARM;

	if (style->uncommon != NULL) {
		if (style->uncommon->counter_increment != NULL) {
			css_computed_counter *c;

			for (c = style->uncommon->counter_increment; 
					c->name != NULL; c++) {
				lwc_string_unref(c->name);
			}

			style->alloc(style->uncommon->counter_increment, 0,
					style->pw);
		}

		if (style->uncommon->counter_reset != NULL) {
			css_computed_counter *c;

			for (c = style->uncommon->counter_reset; 
					c->name != NULL; c++) {
				lwc_string_unref(c->name);
			}

			style->alloc(style->uncommon->counter_reset, 0,
					style->pw);
		}
	
		if (style->uncommon->cursor != NULL) {
			lwc_string **s;

			for (s = style->uncommon->cursor; *s != NULL; s++) {
				lwc_string_unref(*s);
			}

			style->alloc(style->uncommon->cursor, 0, style->pw);
		}

		if (style->uncommon->content != NULL) {
			css_computed_content_item *c;

			for (c = style->uncommon->content; 
					c->type != CSS_COMPUTED_CONTENT_NONE;
					c++) {
				switch (c->type) {
				case CSS_COMPUTED_CONTENT_STRING:
					lwc_string_unref(c->data.string);
					break;
				case CSS_COMPUTED_CONTENT_URI:
					lwc_string_unref(c->data.uri);
					break;
				case CSS_COMPUTED_CONTENT_ATTR:
					lwc_string_unref(c->data.attr);
					break;
				case CSS_COMPUTED_CONTENT_COUNTER:
					lwc_string_unref(c->data.counter.name);
					break;
				case CSS_COMPUTED_CONTENT_COUNTERS:
					lwc_string_unref(c->data.counters.name);
					lwc_string_unref(c->data.counters.sep);
					break;
				default:
					break;
				}
			}

			style->alloc(style->uncommon->content, 0, style->pw);
		}

		style->alloc(style->uncommon, 0, style->pw);
	}

	if (style->page != NULL) {
		style->alloc(style->page, 0, style->pw);
	}

	if (style->aural != NULL) {
		style->alloc(style->aural, 0, style->pw);
	}

	if (style->font_family != NULL) {
		lwc_string **s;

		for (s = style->font_family; *s != NULL; s++) {
			lwc_string_unref(*s);
		}

		style->alloc(style->font_family, 0, style->pw);
	}

	if (style->quotes != NULL) {
		lwc_string **s;

		for (s = style->quotes; *s != NULL; s++) {
			lwc_string_unref(*s);
		}

		style->alloc(style->quotes, 0, style->pw);
	}

	if (style->list_style_image != NULL)
		lwc_string_unref(style->list_style_image);

	if (style->background_image != NULL)
		lwc_string_unref(style->background_image);

	style->alloc(style, 0, style->pw);

	return CSS_OK;
}

/**
 * Populate a blank computed style with Initial values
 *
 * \param style    Computed style to populate
 * \param handler  Dispatch table of handler functions
 * \param pw       Client-specific private data for handler functions
 * \return CSS_OK on success.
 */
css_error css_computed_style_initialise(css_computed_style *style,
		css_select_handler *handler, void *pw)
{
	css_select_state state;
	uint32_t i;
	css_error error;

	if (style == NULL)
		return CSS_BADPARM;

	state.node = NULL;
	state.pseudo_element = CSS_PSEUDO_ELEMENT_NONE;
	state.media = CSS_MEDIA_ALL;
	state.result = style;
	state.handler = handler;
	state.pw = pw;

	for (i = 0; i < CSS_N_PROPERTIES; i++) {
		/* No need to initialise anything other than the normal
		 * properties -- the others are handled by the accessors */
		if (prop_dispatch[i].inherited == false &&
				prop_dispatch[i].group == GROUP_NORMAL) {
			error = prop_dispatch[i].initial(&state);
			if (error != CSS_OK)
				return error;
		}
	}

	return CSS_OK;
}

/**
 * Compose two computed styles
 *
 * \param parent             Parent style
 * \param child              Child style
 * \param compute_font_size  Function to compute an absolute font size
 * \param pw                 Client data for compute_font_size
 * \param result             Pointer to style to compose into
 * \return CSS_OK on success, appropriate error otherwise.
 *
 * \pre \a parent is a fully composed style (thus has no inherited properties)
 *
 * \note \a child and \a result may point at the same object
 */
css_error css_computed_style_compose(const css_computed_style *parent,
		const css_computed_style *child,
		css_error (*compute_font_size)(void *pw,
			const css_hint *parent, css_hint *size),
		void *pw,
		css_computed_style *result)
{
	css_error error = CSS_OK;
	size_t i;

	/* Iterate through the properties */
	for (i = 0; i < CSS_N_PROPERTIES; i++) {
		/* Skip any in extension blocks if the block does not exist */	
		if (prop_dispatch[i].group == GROUP_UNCOMMON &&
				parent->uncommon == NULL && 
				child->uncommon == NULL)
			continue;

		if (prop_dispatch[i].group == GROUP_PAGE &&
				parent->page == NULL && child->page == NULL)
			continue;

		if (prop_dispatch[i].group == GROUP_AURAL &&
				parent->aural == NULL && child->aural == NULL)
			continue;

		/* Compose the property */
		error = prop_dispatch[i].compose(parent, child, result);
		if (error != CSS_OK)
			break;
	}

	/* Finally, compute absolute values for everything */
	return compute_absolute_values(parent, result, compute_font_size, pw);
}

/******************************************************************************
 * Library internals                                                          *
 ******************************************************************************/

/**
 * Compute the absolute values of a style
 *
 * \param parent             Parent style, or NULL for tree root
 * \param style              Computed style to process
 * \param compute_font_size  Callback to calculate an absolute font-size
 * \param pw                 Private word for callback
 * \return CSS_OK on success.
 */
css_error compute_absolute_values(const css_computed_style *parent,
		css_computed_style *style,
		css_error (*compute_font_size)(void *pw, 
			const css_hint *parent, css_hint *size),
		void *pw)
{
	css_hint psize, size, ex_size;
	css_error error;

	/* Ensure font-size is absolute */
	if (parent != NULL) {
		psize.status = get_font_size(parent, 
				&psize.data.length.value, 
				&psize.data.length.unit);
	}

	size.status = get_font_size(style, 
			&size.data.length.value, 
			&size.data.length.unit);

	error = compute_font_size(pw, parent != NULL ? &psize : NULL, &size);
	if (error != CSS_OK)
		return error;

	error = set_font_size(style, size.status,
			size.data.length.value, 
			size.data.length.unit);
	if (error != CSS_OK)
		return error;

	/* Compute the size of an ex unit */
	ex_size.status = CSS_FONT_SIZE_DIMENSION;
	ex_size.data.length.value = INTTOFIX(1);
	ex_size.data.length.unit = CSS_UNIT_EX;
	error = compute_font_size(pw, &size, &ex_size);
	if (error != CSS_OK)
		return error;

	/* Convert ex size into ems */
	if (size.data.length.value != 0)
		ex_size.data.length.value = FDIV(ex_size.data.length.value, 
					size.data.length.value);
	else
		ex_size.data.length.value = 0;
	ex_size.data.length.unit = CSS_UNIT_EM;

	/* Fix up background-position */
	error = compute_absolute_length_pair(style, &ex_size.data.length, 
			get_background_position,
			set_background_position);
	if (error != CSS_OK)
		return error;

	/* Fix up border-{top,right,bottom,left}-color */
	error = compute_border_colors(style);
	if (error != CSS_OK)
		return error;

	/* Fix up border-{top,right,bottom,left}-width */
	error = compute_absolute_border_width(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up sides */
	error = compute_absolute_sides(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up height */
	error = compute_absolute_length_auto(style, &ex_size.data.length, 
			get_height, set_height);
	if (error != CSS_OK)
		return error;

	/* Fix up line-height (must be before vertical-align) */
	error = compute_absolute_line_height(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up margins */
	error = compute_absolute_margins(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up max-height */
	error = compute_absolute_length_none(style, &ex_size.data.length, 
			get_max_height, set_max_height);
	if (error != CSS_OK)
		return error;

	/* Fix up max-width */
	error = compute_absolute_length_none(style, &ex_size.data.length, 
			get_max_width, set_max_width);
	if (error != CSS_OK)
		return error;

	/* Fix up min-height */
	error = compute_absolute_length(style, &ex_size.data.length, 
			get_min_height, set_min_height);
	if (error != CSS_OK)
		return error;

	/* Fix up min-width */
	error = compute_absolute_length(style, &ex_size.data.length, 
			get_min_width, set_min_width);
	if (error != CSS_OK)
		return error;

	/* Fix up padding */
	error = compute_absolute_padding(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up text-indent */
	error = compute_absolute_length(style, &ex_size.data.length, 
			get_text_indent, set_text_indent);
	if (error != CSS_OK)
		return error;

	/* Fix up vertical-align */
	error = compute_absolute_vertical_align(style, &ex_size.data.length);
	if (error != CSS_OK)
		return error;

	/* Fix up width */
	error = compute_absolute_length_auto(style, &ex_size.data.length, 
			get_width, set_width);
	if (error != CSS_OK)
		return error;

	/* Uncommon properties */
	if (style->uncommon != NULL) {
		/* Fix up border-spacing */
		error = compute_absolute_length_pair(style,
				&ex_size.data.length,
				get_border_spacing,
				set_border_spacing);
		if (error != CSS_OK)
			return error;

		/* Fix up clip */
		error = compute_absolute_clip(style, &ex_size.data.length);
		if (error != CSS_OK)
			return error;

		/* Fix up letter-spacing */
		error = compute_absolute_length_normal(style,
				&ex_size.data.length,
				get_letter_spacing, 
				set_letter_spacing);
		if (error != CSS_OK)
			return error;

		/* Fix up outline-width */
		error = compute_absolute_border_side_width(style, 
				&ex_size.data.length, 
				get_outline_width, 
				set_outline_width);
		if (error != CSS_OK)
			return error;

		/* Fix up word spacing */
		error = compute_absolute_length_normal(style,
				&ex_size.data.length,
				get_word_spacing, 
				set_word_spacing);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

/******************************************************************************
 * Absolute value calculators
 ******************************************************************************/

/**
 * Compute border colours, replacing any set to initial with 
 * the computed value of color.
 *
 * \param style  The style to process
 * \return CSS_OK on success
 */
css_error compute_border_colors(css_computed_style *style)
{
	css_color color, bcol;
	css_error error;

	css_computed_color(style, &color);

	if (get_border_top_color(style, &bcol) == CSS_BORDER_COLOR_INITIAL) {
		error = set_border_top_color(style, 
				CSS_BORDER_COLOR_COLOR, color);
		if (error != CSS_OK)
			return error;
	}

	if (get_border_right_color(style, &bcol) == CSS_BORDER_COLOR_INITIAL) {
		error = set_border_right_color(style, 
				CSS_BORDER_COLOR_COLOR, color);
		if (error != CSS_OK)
			return error;
	}

	if (get_border_bottom_color(style, &bcol) == CSS_BORDER_COLOR_INITIAL) {
		error = set_border_bottom_color(style, 
				CSS_BORDER_COLOR_COLOR, color);
		if (error != CSS_OK)
			return error;
	}

	if (get_border_left_color(style, &bcol) == CSS_BORDER_COLOR_INITIAL) {
		error = set_border_left_color(style, 
				CSS_BORDER_COLOR_COLOR, color);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

/**
 * Compute absolute border widths
 *
 * \param style      Style to process
 * \param ex_size    Ex size in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_border_width(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_error error;

	error = compute_absolute_border_side_width(style, ex_size,
			get_border_top_width, 
			set_border_top_width);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_border_side_width(style, ex_size,
			get_border_right_width, 
			set_border_right_width);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_border_side_width(style, ex_size,
			get_border_bottom_width, 
			set_border_bottom_width);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_border_side_width(style, ex_size,
			get_border_left_width, 
			set_border_left_width);
	if (error != CSS_OK)
		return error;

	return CSS_OK;
}

/**
 * Compute an absolute border side width
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_border_side_width(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit))
{
	css_fixed length;
	css_unit unit;
	uint8_t type;

	type = get(style, &length, &unit);
	if (type == CSS_BORDER_WIDTH_THIN) {
		length = INTTOFIX(1);
		unit = CSS_UNIT_PX;
	} else if (type == CSS_BORDER_WIDTH_MEDIUM) {
		length = INTTOFIX(2);
		unit = CSS_UNIT_PX;
	} else if (type == CSS_BORDER_WIDTH_THICK) {
		length = INTTOFIX(4);
		unit = CSS_UNIT_PX;
	}

	if (unit == CSS_UNIT_EX) {
		length = FMUL(length, ex_size->value);
		unit = ex_size->unit;
	}

	return set(style, CSS_BORDER_WIDTH_WIDTH, length, unit);
}

/**
 * Compute absolute clip
 *
 * \param style      Style to process
 * \param ex_size    Ex size in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_clip(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_computed_clip_rect rect = { 0, 0, 0, 0, CSS_UNIT_PX, CSS_UNIT_PX,
			CSS_UNIT_PX, CSS_UNIT_PX, false, false, false, false };
	css_error error;

	if (get_clip(style, &rect) == CSS_CLIP_RECT) {
		if (rect.top_auto == false) {
			if (rect.tunit == CSS_UNIT_EX) {
				rect.top = FMUL(rect.top, ex_size->value);
				rect.tunit = ex_size->unit;
			}
		}

		if (rect.right_auto == false) {
			if (rect.runit == CSS_UNIT_EX) {
				rect.right = FMUL(rect.right, ex_size->value);
				rect.runit = ex_size->unit;
			}
		}

		if (rect.bottom_auto == false) {
			if (rect.bunit == CSS_UNIT_EX) {
				rect.bottom = FMUL(rect.bottom, ex_size->value);
				rect.bunit = ex_size->unit;
			}
		}

		if (rect.left_auto == false) {
			if (rect.lunit == CSS_UNIT_EX) {
				rect.left = FMUL(rect.left, ex_size->value);
				rect.lunit = ex_size->unit;
			}
		}

		error = set_clip(style, CSS_CLIP_RECT, &rect);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

/**
 * Compute absolute line-height
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_line_height(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type;
	css_error error;

	type = get_line_height(style, &length, &unit);

	if (type == CSS_LINE_HEIGHT_DIMENSION) {
		if (unit == CSS_UNIT_EX) {
			length = FMUL(length, ex_size->value);
			unit = ex_size->unit;
		}

		error = set_line_height(style, type, length, unit);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

/**
 * Compute the absolute values of {top,right,bottom,left}
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_sides(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_error error;

	/* Calculate absolute lengths for sides */
	error = compute_absolute_length_auto(style, ex_size, get_top, set_top);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_right, set_right);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_bottom, set_bottom);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_left, set_left);
	if (error != CSS_OK)
		return error;

	return CSS_OK;
}

/**
 * Compute absolute margins
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_margins(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_error error;

	error = compute_absolute_length_auto(style, ex_size,
			get_margin_top, set_margin_top);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_margin_right, set_margin_right);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_margin_bottom, set_margin_bottom);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length_auto(style, ex_size,
			get_margin_left, set_margin_left);
	if (error != CSS_OK)
		return error;
	
	return CSS_OK;
}

/**
 * Compute absolute padding
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_padding(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_error error;

	error = compute_absolute_length(style, ex_size,
			get_padding_top, set_padding_top);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length(style, ex_size,
			get_padding_right, set_padding_right);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length(style, ex_size,
			get_padding_bottom, set_padding_bottom);
	if (error != CSS_OK)
		return error;

	error = compute_absolute_length(style, ex_size,
			get_padding_left, set_padding_left);
	if (error != CSS_OK)
		return error;
	
	return CSS_OK;
}

/**
 * Compute absolute vertical-align
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \return CSS_OK on success
 */
css_error compute_absolute_vertical_align(css_computed_style *style,
		const css_hint_length *ex_size)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type;
	css_error error;

	type = get_vertical_align(style, &length, &unit);

	if (type == CSS_VERTICAL_ALIGN_SET) {
		if (unit == CSS_UNIT_EX) {
			length = FMUL(length, ex_size->value);
			unit = ex_size->unit;
		}

		error = set_vertical_align(style, type, length, unit);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

/**
 * Compute the absolute value of length
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_length(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit))
{
	css_fixed length;
	css_unit unit;
	uint8_t type;

	type = get(style, &length, &unit);

	if (unit == CSS_UNIT_EX) {
		length = FMUL(length, ex_size->value);
		unit = ex_size->unit;
	}

	return set(style, type, length, unit);
}

/**
 * Compute the absolute value of length or auto
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_length_auto(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit))
{
	css_fixed length;
	css_unit unit;
	uint8_t type;

	type = get(style, &length, &unit);
	if (type != CSS_BOTTOM_AUTO) {
		if (unit == CSS_UNIT_EX) {
			length = FMUL(length, ex_size->value);
			unit = ex_size->unit;
		}

		return set(style, CSS_BOTTOM_SET, length, unit);
	}

	return set(style, CSS_BOTTOM_AUTO, 0, CSS_UNIT_PX);
}

/**
 * Compute the absolute value of length or none
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_length_none(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit))
{
	css_fixed length;
	css_unit unit;
	uint8_t type;

	type = get(style, &length, &unit);
	if (type != CSS_MAX_HEIGHT_NONE) {
		if (unit == CSS_UNIT_EX) {
			length = FMUL(length, ex_size->value);
			unit = ex_size->unit;
		}

		return set(style, CSS_MAX_HEIGHT_SET, length, unit);
	}

	return set(style, CSS_MAX_HEIGHT_NONE, 0, CSS_UNIT_PX);
}

/**
 * Compute the absolute value of length or normal
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_length_normal(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len, css_unit *unit),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len, css_unit unit))
{
	css_fixed length;
	css_unit unit;
	uint8_t type;

	type = get(style, &length, &unit);
	if (type != CSS_LETTER_SPACING_NORMAL) {
		if (unit == CSS_UNIT_EX) {
			length = FMUL(length, ex_size->value);
			unit = ex_size->unit;
		}

		return set(style, CSS_LETTER_SPACING_SET, length, unit);
	}

	return set(style, CSS_LETTER_SPACING_NORMAL, 0, CSS_UNIT_PX);
}

/**
 * Compute the absolute value of length pair
 *
 * \param style      Style to process
 * \param ex_size    Ex size, in ems
 * \param get        Function to read length
 * \param set        Function to write length
 * \return CSS_OK on success
 */
css_error compute_absolute_length_pair(css_computed_style *style,
		const css_hint_length *ex_size,
		uint8_t (*get)(const css_computed_style *style, 
				css_fixed *len1, css_unit *unit1,
				css_fixed *len2, css_unit *unit2),
		css_error (*set)(css_computed_style *style, uint8_t type,
				css_fixed len1, css_unit unit1,
				css_fixed len2, css_unit unit2))
{
	css_fixed length1, length2;
	css_unit unit1, unit2;
	uint8_t type;

	type = get(style, &length1, &unit1, &length2, &unit2);

	if (unit1 == CSS_UNIT_EX) {
		length1 = FMUL(length1, ex_size->value);
		unit1 = ex_size->unit;
	}

	if (unit2 == CSS_UNIT_EX) {
		length2 = FMUL(length2, ex_size->value);
		unit2 = ex_size->unit;
	}

	return set(style, type, length1, unit1, length2, unit2);
}

