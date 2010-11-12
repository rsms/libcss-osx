/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/properties.h"
#include "select/propget.h"
#include "select/propset.h"
#include "utils/utils.h"

static css_unit to_css_unit(uint32_t u);
static css_error cascade_bg_border_color(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t, css_color));
static css_error cascade_uri_none(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, 
				lwc_string *));
static css_error cascade_border_style(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t));
static css_error cascade_border_width(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed, 
				css_unit));
static css_error cascade_length_auto(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit));
static css_error cascade_length_normal(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit));
static css_error cascade_length_none(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit));
static css_error cascade_length(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit));
static css_error cascade_number(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed));
static css_error cascade_page_break_after_before(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t));
static css_error cascade_counter_increment_reset(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t,
				css_computed_counter *));

/* Generic destructors */

static uint32_t generic_destroy_color(void *bytecode)
{
	return sizeof(uint32_t) + 
		((getValue(*((uint32_t*)bytecode)) == BACKGROUND_COLOR_SET) ? sizeof(css_color) : 0);
}

static uint32_t generic_destroy_uri(void *bytecode)
{
	bool has_uri = (getValue(*((uint32_t*)bytecode)) & BACKGROUND_IMAGE_URI) == BACKGROUND_IMAGE_URI;
	
	if (has_uri) {
		void *vstr = (((uint8_t*)bytecode) + sizeof(uint32_t));
		lwc_string *str = *(lwc_string **) vstr;
		lwc_string_unref(str);
	}
	return sizeof(uint32_t) + (has_uri ? sizeof(lwc_string*) : 0);
}

static uint32_t generic_destroy_length(void *bytecode)
{
	bool has_length = (getValue(*((uint32_t*)bytecode)) & BORDER_WIDTH_SET) == BORDER_WIDTH_SET;
	
	return sizeof(uint32_t) + (has_length ? sizeof(css_fixed) + sizeof(uint32_t) : 0);
}

static uint32_t generic_destroy_number(void *bytecode)
{
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bool has_number = (value == ORPHANS_SET);
	
	return sizeof(uint32_t) + (has_number ? sizeof(css_fixed) : 0);
}

/* Useful helpers */

css_unit to_css_unit(uint32_t u)
{
	switch (u) {
	case UNIT_PX: return CSS_UNIT_PX;
	case UNIT_EX: return CSS_UNIT_EX;
	case UNIT_EM: return CSS_UNIT_EM;
	case UNIT_IN: return CSS_UNIT_IN;
	case UNIT_CM: return CSS_UNIT_CM;
	case UNIT_MM: return CSS_UNIT_MM;
	case UNIT_PT: return CSS_UNIT_PT;
	case UNIT_PC: return CSS_UNIT_PC;
	case UNIT_PCT: return CSS_UNIT_PCT;
	case UNIT_DEG: return CSS_UNIT_DEG;
	case UNIT_GRAD: return CSS_UNIT_GRAD;
	case UNIT_RAD: return CSS_UNIT_RAD;
	case UNIT_MS: return CSS_UNIT_MS;
	case UNIT_S: return CSS_UNIT_S;
	case UNIT_HZ: return CSS_UNIT_HZ;
	case UNIT_KHZ: return CSS_UNIT_KHZ;
	}

	return 0;
}

css_error cascade_azimuth(uint32_t opv, css_style *style,
		 css_select_state *state)
{
	uint16_t value = 0;
	css_fixed val = 0;
	uint32_t unit = UNIT_DEG;

	if (isInherit(opv) == false) {
		switch (getValue(opv) & ~AZIMUTH_BEHIND) {
		case AZIMUTH_ANGLE:
			value = 0;

			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case AZIMUTH_LEFTWARDS:
		case AZIMUTH_RIGHTWARDS:
		case AZIMUTH_LEFT_SIDE:
		case AZIMUTH_FAR_LEFT:
		case AZIMUTH_LEFT:
		case AZIMUTH_CENTER_LEFT:
		case AZIMUTH_CENTER:
		case AZIMUTH_CENTER_RIGHT:
		case AZIMUTH_RIGHT:
		case AZIMUTH_FAR_RIGHT:
		case AZIMUTH_RIGHT_SIDE:
			/** \todo azimuth values */
			break;
		}

		/** \todo azimuth behind */
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state, 
			isInherit(opv))) {
		/** \todo set computed azimuth */
	}

	return CSS_OK;
}

css_error set_azimuth_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_azimuth(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_azimuth(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_azimuth(void *bytecode)
{
	bool has_angle = (((getValue(*(uint32_t*)bytecode) & (1<<7)) != 0));
	uint32_t extra_size =  has_angle ? (sizeof(css_fixed) + sizeof(uint32_t)) : 0;
	
	return sizeof(uint32_t) + extra_size;
}

css_error cascade_background_attachment(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_BACKGROUND_ATTACHMENT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BACKGROUND_ATTACHMENT_FIXED:
			value = CSS_BACKGROUND_ATTACHMENT_FIXED;
			break;
		case BACKGROUND_ATTACHMENT_SCROLL:
			value = CSS_BACKGROUND_ATTACHMENT_SCROLL;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_background_attachment(state->result, value);
	}

	return CSS_OK;
}

css_error set_background_attachment_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_background_attachment(style, hint->status);
}

css_error initial_background_attachment(css_select_state *state)
{
	return set_background_attachment(state->result, 
			CSS_BACKGROUND_ATTACHMENT_SCROLL);
}

css_error compose_background_attachment(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_background_attachment(child);

	if (type == CSS_BACKGROUND_ATTACHMENT_INHERIT) {
		type = get_background_attachment(parent);
	}

	return set_background_attachment(result, type);
}

uint32_t destroy_background_attachment(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_background_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_bg_border_color(opv, style, state, set_background_color);
}

css_error set_background_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_background_color(style, hint->status, hint->data.color);
}

css_error initial_background_color(css_select_state *state)
{
	return set_background_color(state->result, 
			CSS_BACKGROUND_COLOR_TRANSPARENT, 0);
}

css_error compose_background_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_background_color(child, &color);

	if (type == CSS_BACKGROUND_COLOR_INHERIT) {
		type = get_background_color(parent, &color);
	}

	return set_background_color(result, type, color);
}

uint32_t destroy_background_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_background_image(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_uri_none(opv, style, state, set_background_image);
}

css_error set_background_image_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	css_error error;

	error = set_background_image(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error initial_background_image(css_select_state *state)
{
	return set_background_image(state->result, 
			CSS_BACKGROUND_IMAGE_NONE, NULL);
}

css_error compose_background_image(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_background_image(child, &url);

	if (type == CSS_BACKGROUND_IMAGE_INHERIT) {
		type = get_background_image(parent, &url);
	}

	return set_background_image(result, type, url);
}

uint32_t destroy_background_image(void *bytecode)
{
	return generic_destroy_uri(bytecode);
}

css_error cascade_background_position(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_BACKGROUND_POSITION_INHERIT;
	css_fixed hlength = 0;
	css_fixed vlength = 0;
	uint32_t hunit = UNIT_PX;
	uint32_t vunit = UNIT_PX;

	if (isInherit(opv) == false) {
		value = CSS_BACKGROUND_POSITION_SET;

		switch (getValue(opv) & 0xf0) {
		case BACKGROUND_POSITION_HORZ_SET:
			hlength = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(hlength));
			hunit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(hunit));
			break;
		case BACKGROUND_POSITION_HORZ_CENTER:
			hlength = INTTOFIX(50);
			hunit = UNIT_PCT;
			break;
		case BACKGROUND_POSITION_HORZ_RIGHT:
			hlength = INTTOFIX(100);
			hunit = UNIT_PCT;
			break;
		case BACKGROUND_POSITION_HORZ_LEFT:
			hlength = INTTOFIX(0);
			hunit = UNIT_PCT;
			break;
		}

		switch (getValue(opv) & 0x0f) {
		case BACKGROUND_POSITION_VERT_SET:
			vlength = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(vlength));
			vunit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(vunit));
			break;
		case BACKGROUND_POSITION_VERT_CENTER:
			vlength = INTTOFIX(50);
			vunit = UNIT_PCT;
			break;
		case BACKGROUND_POSITION_VERT_BOTTOM:
			vlength = INTTOFIX(100);
			vunit = UNIT_PCT;
			break;
		case BACKGROUND_POSITION_VERT_TOP:
			vlength = INTTOFIX(0);
			vunit = UNIT_PCT;
			break;
		}
	}

	hunit = to_css_unit(hunit);
	vunit = to_css_unit(vunit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_background_position(state->result, value,
				hlength, hunit, vlength, vunit);
	}

	return CSS_OK;
}

css_error set_background_position_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_background_position(style, hint->status, 
		hint->data.position.h.value, hint->data.position.h.unit,
		hint->data.position.v.value, hint->data.position.v.unit);
}

css_error initial_background_position(css_select_state *state)
{
	return set_background_position(state->result, 
			CSS_BACKGROUND_POSITION_SET, 
			0, CSS_UNIT_PCT, 0, CSS_UNIT_PCT);
}

css_error compose_background_position(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed hlength = 0, vlength = 0;
	css_unit hunit = CSS_UNIT_PX, vunit = CSS_UNIT_PX;
	uint8_t type = get_background_position(child, &hlength, &hunit, 
			&vlength, &vunit);

	if (type == CSS_BACKGROUND_POSITION_INHERIT) {
		type = get_background_position(parent,
				&hlength, &hunit, &vlength, &vunit);
	}

	return set_background_position(result, type, hlength, hunit,
				vlength, vunit);
}


uint32_t destroy_background_position(void *bytecode)
{
	uint32_t value = getValue(*((uint32_t*)bytecode));
	uint32_t extra_size = 0;
	
	if ((value & 0x0f) == BACKGROUND_POSITION_VERT_SET)
		extra_size += sizeof(css_fixed) + sizeof(uint32_t);
	if ((value & 0xf0) == BACKGROUND_POSITION_HORZ_SET)
		extra_size += sizeof(css_fixed) + sizeof(uint32_t);
	
	return sizeof(uint32_t) + extra_size;
}

css_error cascade_background_repeat(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_BACKGROUND_REPEAT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BACKGROUND_REPEAT_NO_REPEAT:
			value = CSS_BACKGROUND_REPEAT_NO_REPEAT;
			break;
		case BACKGROUND_REPEAT_REPEAT_X:
			value = CSS_BACKGROUND_REPEAT_REPEAT_X;
			break;
		case BACKGROUND_REPEAT_REPEAT_Y:
			value = CSS_BACKGROUND_REPEAT_REPEAT_Y;
			break;
		case BACKGROUND_REPEAT_REPEAT:
			value = CSS_BACKGROUND_REPEAT_REPEAT;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_background_repeat(state->result, value);
	}

	return CSS_OK;
}

css_error set_background_repeat_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_background_repeat(style, hint->status);
}

css_error initial_background_repeat(css_select_state *state)
{
	return set_background_repeat(state->result, 
			CSS_BACKGROUND_REPEAT_REPEAT);
}

css_error compose_background_repeat(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_background_repeat(child);

	if (type == CSS_BACKGROUND_REPEAT_INHERIT) {
		type = get_background_repeat(parent);
	}

	return set_background_repeat(result, type);
}

uint32_t destroy_background_repeat(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_collapse(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_BORDER_COLLAPSE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BORDER_COLLAPSE_SEPARATE:
			value = CSS_BORDER_COLLAPSE_SEPARATE;
			break;
		case BORDER_COLLAPSE_COLLAPSE:
			value = CSS_BORDER_COLLAPSE_COLLAPSE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_border_collapse(state->result, value);
	}

	return CSS_OK;
}

css_error set_border_collapse_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_collapse(style, hint->status);
}

css_error initial_border_collapse(css_select_state *state)
{
	return set_border_collapse(state->result, CSS_BORDER_COLLAPSE_SEPARATE);
}

css_error compose_border_collapse(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_border_collapse(child);

	if (type == CSS_BORDER_COLLAPSE_INHERIT) {
		type = get_border_collapse(parent);
	}
	
	return set_border_collapse(result, type);
}

uint32_t destroy_border_collapse(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_spacing(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_BORDER_SPACING_INHERIT;
	css_fixed hlength = 0;
	css_fixed vlength = 0;
	uint32_t hunit = UNIT_PX;
	uint32_t vunit = UNIT_PX;

	if (isInherit(opv) == false) {
		value = CSS_BORDER_SPACING_SET;
		hlength = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(hlength));
		hunit = *((uint32_t *) style->bytecode);
		advance_bytecode(style, sizeof(hunit));

		vlength = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(vlength));
		vunit = *((uint32_t *) style->bytecode);
		advance_bytecode(style, sizeof(vunit));
	}

	hunit = to_css_unit(hunit);
	vunit = to_css_unit(vunit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_border_spacing(state->result, value,
				hlength, hunit, vlength, vunit);
	}

	return CSS_OK;
}

css_error set_border_spacing_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_spacing(style, hint->status,
		hint->data.position.h.value, hint->data.position.h.unit,
		hint->data.position.v.value, hint->data.position.v.unit);
}

css_error initial_border_spacing(css_select_state *state)
{
	return set_border_spacing(state->result, CSS_BORDER_SPACING_SET,
			0, CSS_UNIT_PX, 0, CSS_UNIT_PX);
}

css_error compose_border_spacing(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed hlength = 0, vlength = 0;
	css_unit hunit = CSS_UNIT_PX, vunit = CSS_UNIT_PX;
	uint8_t type = get_border_spacing(child, &hlength, &hunit, 
			&vlength, &vunit);

	if ((child->uncommon == NULL && parent->uncommon != NULL) || 
			type == CSS_BORDER_SPACING_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) || 
				type == CSS_BORDER_SPACING_INHERIT) {
			type = get_border_spacing(parent, 
					&hlength, &hunit, &vlength, &vunit);
		}

		return set_border_spacing(result, type, hlength, hunit, 
				vlength, vunit);
	}

	return CSS_OK;
}

uint32_t destroy_border_spacing(void *bytecode)
{
	bool has_values = (getValue(*((uint32_t*)bytecode)) == BORDER_SPACING_SET);
	
	return sizeof(uint32_t) + (has_values ? (sizeof(css_fixed) + sizeof(uint32_t)) * 2 : 0);
}

css_error cascade_border_top_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_bg_border_color(opv, style, state, set_border_top_color);
}

css_error set_border_top_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_top_color(style, hint->status, hint->data.color);
}

css_error initial_border_top_color(css_select_state *state)
{
	return set_border_top_color(state->result, CSS_BORDER_COLOR_INITIAL, 0);
}

css_error compose_border_top_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_border_top_color(child, &color);

	if (type == CSS_BORDER_COLOR_INHERIT) {
		type = get_border_top_color(parent, &color);
	}

	return set_border_top_color(result, type, color);
}

uint32_t destroy_border_top_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_border_right_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_bg_border_color(opv, style, state, 
			set_border_right_color);
}

css_error set_border_right_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_right_color(style, hint->status, hint->data.color);
}

css_error initial_border_right_color(css_select_state *state)
{
	return set_border_right_color(state->result, 
			CSS_BORDER_COLOR_INITIAL, 0);
}

css_error compose_border_right_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_border_right_color(child, &color);

	if (type == CSS_BORDER_COLOR_INHERIT) {
		type = get_border_right_color(parent, &color);
	}

	return set_border_right_color(result, type, color);
}

uint32_t destroy_border_right_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_border_bottom_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_bg_border_color(opv, style, state,
			set_border_bottom_color);
}

css_error set_border_bottom_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_bottom_color(style, hint->status, hint->data.color);
}

css_error initial_border_bottom_color(css_select_state *state)
{
	return set_border_bottom_color(state->result, 
			CSS_BORDER_COLOR_INITIAL, 0);
}

css_error compose_border_bottom_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_border_bottom_color(child, &color);

	if (type == CSS_BORDER_COLOR_INHERIT) {
		type = get_border_bottom_color(parent, &color);
	}

	return set_border_bottom_color(result, type, color);
}

uint32_t destroy_border_bottom_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_border_left_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_bg_border_color(opv, style, state, 
			set_border_left_color);
}

css_error set_border_left_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_left_color(style, hint->status, hint->data.color);
}

css_error initial_border_left_color(css_select_state *state)
{
	return set_border_left_color(state->result, 
			CSS_BORDER_COLOR_INITIAL, 0);
}

css_error compose_border_left_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_border_left_color(child, &color);

	if (type == CSS_BORDER_COLOR_INHERIT) {
		type = get_border_left_color(parent, &color);
	}

	return set_border_left_color(result, type, color);
}

uint32_t destroy_border_left_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_border_top_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_style(opv, style, state, set_border_top_style);
}

css_error set_border_top_style_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_top_style(style, hint->status);
}

css_error initial_border_top_style(css_select_state *state)
{
	return set_border_top_style(state->result, CSS_BORDER_STYLE_NONE);
}

css_error compose_border_top_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_border_top_style(child);

	if (type == CSS_BORDER_STYLE_INHERIT) {
		type = get_border_top_style(parent);
	}

	return set_border_top_style(result, type);
}

uint32_t destroy_border_top_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_right_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_style(opv, style, state, set_border_right_style);
}

css_error set_border_right_style_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_right_style(style, hint->status);
}

css_error initial_border_right_style(css_select_state *state)
{
	return set_border_right_style(state->result, CSS_BORDER_STYLE_NONE);
}

css_error compose_border_right_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_border_right_style(child);

	if (type == CSS_BORDER_STYLE_INHERIT) {
		type = get_border_right_style(parent);
	}

	return set_border_right_style(result, type);
}

uint32_t destroy_border_right_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_bottom_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_style(opv, style, state, set_border_bottom_style);
}

css_error set_border_bottom_style_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_bottom_style(style, hint->status);
}

css_error initial_border_bottom_style(css_select_state *state)
{
	return set_border_bottom_style(state->result, CSS_BORDER_STYLE_NONE);
}

css_error compose_border_bottom_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_border_bottom_style(child);

	if (type == CSS_BORDER_STYLE_INHERIT) {
		type = get_border_bottom_style(parent);
	}

	return set_border_bottom_style(result, type);
}

uint32_t destroy_border_bottom_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_left_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_style(opv, style, state, set_border_left_style);
}

css_error set_border_left_style_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_left_style(style, hint->status);
}

css_error initial_border_left_style(css_select_state *state)
{
	return set_border_left_style(state->result, CSS_BORDER_STYLE_NONE);
}

css_error compose_border_left_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_border_left_style(child);

	if (type == CSS_BORDER_STYLE_INHERIT) {
		type = get_border_left_style(parent);
	}

	return set_border_left_style(result, type);
}

uint32_t destroy_border_left_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_border_top_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_width(opv, style, state, set_border_top_width);
}

css_error set_border_top_width_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_top_width(style, hint->status, 
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_border_top_width(css_select_state *state)
{
	return set_border_top_width(state->result, CSS_BORDER_WIDTH_MEDIUM, 
			0, CSS_UNIT_PX);
}

css_error compose_border_top_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_border_top_width(child, &length, &unit);

	if (type == CSS_BORDER_WIDTH_INHERIT) {
		type = get_border_top_width(parent, &length, &unit);
	}

	return set_border_top_width(result, type, length, unit);
}

uint32_t destroy_border_top_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_border_right_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_width(opv, style, state, set_border_right_width);
}

css_error set_border_right_width_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_right_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_border_right_width(css_select_state *state)
{
	return set_border_right_width(state->result, CSS_BORDER_WIDTH_MEDIUM,
			0, CSS_UNIT_PX);
}

css_error compose_border_right_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_border_right_width(child, &length, &unit);

	if (type == CSS_BORDER_WIDTH_INHERIT) {
		type = get_border_right_width(parent, &length, &unit);
	}

	return set_border_right_width(result, type, length, unit);
}

uint32_t destroy_border_right_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_border_bottom_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_width(opv, style, state, set_border_bottom_width);
}

css_error set_border_bottom_width_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_bottom_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_border_bottom_width(css_select_state *state)
{
	return set_border_bottom_width(state->result, CSS_BORDER_WIDTH_MEDIUM,
			0, CSS_UNIT_PX);
}

css_error compose_border_bottom_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_border_bottom_width(child, &length, &unit);

	if (type == CSS_BORDER_WIDTH_INHERIT) {
		type = get_border_bottom_width(parent, &length, &unit);
	}

	return set_border_bottom_width(result, type, length, unit);
}

uint32_t destroy_border_bottom_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_border_left_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_width(opv, style, state, set_border_left_width);
}

css_error set_border_left_width_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_border_left_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_border_left_width(css_select_state *state)
{
	return set_border_left_width(state->result, CSS_BORDER_WIDTH_MEDIUM,
			0, CSS_UNIT_PX);
}

css_error compose_border_left_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_border_left_width(child, &length, &unit);

	if (type == CSS_BORDER_WIDTH_INHERIT) {
		type = get_border_left_width(parent, &length, &unit);
	}

	return set_border_left_width(result, type, length, unit);
}

uint32_t destroy_border_left_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_bottom(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_bottom);
}

css_error set_bottom_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_bottom(style, hint->status, 
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_bottom(css_select_state *state)
{
	return set_bottom(state->result, CSS_BOTTOM_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_bottom(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_bottom(child, &length, &unit);

	if (type == CSS_BOTTOM_INHERIT) {
		type = get_bottom(parent, &length, &unit);
	}

	return set_bottom(result, type, length, unit);
}

uint32_t destroy_bottom(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_caption_side(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_CAPTION_SIDE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case CAPTION_SIDE_TOP:
			value = CSS_CAPTION_SIDE_TOP;
			break;
		case CAPTION_SIDE_BOTTOM:
			value = CSS_CAPTION_SIDE_BOTTOM;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_caption_side(state->result, value);
	}

	return CSS_OK;
}

css_error set_caption_side_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_caption_side(style, hint->status);
}

css_error initial_caption_side(css_select_state *state)
{
	return set_caption_side(state->result, CSS_CAPTION_SIDE_TOP);
}

css_error compose_caption_side(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_caption_side(child);

	if (type == CSS_CAPTION_SIDE_INHERIT) {
		type = get_caption_side(parent);
	}
	
	return set_caption_side(result, type);
}

uint32_t destroy_caption_side(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_clear(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_CLEAR_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case CLEAR_NONE:
			value = CSS_CLEAR_NONE;
			break;
		case CLEAR_LEFT:
			value = CSS_CLEAR_LEFT;
			break;
		case CLEAR_RIGHT:
			value = CSS_CLEAR_RIGHT;
			break;
		case CLEAR_BOTH:
			value = CSS_CLEAR_BOTH;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_clear(state->result, value);
	}

	return CSS_OK;
}

css_error set_clear_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_clear(style, hint->status);
}

css_error initial_clear(css_select_state *state)
{
	return set_clear(state->result, CSS_CLEAR_NONE);
}

css_error compose_clear(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_clear(child);

	if (type == CSS_CLEAR_INHERIT) {
		type = get_clear(parent);
	}

	return set_clear(result, type);
}

uint32_t destroy_clear(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_clip(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_CLIP_INHERIT;
	css_computed_clip_rect rect = { 0, 0, 0, 0, 
			UNIT_PX, UNIT_PX, UNIT_PX, UNIT_PX,
			false, false, false, false };

	if (isInherit(opv) == false) {
		switch (getValue(opv) & CLIP_SHAPE_MASK) {
		case CLIP_SHAPE_RECT:
			if (getValue(opv) & CLIP_RECT_TOP_AUTO) {
				rect.top_auto = true;
			} else {
				rect.top = *((css_fixed *) style->bytecode);
				advance_bytecode(style, sizeof(css_fixed));
				rect.tunit = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(uint32_t));
			}
			if (getValue(opv) & CLIP_RECT_RIGHT_AUTO) {
				rect.right_auto = true;
			} else {
				rect.right = *((css_fixed *) style->bytecode);
				advance_bytecode(style, sizeof(css_fixed));
				rect.runit = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(uint32_t));
			}
			if (getValue(opv) & CLIP_RECT_BOTTOM_AUTO) {
				rect.bottom_auto = true;
			} else {
				rect.bottom = *((css_fixed *) style->bytecode);
				advance_bytecode(style, sizeof(css_fixed));
				rect.bunit = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(uint32_t));
			}
			if (getValue(opv) & CLIP_RECT_LEFT_AUTO) {
				rect.left_auto = true;
			} else {
				rect.left = *((css_fixed *) style->bytecode);
				advance_bytecode(style, sizeof(css_fixed));
				rect.lunit = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(uint32_t));
			}
			break;
		case CLIP_AUTO:
			value = CSS_CLIP_AUTO;
			break;
		}
	}

	rect.tunit = to_css_unit(rect.tunit);
	rect.runit = to_css_unit(rect.runit);
	rect.bunit = to_css_unit(rect.bunit);
	rect.lunit = to_css_unit(rect.lunit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_clip(state->result, value, &rect);
	}

	return CSS_OK;
}

css_error set_clip_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_clip(style, hint->status, hint->data.clip);
}

css_error initial_clip(css_select_state *state)
{
	css_computed_clip_rect rect = { 0, 0, 0, 0, 
			CSS_UNIT_PX, CSS_UNIT_PX, CSS_UNIT_PX, CSS_UNIT_PX,
			false, false, false, false };

	return set_clip(state->result, CSS_CLIP_AUTO, &rect);
}

css_error compose_clip(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_computed_clip_rect rect = { 0, 0, 0, 0, 
			CSS_UNIT_PX, CSS_UNIT_PX, CSS_UNIT_PX, CSS_UNIT_PX,
			false, false, false, false };
	uint8_t type = get_clip(child, &rect);

	if ((child->uncommon == NULL && parent->uncommon != NULL) || 
			type == CSS_CLIP_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) || 
				type == CSS_CLIP_INHERIT) {
			type = get_clip(parent, &rect);
		}

		return set_clip(result, type, &rect);
	}

	return CSS_OK;
}

uint32_t destroy_clip(void *bytecode)
{
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bool has_rect = value & CLIP_SHAPE_RECT;
	int nonautos = 0;
	
	if (has_rect) {
		if ((value & CLIP_RECT_TOP_AUTO) == 0)
			nonautos += 1;
		if ((value & CLIP_RECT_RIGHT_AUTO) == 0)
			nonautos += 1;
		if ((value & CLIP_RECT_BOTTOM_AUTO) == 0)
			nonautos += 1;
		if ((value & CLIP_RECT_LEFT_AUTO) == 0)
			nonautos += 1;
	}
	
	return sizeof(uint32_t) + ((sizeof(css_fixed) + sizeof(uint32_t)) * nonautos);
}

css_error cascade_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_COLOR_INHERIT;
	css_color color = 0;

	if (isInherit(opv) == false) {
		value = CSS_COLOR_COLOR;
		color = *((css_color *) style->bytecode);
		advance_bytecode(style, sizeof(color));
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_color(state->result, value, color);
	}

	return CSS_OK;
}

css_error set_color_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	return set_color(style, hint->status, hint->data.color);
}

css_error initial_color(css_select_state *state)
{
	css_hint hint;
	css_error error;

	error = state->handler->ua_default_for_property(state->pw, 
			CSS_PROP_COLOR, &hint);
	if (error != CSS_OK)
		return error;

	return set_color_from_hint(&hint, state->result);
}

css_error compose_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_color(child, &color);

	if (type == CSS_COLOR_INHERIT) {
		type = get_color(parent, &color);
	}

	return set_color(result, type, color);
}

uint32_t destroy_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_content(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_CONTENT_INHERIT;
	css_computed_content_item *content = NULL;
	uint32_t n_contents = 0;

	if (isInherit(opv) == false) {
		uint32_t v = getValue(opv);

		if (v == CONTENT_NORMAL) {
			value = CSS_CONTENT_NORMAL;
		} else if (v == CONTENT_NONE) {
			value = CSS_CONTENT_NONE;
		} else {
			value = CSS_CONTENT_SET;
			
			while (v != CONTENT_NORMAL) {
				lwc_string *he = *((lwc_string **) style->bytecode);
				css_computed_content_item *temp;
				
				temp = state->result->alloc(content,
						(n_contents + 1) *
						sizeof(css_computed_content_item),
						state->result->pw);
				if (temp == NULL) {
					if (content != NULL) {
						state->result->alloc(content,
							0, state->result->pw);
					}
					return CSS_NOMEM;
				}

				content = temp;

				switch (v & 0xff) {
				case CONTENT_COUNTER:
					advance_bytecode(style, sizeof(he));

					content[n_contents].type =
						CSS_COMPUTED_CONTENT_COUNTER;
					content[n_contents].data.counter.name = he;
					content[n_contents].data.counter.style = v >> CONTENT_COUNTER_STYLE_SHIFT;
					break;
				case CONTENT_COUNTERS:
				{
					lwc_string *sep;
	
					advance_bytecode(style, sizeof(he));

					sep = *((lwc_string **) 
							style->bytecode);
					advance_bytecode(style, sizeof(sep));

					content[n_contents].type =
						CSS_COMPUTED_CONTENT_COUNTERS;
					content[n_contents].data.counters.name = he;
					content[n_contents].data.counters.sep = sep;
					content[n_contents].data.counters.style = v >> CONTENT_COUNTERS_STYLE_SHIFT;
				}
					break;
				case CONTENT_URI:
					advance_bytecode(style, sizeof(he));

					content[n_contents].type =
						CSS_COMPUTED_CONTENT_URI;
					content[n_contents].data.uri = he;
					break;
				case CONTENT_ATTR:
					advance_bytecode(style, sizeof(he));

					content[n_contents].type =
						CSS_COMPUTED_CONTENT_ATTR;
					content[n_contents].data.attr = he;
					break;
				case CONTENT_STRING:
					advance_bytecode(style, sizeof(he));

					content[n_contents].type =
						CSS_COMPUTED_CONTENT_STRING;
					content[n_contents].data.string = he;
					break;
				case CONTENT_OPEN_QUOTE:
					content[n_contents].type =
						CSS_COMPUTED_CONTENT_OPEN_QUOTE;
					break;
				case CONTENT_CLOSE_QUOTE:
					content[n_contents].type =
						CSS_COMPUTED_CONTENT_CLOSE_QUOTE;
					break;
				case CONTENT_NO_OPEN_QUOTE:
					content[n_contents].type =
						CSS_COMPUTED_CONTENT_NO_OPEN_QUOTE;
					break;
				case CONTENT_NO_CLOSE_QUOTE:
					content[n_contents].type =
						CSS_COMPUTED_CONTENT_NO_CLOSE_QUOTE;
					break;
				}

				n_contents++;

				v = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(v));
			}
		}
	}

	/* If we have some content, terminate the array with a blank entry */
	if (n_contents > 0) {
		css_computed_content_item *temp;

		temp = state->result->alloc(content,
				(n_contents + 1) * sizeof(css_computed_content_item),
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(content, 0, state->result->pw);
			return CSS_NOMEM;
		}

		content = temp;

		content[n_contents].type = CSS_COMPUTED_CONTENT_NONE;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		css_error error;

		error = set_content(state->result, value, content);
		if (error != CSS_OK && content != NULL)
			state->result->alloc(content, 0, state->result->pw);

		return error;
	} else if (content != NULL) {
		state->result->alloc(content, 0, state->result->pw);
	}

	return CSS_OK;
}

css_error set_content_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	css_computed_content_item *item;
	css_error error;

	error = set_content(style, hint->status, hint->data.content);

	for (item = hint->data.content; item != NULL &&
			item->type != CSS_COMPUTED_CONTENT_NONE;
			item++) {
		switch (item->type) {
		case CSS_COMPUTED_CONTENT_STRING:
			lwc_string_unref(item->data.string);
			break;
		case CSS_COMPUTED_CONTENT_URI:
			lwc_string_unref(item->data.uri);
			break;
		case CSS_COMPUTED_CONTENT_COUNTER:
			lwc_string_unref(item->data.counter.name);
			break;
		case CSS_COMPUTED_CONTENT_COUNTERS:
			lwc_string_unref(item->data.counters.name);
			lwc_string_unref(item->data.counters.sep);
			break;
		case CSS_COMPUTED_CONTENT_ATTR:
			lwc_string_unref(item->data.attr);
			break;
		default:
			break;
		}
	}

	if (error != CSS_OK && hint->data.content != NULL)
		style->alloc(hint->data.content, 0, style->pw);

	return error;
}

css_error initial_content(css_select_state *state)
{
	return set_content(state->result, CSS_CONTENT_NORMAL, NULL);
}

css_error compose_content(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	const css_computed_content_item *items = NULL;
	uint8_t type = get_content(child, &items);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type == CSS_CONTENT_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		size_t n_items = 0;
		css_computed_content_item *copy = NULL;

		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type == CSS_CONTENT_INHERIT) {
			type = get_content(parent, &items);
		}

		if (type == CSS_CONTENT_SET) {
			const css_computed_content_item *i;

			for (i = items; i->type != CSS_COMPUTED_CONTENT_NONE; 
					i++)
				n_items++;

			copy = result->alloc(NULL, (n_items + 1) * 
					sizeof(css_computed_content_item),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, items, (n_items + 1) * 
					sizeof(css_computed_content_item));
		}

		error = set_content(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_content(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	if (value == CONTENT_NONE || value == CONTENT_NORMAL)
		return sizeof(uint32_t);
	
	while (value != 0) {
		switch (value & 0xff) {
		case CONTENT_COUNTERS: {
			lwc_string *str = *(lwc_string **)bytecode;
			lwc_string_unref(str);
			consumed += sizeof(lwc_string*);
			bytecode = (uint8_t*)bytecode + sizeof(lwc_string *);
		}
		case CONTENT_STRING:
		case CONTENT_URI:
		case CONTENT_COUNTER:
		case CONTENT_ATTR: {
			lwc_string *str = *(lwc_string **)bytecode;
			lwc_string_unref(str);
			consumed += sizeof(lwc_string*);
			bytecode = (uint8_t*)bytecode + sizeof(lwc_string *);
		}
		}
		consumed += sizeof(uint32_t);
		value = *((uint32_t*)bytecode);
		bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	}
	
	return consumed;
}

css_error cascade_counter_increment(uint32_t opv, css_style *style, 
		css_select_state *state)
{	
	return cascade_counter_increment_reset(opv, style, state, 
			set_counter_increment);
}

css_error set_counter_increment_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	css_computed_counter *item;
	css_error error;

	error = set_counter_increment(style, hint->status, hint->data.counter);

	if (hint->status == CSS_COUNTER_INCREMENT_NAMED &&
			hint->data.counter != NULL) {
		for (item = hint->data.counter; item->name != NULL; item++) {
			lwc_string_unref(item->name);
		}
	}

	if (error != CSS_OK && hint->data.counter != NULL)
		style->alloc(hint->data.counter, 0, style->pw);

	return error;
}

css_error initial_counter_increment(css_select_state *state)
{
	return set_counter_increment(state->result, 
			CSS_COUNTER_INCREMENT_NONE, NULL);
}

css_error compose_counter_increment(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	const css_computed_counter *items = NULL;
	uint8_t type = get_counter_increment(child, &items);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type ==	CSS_COUNTER_INCREMENT_INHERIT || 
			(child->uncommon != NULL && result != child)) {
		size_t n_items = 0;
		css_computed_counter *copy = NULL;

		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type ==	CSS_COUNTER_INCREMENT_INHERIT) {
			type = get_counter_increment(parent, &items);
		}

		if (type == CSS_COUNTER_INCREMENT_NAMED && items != NULL) {
			const css_computed_counter *i;

			for (i = items; i->name != NULL; i++)
				n_items++;

			copy = result->alloc(NULL, (n_items + 1) * 
					sizeof(css_computed_counter),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, items, (n_items + 1) * 
					sizeof(css_computed_counter));
		}

		error = set_counter_increment(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_counter_increment(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	if (value == COUNTER_INCREMENT_NAMED) {
		while (value != COUNTER_INCREMENT_NONE) {
			lwc_string *str = *((lwc_string **)bytecode);
			consumed += sizeof(lwc_string*) + sizeof(css_fixed);
			bytecode = ((uint8_t*)bytecode) + sizeof(lwc_string*) + sizeof(css_fixed);
			lwc_string_unref(str);
			
			consumed += sizeof(uint32_t);
			value = *((uint32_t*)bytecode);
			bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
		}
	}
	
	return consumed;
}

css_error cascade_counter_reset(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_counter_increment_reset(opv, style, state,
			set_counter_reset);
}

css_error set_counter_reset_from_hint(const css_hint *hint, 
		css_computed_style *style)
{
	css_computed_counter *item;
	css_error error;

	error = set_counter_reset(style, hint->status, hint->data.counter);

	if (hint->status == CSS_COUNTER_RESET_NAMED &&
			hint->data.counter != NULL) {
		for (item = hint->data.counter; item->name != NULL; item++) {
			lwc_string_unref(item->name);
		}
	}

	if (error != CSS_OK && hint->data.counter != NULL)
		style->alloc(hint->data.counter, 0, style->pw);

	return error;
}

css_error initial_counter_reset(css_select_state *state)
{
	return set_counter_reset(state->result, CSS_COUNTER_RESET_NONE, NULL);
}

css_error compose_counter_reset(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	const css_computed_counter *items = NULL;
	uint8_t type = get_counter_reset(child, &items);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type ==	CSS_COUNTER_RESET_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		size_t n_items = 0;
		css_computed_counter *copy = NULL;

		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type ==	CSS_COUNTER_RESET_INHERIT) {
			type = get_counter_reset(parent, &items);
		}

		if (type == CSS_COUNTER_RESET_NAMED && items != NULL) {
			const css_computed_counter *i;

			for (i = items; i->name != NULL; i++)
				n_items++;

			copy = result->alloc(NULL, (n_items + 1) * 
					sizeof(css_computed_counter),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, items, (n_items + 1) * 
					sizeof(css_computed_counter));
		}

		error = set_counter_reset(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_counter_reset(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	if (value == COUNTER_INCREMENT_NAMED) {
		while (value != COUNTER_INCREMENT_NONE) {
			lwc_string *str = *((lwc_string **)bytecode);
			consumed += sizeof(lwc_string*) + sizeof(css_fixed);
			bytecode = ((uint8_t*)bytecode) + sizeof(lwc_string*) + sizeof(css_fixed);
			lwc_string_unref(str);
			
			consumed += sizeof(uint32_t);
			value = *((uint32_t*)bytecode);
			bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
		}
	}
	
	return consumed;
}

css_error cascade_cue_after(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo cue-after */
	return cascade_uri_none(opv, style, state, NULL);
}

css_error set_cue_after_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_cue_after(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_cue_after(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_cue_after(void *bytecode)
{
	return generic_destroy_uri(bytecode);
}

css_error cascade_cue_before(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo cue-before */
	return cascade_uri_none(opv, style, state, NULL);
}

css_error set_cue_before_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_cue_before(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_cue_before(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_cue_before(void *bytecode)
{
	return generic_destroy_uri(bytecode);
}

css_error cascade_cursor(uint32_t opv, css_style *style, 
		css_select_state *state)
{	
	uint16_t value = CSS_CURSOR_INHERIT;
	lwc_string **uris = NULL;
	uint32_t n_uris = 0;

	if (isInherit(opv) == false) {
		uint32_t v = getValue(opv);

		while (v == CURSOR_URI) {
			lwc_string *uri;
			lwc_string **temp;

			uri = *((lwc_string **) style->bytecode);
			advance_bytecode(style, sizeof(uri));

			temp = state->result->alloc(uris, 
					(n_uris + 1) * sizeof(lwc_string *), 
					state->result->pw);
			if (temp == NULL) {
				if (uris != NULL) {
					state->result->alloc(uris, 0,
							state->result->pw);
				}
				return CSS_NOMEM;
			}

			uris = temp;

			uris[n_uris] = uri;

			n_uris++;

			v = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(v));
		}

		switch (v) {
		case CURSOR_AUTO:
			value = CSS_CURSOR_AUTO;
			break;
		case CURSOR_CROSSHAIR:
			value = CSS_CURSOR_CROSSHAIR;
			break;
		case CURSOR_DEFAULT:
			value = CSS_CURSOR_DEFAULT;
			break;
		case CURSOR_POINTER:
			value = CSS_CURSOR_POINTER;
			break;
		case CURSOR_MOVE:
			value = CSS_CURSOR_MOVE;
			break;
		case CURSOR_E_RESIZE:
			value = CSS_CURSOR_E_RESIZE;
			break;
		case CURSOR_NE_RESIZE:
			value = CSS_CURSOR_NE_RESIZE;
			break;
		case CURSOR_NW_RESIZE:
			value = CSS_CURSOR_NW_RESIZE;
			break;
		case CURSOR_N_RESIZE:
			value = CSS_CURSOR_N_RESIZE;
			break;
		case CURSOR_SE_RESIZE:
			value = CSS_CURSOR_SE_RESIZE;
			break;
		case CURSOR_SW_RESIZE:
			value = CSS_CURSOR_SW_RESIZE;
			break;
		case CURSOR_S_RESIZE:
			value = CSS_CURSOR_S_RESIZE;
			break;
		case CURSOR_W_RESIZE:
			value = CSS_CURSOR_W_RESIZE;
			break;
		case CURSOR_TEXT:
			value = CSS_CURSOR_TEXT;
			break;
		case CURSOR_WAIT:
			value = CSS_CURSOR_WAIT;
			break;
		case CURSOR_HELP:
			value = CSS_CURSOR_HELP;
			break;
		case CURSOR_PROGRESS:
			value = CSS_CURSOR_PROGRESS;
			break;
		}
	}

	/* Terminate array with blank entry, if needed */
	if (n_uris > 0) {
		lwc_string **temp;

		temp = state->result->alloc(uris, 
				(n_uris + 1) * sizeof(lwc_string *), 
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(uris, 0, state->result->pw);
			return CSS_NOMEM;
		}

		uris = temp;

		uris[n_uris] = NULL;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		css_error error;

		error = set_cursor(state->result, value, uris);
		if (error != CSS_OK && n_uris > 0)
			state->result->alloc(uris, 0, state->result->pw);

		return error;
	} else {
		if (n_uris > 0)
			state->result->alloc(uris, 0, state->result->pw);
	}

	return CSS_OK;
}

css_error set_cursor_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	lwc_string **item;
	css_error error;

	error = set_cursor(style, hint->status, hint->data.strings);

	for (item = hint->data.strings; 
			item != NULL && (*item) != NULL; item++) {
		lwc_string_unref(*item);
	}

	if (error != CSS_OK && hint->data.strings != NULL)
		style->alloc(hint->data.strings, 0, style->pw);

	return error;
}

css_error initial_cursor(css_select_state *state)
{
	return set_cursor(state->result, CSS_CURSOR_AUTO, NULL);
}

css_error compose_cursor(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	lwc_string **urls = NULL;
	uint8_t type = get_cursor(child, &urls);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type == CSS_CURSOR_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		size_t n_urls = 0;
		lwc_string **copy = NULL;

		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type == CSS_CURSOR_INHERIT) {
			type = get_cursor(parent, &urls);
		}

		if (urls != NULL) {
			lwc_string **i;

			for (i = urls; (*i) != NULL; i++)
				n_urls++;

			copy = result->alloc(NULL, (n_urls + 1) * 
					sizeof(lwc_string *),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, urls, (n_urls + 1) * 
					sizeof(lwc_string *));
		}

		error = set_cursor(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_cursor(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	while (value == CURSOR_URI) {
		lwc_string *str = *((lwc_string **)bytecode);
		consumed += sizeof(lwc_string*);
		bytecode = ((uint8_t*)bytecode) + sizeof(lwc_string*);
		lwc_string_unref(str);
		
		consumed += sizeof(uint32_t);
		value = *((uint32_t*)bytecode);
		bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	}
	
	return consumed;
}

css_error cascade_direction(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_DIRECTION_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case DIRECTION_LTR:
			value = CSS_DIRECTION_LTR;
			break;
		case DIRECTION_RTL:
			value = CSS_DIRECTION_RTL;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_direction(state->result, value);
	}

	return CSS_OK;
}

css_error set_direction_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_direction(style, hint->status);
}

css_error initial_direction(css_select_state *state)
{
	return set_direction(state->result, CSS_DIRECTION_LTR);
}

css_error compose_direction(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_direction(child);

	if (type == CSS_DIRECTION_INHERIT) {
		type = get_direction(parent);
	}

	return set_direction(result, type);
}

uint32_t destroy_direction(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_display(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_DISPLAY_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case DISPLAY_INLINE:
			value = CSS_DISPLAY_INLINE;
			break;
		case DISPLAY_BLOCK:
			value = CSS_DISPLAY_BLOCK;
			break;
		case DISPLAY_LIST_ITEM:
			value = CSS_DISPLAY_LIST_ITEM;
			break;
		case DISPLAY_RUN_IN:
			value = CSS_DISPLAY_RUN_IN;
			break;
		case DISPLAY_INLINE_BLOCK:
			value = CSS_DISPLAY_INLINE_BLOCK;
			break;
		case DISPLAY_TABLE:
			value = CSS_DISPLAY_TABLE;
			break;
		case DISPLAY_INLINE_TABLE:
			value = CSS_DISPLAY_INLINE_TABLE;
			break;
		case DISPLAY_TABLE_ROW_GROUP:
			value = CSS_DISPLAY_TABLE_ROW_GROUP;
			break;
		case DISPLAY_TABLE_HEADER_GROUP:
			value = CSS_DISPLAY_TABLE_HEADER_GROUP;
			break;
		case DISPLAY_TABLE_FOOTER_GROUP:
			value = CSS_DISPLAY_TABLE_FOOTER_GROUP;
			break;
		case DISPLAY_TABLE_ROW:
			value = CSS_DISPLAY_TABLE_ROW;
			break;
		case DISPLAY_TABLE_COLUMN_GROUP:
			value = CSS_DISPLAY_TABLE_COLUMN_GROUP;
			break;
		case DISPLAY_TABLE_COLUMN:
			value = CSS_DISPLAY_TABLE_COLUMN;
			break;
		case DISPLAY_TABLE_CELL:
			value = CSS_DISPLAY_TABLE_CELL;
			break;
		case DISPLAY_TABLE_CAPTION:
			value = CSS_DISPLAY_TABLE_CAPTION;
			break;
		case DISPLAY_NONE:
			value = CSS_DISPLAY_NONE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_display(state->result, value);
	}

	return CSS_OK;
}

css_error set_display_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_display(style, hint->status);
}

css_error initial_display(css_select_state *state)
{
	return set_display(state->result, CSS_DISPLAY_INLINE);
}

css_error compose_display(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_display_static(child);

	if (type == CSS_DISPLAY_INHERIT) {
		type = get_display_static(parent);
	}

	return set_display(result, type);
}

uint32_t destroy_display(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_elevation(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	css_fixed val = 0;
	uint32_t unit = UNIT_DEG;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case ELEVATION_ANGLE: 
			value = 0;

			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));

			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case ELEVATION_BELOW:
		case ELEVATION_LEVEL:
		case ELEVATION_ABOVE:
		case ELEVATION_HIGHER:
		case ELEVATION_LOWER:
			/** \todo convert to public values */
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo set computed elevation */
	}

	return CSS_OK;
}

css_error set_elevation_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_elevation(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_elevation(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_elevation(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_empty_cells(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_EMPTY_CELLS_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case EMPTY_CELLS_SHOW:
			value = CSS_EMPTY_CELLS_SHOW;
			break;
		case EMPTY_CELLS_HIDE:
			value = CSS_EMPTY_CELLS_HIDE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_empty_cells(state->result, value);
	}

	return CSS_OK;
}

css_error set_empty_cells_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_empty_cells(style, hint->status);
}

css_error initial_empty_cells(css_select_state *state)
{
	return set_empty_cells(state->result, CSS_EMPTY_CELLS_SHOW);
}

css_error compose_empty_cells(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_empty_cells(child);

	if (type == CSS_EMPTY_CELLS_INHERIT) {
		type = get_empty_cells(parent);
	}

	return set_empty_cells(result, type);
}

uint32_t destroy_empty_cells(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_float(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FLOAT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FLOAT_LEFT:
			value = CSS_FLOAT_LEFT;
			break;
		case FLOAT_RIGHT:
			value = CSS_FLOAT_RIGHT;
			break;
		case FLOAT_NONE:
			value = CSS_FLOAT_NONE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_float(state->result, value);
	}

	return CSS_OK;
}

css_error set_float_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_float(style, hint->status);
}

css_error initial_float(css_select_state *state)
{
	return set_float(state->result, CSS_FLOAT_NONE);
}

css_error compose_float(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_float(child);

	if (type == CSS_FLOAT_INHERIT) {
		type = get_float(parent);
	}

	return set_float(result, type);
}

uint32_t destroy_float(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_font_family(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FONT_FAMILY_INHERIT;
	lwc_string **fonts = NULL;
	uint32_t n_fonts = 0;

	if (isInherit(opv) == false) {
		uint32_t v = getValue(opv);

		while (v != FONT_FAMILY_END) {
			lwc_string *font = NULL;
			lwc_string **temp;

			switch (v) {
			case FONT_FAMILY_STRING:
			case FONT_FAMILY_IDENT_LIST:
				font = *((lwc_string **) 
						style->bytecode);
				advance_bytecode(style, sizeof(font));
				break;
			case FONT_FAMILY_SERIF:
				if (value == CSS_FONT_FAMILY_INHERIT)
					value = CSS_FONT_FAMILY_SERIF;
				break;
			case FONT_FAMILY_SANS_SERIF:
				if (value == CSS_FONT_FAMILY_INHERIT)
					value = CSS_FONT_FAMILY_SANS_SERIF;
				break;
			case FONT_FAMILY_CURSIVE:
				if (value == CSS_FONT_FAMILY_INHERIT)
					value = CSS_FONT_FAMILY_CURSIVE;
				break;
			case FONT_FAMILY_FANTASY:
				if (value == CSS_FONT_FAMILY_INHERIT)
					value = CSS_FONT_FAMILY_FANTASY;
				break;
			case FONT_FAMILY_MONOSPACE:
				if (value == CSS_FONT_FAMILY_INHERIT)
					value = CSS_FONT_FAMILY_MONOSPACE;
				break;
			}

			/* Only use family-names which occur before the first
			 * generic-family. Any values which occur after the
			 * first generic-family are ignored. */
			/** \todo Do this at bytecode generation time? */
			if (value == CSS_FONT_FAMILY_INHERIT && font != NULL) {
				temp = state->result->alloc(fonts, 
					(n_fonts + 1) * sizeof(lwc_string *), 
					state->result->pw);
				if (temp == NULL) {
					if (fonts != NULL) {
						state->result->alloc(fonts, 0,
							state->result->pw);
					}
					return CSS_NOMEM;
				}

				fonts = temp;

				fonts[n_fonts] = font;

				n_fonts++;
			}

			v = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(v));
		}
	}

	/* Terminate array with blank entry, if needed */
	if (n_fonts > 0) {
		lwc_string **temp;

		temp = state->result->alloc(fonts, 
				(n_fonts + 1) * sizeof(lwc_string *), 
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(fonts, 0, state->result->pw);
			return CSS_NOMEM;
		}

		fonts = temp;

		fonts[n_fonts] = NULL;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		css_error error;

		error = set_font_family(state->result, value, fonts);
		if (error != CSS_OK && n_fonts > 0)
			state->result->alloc(fonts, 0, state->result->pw);

		return error;
	} else {
		if (n_fonts > 0)
			state->result->alloc(fonts, 0, state->result->pw);
	}

	return CSS_OK;
}

css_error set_font_family_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	lwc_string **item;
	css_error error;

	error = set_font_family(style, hint->status, hint->data.strings);

	for (item = hint->data.strings; 
			item != NULL && (*item) != NULL; item++) {
		lwc_string_unref(*item);
	}

	if (error != CSS_OK && hint->data.strings != NULL)
		style->alloc(hint->data.strings, 0, style->pw);

	return error;
}

css_error initial_font_family(css_select_state *state)
{
	css_hint hint;
	css_error error;

	error = state->handler->ua_default_for_property(state->pw,
			CSS_PROP_FONT_FAMILY, &hint);
	if (error != CSS_OK)
		return error;

	return set_font_family_from_hint(&hint, state->result);
}

css_error compose_font_family(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	lwc_string **urls = NULL;
	uint8_t type = get_font_family(child, &urls);

	if (type == CSS_FONT_FAMILY_INHERIT || result != child) {
		size_t n_urls = 0;
		lwc_string **copy = NULL;

		if (type == CSS_FONT_FAMILY_INHERIT)
			type = get_font_family(parent, &urls);

		if (urls != NULL) {
			lwc_string **i;

			for (i = urls; (*i) != NULL; i++)
				n_urls++;

			copy = result->alloc(NULL, (n_urls + 1) * 
					sizeof(lwc_string *),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, urls, (n_urls + 1) * 
					sizeof(lwc_string *));
		}

		error = set_font_family(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_font_family(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	while (value != FONT_FAMILY_END) {
		if (value == FONT_FAMILY_STRING || value == FONT_FAMILY_IDENT_LIST) {
			lwc_string *str = *((lwc_string **)bytecode);
			consumed += sizeof(lwc_string*);
			bytecode = ((uint8_t*)bytecode) + sizeof(lwc_string*);
			lwc_string_unref(str);
		}
		
		consumed += sizeof(uint32_t);
		value = *((uint32_t*)bytecode);
		bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	}
	
	return consumed;
}

css_error cascade_font_size(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FONT_SIZE_INHERIT;
	css_fixed size = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FONT_SIZE_DIMENSION: 
			value = CSS_FONT_SIZE_DIMENSION;

			size = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(size));

			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case FONT_SIZE_XX_SMALL:
			value = CSS_FONT_SIZE_XX_SMALL;
			break;
		case FONT_SIZE_X_SMALL:
			value = CSS_FONT_SIZE_X_SMALL;
			break;
		case FONT_SIZE_SMALL:
			value = CSS_FONT_SIZE_SMALL;
			break;
		case FONT_SIZE_MEDIUM:
			value = CSS_FONT_SIZE_MEDIUM;
			break;
		case FONT_SIZE_LARGE:
			value = CSS_FONT_SIZE_LARGE;
			break;
		case FONT_SIZE_X_LARGE:
			value = CSS_FONT_SIZE_X_LARGE;
			break;
		case FONT_SIZE_XX_LARGE:
			value = CSS_FONT_SIZE_XX_LARGE;
			break;
		case FONT_SIZE_LARGER:
			value = CSS_FONT_SIZE_LARGER;
			break;
		case FONT_SIZE_SMALLER:
			value = CSS_FONT_SIZE_SMALLER;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_font_size(state->result, value, size, unit);
	}

	return CSS_OK;
}

css_error set_font_size_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_font_size(style, hint->status, 
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_font_size(css_select_state *state)
{
	return set_font_size(state->result, CSS_FONT_SIZE_MEDIUM, 
			0, CSS_UNIT_PX);
}

css_error compose_font_size(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed size = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_font_size(child, &size, &unit);

	if (type == CSS_FONT_SIZE_INHERIT) {
		type = get_font_size(parent, &size, &unit);
	}

	return set_font_size(result, type, size, unit);
}

uint32_t destroy_font_size(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_font_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FONT_STYLE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FONT_STYLE_NORMAL:
			value = CSS_FONT_STYLE_NORMAL;
			break;
		case FONT_STYLE_ITALIC:
			value = CSS_FONT_STYLE_ITALIC;
			break;
		case FONT_STYLE_OBLIQUE:
			value = CSS_FONT_STYLE_OBLIQUE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_font_style(state->result, value);
	}

	return CSS_OK;
}

css_error set_font_style_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_font_style(style, hint->status);
}

css_error initial_font_style(css_select_state *state)
{
	return set_font_style(state->result, CSS_FONT_STYLE_NORMAL);
}

css_error compose_font_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_font_style(child);

	if (type == CSS_FONT_STYLE_INHERIT) {
		type= get_font_style(parent);
	}
	
	return set_font_style(result, type);
}

uint32_t destroy_font_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_font_variant(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FONT_VARIANT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FONT_VARIANT_NORMAL:
			value = CSS_FONT_VARIANT_NORMAL;
			break;
		case FONT_VARIANT_SMALL_CAPS:
			value = CSS_FONT_VARIANT_SMALL_CAPS;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_font_variant(state->result, value);
	}

	return CSS_OK;
}

css_error set_font_variant_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_font_variant(style, hint->status);
}

css_error initial_font_variant(css_select_state *state)
{
	return set_font_variant(state->result, CSS_FONT_VARIANT_NORMAL);
}

css_error compose_font_variant(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_font_variant(child);

	if (type == CSS_FONT_VARIANT_INHERIT) {
		type = get_font_variant(parent);
	}

	return set_font_variant(result, type);
}

uint32_t destroy_font_variant(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_font_weight(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_FONT_WEIGHT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case FONT_WEIGHT_NORMAL:
			value = CSS_FONT_WEIGHT_NORMAL;
			break;
		case FONT_WEIGHT_BOLD:
			value = CSS_FONT_WEIGHT_BOLD;
			break;
		case FONT_WEIGHT_BOLDER:
			value = CSS_FONT_WEIGHT_BOLDER;
			break;
		case FONT_WEIGHT_LIGHTER:
			value = CSS_FONT_WEIGHT_LIGHTER;
			break;
		case FONT_WEIGHT_100:
			value = CSS_FONT_WEIGHT_100;
			break;
		case FONT_WEIGHT_200:
			value = CSS_FONT_WEIGHT_200;
			break;
		case FONT_WEIGHT_300:
			value = CSS_FONT_WEIGHT_300;
			break;
		case FONT_WEIGHT_400:
			value = CSS_FONT_WEIGHT_400;
			break;
		case FONT_WEIGHT_500:
			value = CSS_FONT_WEIGHT_500;
			break;
		case FONT_WEIGHT_600:
			value = CSS_FONT_WEIGHT_600;
			break;
		case FONT_WEIGHT_700:
			value = CSS_FONT_WEIGHT_700;
			break;
		case FONT_WEIGHT_800:
			value = CSS_FONT_WEIGHT_800;
			break;
		case FONT_WEIGHT_900:
			value = CSS_FONT_WEIGHT_900;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_font_weight(state->result, value);
	}

	return CSS_OK;
}

css_error set_font_weight_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_font_weight(style, hint->status);
}

css_error initial_font_weight(css_select_state *state)
{
	return set_font_weight(state->result, CSS_FONT_WEIGHT_NORMAL);
}

css_error compose_font_weight(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_font_weight(child);

	if (type == CSS_FONT_WEIGHT_INHERIT) {
		type = get_font_weight(parent);
	}

	return set_font_weight(result, type);
}

uint32_t destroy_font_weight(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_height(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_height);
}

css_error set_height_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_height(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_height(css_select_state *state)
{
	return set_height(state->result, CSS_HEIGHT_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_height(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_height(child, &length, &unit);

	if (type == CSS_HEIGHT_INHERIT) {
		type = get_height(parent, &length, &unit);
	}

	return set_height(result, type, length, unit);
}

uint32_t destroy_height(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_left(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_left);
}

css_error set_left_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_left(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_left(css_select_state *state)
{
	return set_left(state->result, CSS_LEFT_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_left(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_left(child, &length, &unit);

	if (type == CSS_LEFT_INHERIT) {
		type = get_left(parent, &length, &unit);
	}

	return set_left(result, type, length, unit);
}

uint32_t destroy_left(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_letter_spacing(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_normal(opv, style, state, set_letter_spacing);
}

css_error set_letter_spacing_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_letter_spacing(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_letter_spacing(css_select_state *state)
{
	return set_letter_spacing(state->result, CSS_LETTER_SPACING_NORMAL, 
			0, CSS_UNIT_PX);
}

css_error compose_letter_spacing(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_letter_spacing(child, &length, &unit);

	if ((child->uncommon == NULL && parent->uncommon != NULL) || 
			type == CSS_LETTER_SPACING_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) || 
				type == CSS_LETTER_SPACING_INHERIT) {
			type = get_letter_spacing(parent, &length, &unit);
		}

		return set_letter_spacing(result, type, length, unit);
	}

	return CSS_OK;
}

uint32_t destroy_letter_spacing(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_line_height(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_LINE_HEIGHT_INHERIT;
	css_fixed val = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case LINE_HEIGHT_NUMBER:
			value = CSS_LINE_HEIGHT_NUMBER;
			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));
			break;
		case LINE_HEIGHT_DIMENSION:
			value = CSS_LINE_HEIGHT_DIMENSION;
			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case LINE_HEIGHT_NORMAL:
			value = CSS_LINE_HEIGHT_NORMAL;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_line_height(state->result, value, val, unit);
	}

	return CSS_OK;
}

css_error set_line_height_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_line_height(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_line_height(css_select_state *state)
{
	return set_line_height(state->result, CSS_LINE_HEIGHT_NORMAL, 
			0, CSS_UNIT_PX);
}

css_error compose_line_height(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_line_height(child, &length, &unit);

	if (type == CSS_LINE_HEIGHT_INHERIT) {
		type = get_line_height(parent, &length, &unit);
	}

	return set_line_height(result, type, length, unit);
}

uint32_t destroy_line_height(void *bytecode)
{
	uint32_t value = getValue(*((uint32_t*)bytecode));
	if (value == LINE_HEIGHT_NUMBER)
		return generic_destroy_number(bytecode);
	else
		return generic_destroy_length(bytecode);
}

css_error cascade_list_style_image(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_uri_none(opv, style, state, set_list_style_image);
}

css_error set_list_style_image_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_list_style_image(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error initial_list_style_image(css_select_state *state)
{
	return set_list_style_image(state->result, 
			CSS_LIST_STYLE_IMAGE_NONE, NULL);
}

css_error compose_list_style_image(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_list_style_image(child, &url);

	if (type == CSS_LIST_STYLE_IMAGE_INHERIT) {
		type = get_list_style_image(parent, &url);
	}

	return set_list_style_image(result, type, url);
}

uint32_t destroy_list_style_image(void *bytecode)
{
	return generic_destroy_uri(bytecode);
}

css_error cascade_list_style_position(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_LIST_STYLE_POSITION_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case LIST_STYLE_POSITION_INSIDE:
			value = CSS_LIST_STYLE_POSITION_INSIDE;
			break;
		case LIST_STYLE_POSITION_OUTSIDE:
			value = CSS_LIST_STYLE_POSITION_OUTSIDE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_list_style_position(state->result, value);
	}

	return CSS_OK;
}

css_error set_list_style_position_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_list_style_position(style, hint->status);
}

css_error initial_list_style_position(css_select_state *state)
{
	return set_list_style_position(state->result, 
			CSS_LIST_STYLE_POSITION_OUTSIDE);
}

css_error compose_list_style_position(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_list_style_position(child);

	if (type == CSS_LIST_STYLE_POSITION_INHERIT) {
		type = get_list_style_position(parent);
	}

	return set_list_style_position(result, type);
}

uint32_t destroy_list_style_position(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_list_style_type(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_LIST_STYLE_TYPE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case LIST_STYLE_TYPE_DISC:
			value = CSS_LIST_STYLE_TYPE_DISC;
			break;
		case LIST_STYLE_TYPE_CIRCLE:
			value = CSS_LIST_STYLE_TYPE_CIRCLE;
			break;
		case LIST_STYLE_TYPE_SQUARE:
			value = CSS_LIST_STYLE_TYPE_SQUARE;
			break;
		case LIST_STYLE_TYPE_DECIMAL:
			value = CSS_LIST_STYLE_TYPE_DECIMAL;
			break;
		case LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
			value = CSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO;
			break;
		case LIST_STYLE_TYPE_LOWER_ROMAN:
			value = CSS_LIST_STYLE_TYPE_LOWER_ROMAN;
			break;
		case LIST_STYLE_TYPE_UPPER_ROMAN:
			value = CSS_LIST_STYLE_TYPE_UPPER_ROMAN;
			break;
		case LIST_STYLE_TYPE_LOWER_GREEK:
			value = CSS_LIST_STYLE_TYPE_LOWER_GREEK;
			break;
		case LIST_STYLE_TYPE_LOWER_LATIN:
			value = CSS_LIST_STYLE_TYPE_LOWER_LATIN;
			break;
		case LIST_STYLE_TYPE_UPPER_LATIN:
			value = CSS_LIST_STYLE_TYPE_UPPER_LATIN;
			break;
		case LIST_STYLE_TYPE_ARMENIAN:
			value = CSS_LIST_STYLE_TYPE_ARMENIAN;
			break;
		case LIST_STYLE_TYPE_GEORGIAN:
			value = CSS_LIST_STYLE_TYPE_GEORGIAN;
			break;
		case LIST_STYLE_TYPE_LOWER_ALPHA:
			value = CSS_LIST_STYLE_TYPE_LOWER_ALPHA;
			break;
		case LIST_STYLE_TYPE_UPPER_ALPHA:
			value = CSS_LIST_STYLE_TYPE_UPPER_ALPHA;
			break;
		case LIST_STYLE_TYPE_NONE:
			value = CSS_LIST_STYLE_TYPE_NONE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_list_style_type(state->result, value);
	}

	return CSS_OK;
}

css_error set_list_style_type_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_list_style_type(style, hint->status);
}

css_error initial_list_style_type(css_select_state *state)
{
	return set_list_style_type(state->result, CSS_LIST_STYLE_TYPE_DISC);
}

css_error compose_list_style_type(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_list_style_type(child);

	if (type == CSS_LIST_STYLE_TYPE_INHERIT) {
		type = get_list_style_type(parent);
	}

	return set_list_style_type(result, type);
}

uint32_t destroy_list_style_type(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_margin_top(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_margin_top);
}

css_error set_margin_top_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_margin_top(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_margin_top(css_select_state *state)
{
	return set_margin_top(state->result, CSS_MARGIN_SET, 0, CSS_UNIT_PX);
}

css_error compose_margin_top(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_margin_top(child, &length, &unit);

	if (type == CSS_MARGIN_INHERIT) {
		type = get_margin_top(parent, &length, &unit);
	}

	return set_margin_top(result, type, length, unit);
}

uint32_t destroy_margin_top(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_margin_right(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_margin_right);
}

css_error set_margin_right_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_margin_right(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_margin_right(css_select_state *state)
{
	return set_margin_right(state->result, CSS_MARGIN_SET, 0, CSS_UNIT_PX);
}

css_error compose_margin_right(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_margin_right(child, &length, &unit);

	if (type == CSS_MARGIN_INHERIT) {
		type = get_margin_right(parent, &length, &unit);
	}

	return set_margin_right(result, type, length, unit);
}

uint32_t destroy_margin_right(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_margin_bottom(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_margin_bottom);
}

css_error set_margin_bottom_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_margin_bottom(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_margin_bottom(css_select_state *state)
{
	return set_margin_bottom(state->result, CSS_MARGIN_SET, 0, CSS_UNIT_PX);
}

css_error compose_margin_bottom(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_margin_bottom(child, &length, &unit);

	if (type == CSS_MARGIN_INHERIT) {
		type = get_margin_bottom(parent, &length, &unit);
	}

	return set_margin_bottom(result, type, length, unit);
}

uint32_t destroy_margin_bottom(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_margin_left(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_margin_left);
}

css_error set_margin_left_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_margin_left(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_margin_left(css_select_state *state)
{
	return set_margin_left(state->result, CSS_MARGIN_SET, 0, CSS_UNIT_PX);
}

css_error compose_margin_left(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_margin_left(child, &length, &unit);

	if (type == CSS_MARGIN_INHERIT) {
		type = get_margin_left(parent, &length, &unit);
	}

	return set_margin_left(result, type, length, unit);
}

uint32_t destroy_margin_left(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_max_height(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_none(opv, style, state, set_max_height);
}

css_error set_max_height_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_max_height(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_max_height(css_select_state *state)
{
	return set_max_height(state->result, CSS_MAX_HEIGHT_NONE, 
			0, CSS_UNIT_PX);
}

css_error compose_max_height(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_max_height(child, &length, &unit);

	if (type == CSS_MAX_HEIGHT_INHERIT) {
		type = get_max_height(parent, &length, &unit);
	}

	return set_max_height(result, type, length, unit);
}

uint32_t destroy_max_height(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_max_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_none(opv, style, state, set_max_width);;
}

css_error set_max_width_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_max_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_max_width(css_select_state *state)
{
	return set_max_width(state->result, CSS_MAX_WIDTH_NONE, 0, CSS_UNIT_PX);
}

css_error compose_max_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_max_width(child, &length, &unit);

	if (type == CSS_MAX_WIDTH_INHERIT) {
		type = get_max_width(parent, &length, &unit);
	}

	return set_max_width(result, type, length, unit);
}

uint32_t destroy_max_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_min_height(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_min_height);
}

css_error set_min_height_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_min_height(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_min_height(css_select_state *state)
{
	return set_min_height(state->result, CSS_MIN_HEIGHT_SET, 
			0, CSS_UNIT_PX);
}

css_error compose_min_height(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_min_height(child, &length, &unit);

	if (type == CSS_MIN_HEIGHT_INHERIT) {
		type = get_min_height(parent, &length, &unit);
	}

	return set_min_height(result, type, length, unit);
}

uint32_t destroy_min_height(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_min_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_min_width);
}

css_error set_min_width_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_min_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_min_width(css_select_state *state)
{
	return set_min_width(state->result, CSS_MIN_WIDTH_SET, 0, CSS_UNIT_PX);
}

css_error compose_min_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_min_width(child, &length, &unit);

	if (type == CSS_MIN_WIDTH_INHERIT) {
		type = get_min_width(parent, &length, &unit);
	}

	return set_min_width(result, type, length, unit);
}

uint32_t destroy_min_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_orphans(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo orphans */
	return cascade_number(opv, style, state, NULL);
}

css_error set_orphans_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_orphans(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_orphans(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_orphans(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_outline_color(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_OUTLINE_COLOR_INHERIT;
	css_color color = 0;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case OUTLINE_COLOR_SET:
			value = CSS_OUTLINE_COLOR_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
			break;
		case OUTLINE_COLOR_INVERT:
			value = CSS_OUTLINE_COLOR_INVERT;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_outline_color(state->result, value, color);
	}

	return CSS_OK;
}

css_error set_outline_color_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_outline_color(style, hint->status, hint->data.color);
}

css_error initial_outline_color(css_select_state *state)
{
	return set_outline_color(state->result, CSS_OUTLINE_COLOR_INVERT, 0);
}

css_error compose_outline_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color = 0;
	uint8_t type = get_outline_color(child, &color);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type == CSS_OUTLINE_COLOR_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type == CSS_OUTLINE_COLOR_INHERIT) {
			type = get_outline_color(parent, &color);
		}

		return set_outline_color(result, type, color);
	}

	return CSS_OK;
}

uint32_t destroy_outline_color(void *bytecode)
{
	return generic_destroy_color(bytecode);
}

css_error cascade_outline_style(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_style(opv, style, state, set_outline_style);
}

css_error set_outline_style_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_outline_style(style, hint->status);
}

css_error initial_outline_style(css_select_state *state)
{
	return set_outline_style(state->result, CSS_OUTLINE_STYLE_NONE);
}

css_error compose_outline_style(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_outline_style(child);

	if (type == CSS_OUTLINE_STYLE_INHERIT) {
		type = get_outline_style(parent);
	}

	return set_outline_style(result, type);
}

uint32_t destroy_outline_style(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_outline_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_border_width(opv, style, state, set_outline_width);
}

css_error set_outline_width_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_outline_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_outline_width(css_select_state *state)
{
	return set_outline_width(state->result, CSS_OUTLINE_WIDTH_MEDIUM,
			0, CSS_UNIT_PX);
}

css_error compose_outline_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_outline_width(child, &length, &unit);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type == CSS_OUTLINE_WIDTH_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type == CSS_OUTLINE_WIDTH_INHERIT) {
			type = get_outline_width(parent, &length, &unit);
		}

		return set_outline_width(result, type, length, unit);
	}

	return CSS_OK;
}

uint32_t destroy_outline_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_overflow(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_OVERFLOW_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case OVERFLOW_VISIBLE:
			value = CSS_OVERFLOW_VISIBLE;
			break;
		case OVERFLOW_HIDDEN:
			value = CSS_OVERFLOW_HIDDEN;
			break;
		case OVERFLOW_SCROLL:
			value = CSS_OVERFLOW_SCROLL;
			break;
		case OVERFLOW_AUTO:
			value = CSS_OVERFLOW_AUTO;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_overflow(state->result, value);
	}

	return CSS_OK;
}

css_error set_overflow_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_overflow(style, hint->status);
}

css_error initial_overflow(css_select_state *state)
{
	return set_overflow(state->result, CSS_OVERFLOW_VISIBLE);
}

css_error compose_overflow(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_overflow(child);

	if (type == CSS_OVERFLOW_INHERIT) {
		type = get_overflow(parent);
	}

	return set_overflow(result, type);
}

uint32_t destroy_overflow(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_padding_top(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_padding_top);
}

css_error set_padding_top_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_padding_top(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_padding_top(css_select_state *state)
{
	return set_padding_top(state->result, CSS_PADDING_SET, 0, CSS_UNIT_PX);
}

css_error compose_padding_top(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_padding_top(child, &length, &unit);

	if (type == CSS_PADDING_INHERIT) {
		type = get_padding_top(parent, &length, &unit);
	}

	return set_padding_top(result, type, length, unit);
}

uint32_t destroy_padding_top(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_padding_right(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_padding_right);
}

css_error set_padding_right_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_padding_right(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_padding_right(css_select_state *state)
{
	return set_padding_right(state->result, CSS_PADDING_SET, 
			0, CSS_UNIT_PX);
}

css_error compose_padding_right(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_padding_right(child, &length, &unit);

	if (type == CSS_PADDING_INHERIT) {
		type = get_padding_right(parent, &length, &unit);
	}

	return set_padding_right(result, type, length, unit);
}

uint32_t destroy_padding_right(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_padding_bottom(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_padding_bottom);
}

css_error set_padding_bottom_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_padding_bottom(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_padding_bottom(css_select_state *state)
{
	return set_padding_bottom(state->result, CSS_PADDING_SET, 
			0, CSS_UNIT_PX);
}

css_error compose_padding_bottom(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_padding_bottom(child, &length, &unit);

	if (type == CSS_PADDING_INHERIT) {
		type = get_padding_bottom(parent, &length, &unit);
	}

	return set_padding_bottom(result, type, length, unit);
}

uint32_t destroy_padding_bottom(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_padding_left(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_padding_left);
}

css_error set_padding_left_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_padding_left(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_padding_left(css_select_state *state)
{
	return set_padding_left(state->result, CSS_PADDING_SET, 0, CSS_UNIT_PX);
}

css_error compose_padding_left(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_padding_left(child, &length, &unit);

	if (type == CSS_PADDING_INHERIT) {
		type = get_padding_left(parent, &length, &unit);
	}

	return set_padding_left(result, type, length, unit);
}

uint32_t destroy_padding_left(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_page_break_after(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo page-break-after */
	return cascade_page_break_after_before(opv, style, state, NULL);
}

css_error set_page_break_after_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_page_break_after(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_page_break_after(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_page_break_after(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_page_break_before(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo page-break-before */
	return cascade_page_break_after_before(opv, style, state, NULL);
}

css_error set_page_break_before_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_page_break_before(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_page_break_before(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_page_break_before(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_page_break_inside(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case PAGE_BREAK_INSIDE_AUTO:
		case PAGE_BREAK_INSIDE_AVOID:
			/** \todo convert to public values */
			value = 0;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo page-break-inside */
	}

	return CSS_OK;
}

css_error set_page_break_inside_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_page_break_inside(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_page_break_inside(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_page_break_inside(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_pause_after(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo pause-after */
	return cascade_length(opv, style, state, NULL);
}

css_error set_pause_after_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_pause_after(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_pause_after(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_pause_after(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_pause_before(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo pause-before */
	return cascade_length(opv, style, state, NULL);
}

css_error set_pause_before_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_pause_before(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_pause_before(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_pause_before(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_pitch_range(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo pitch-range */
	return cascade_number(opv, style, state, NULL);
}

css_error set_pitch_range_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_pitch_range(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_pitch_range(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_pitch_range(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_pitch(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	css_fixed freq = 0;
	uint32_t unit = UNIT_HZ;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case PITCH_FREQUENCY:
			value = 0;

			freq = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(freq));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case PITCH_X_LOW:
		case PITCH_LOW:
		case PITCH_MEDIUM:
		case PITCH_HIGH:
		case PITCH_X_HIGH:
			/** \todo convert to public values */
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo pitch */
	}

	return CSS_OK;
}

css_error set_pitch_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_pitch(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_pitch(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_pitch(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_play_during(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	lwc_string *uri = NULL;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case PLAY_DURING_URI:
			value = 0;

			uri = *((lwc_string **) style->bytecode);
			advance_bytecode(style, sizeof(uri));
			break;
		case PLAY_DURING_AUTO:
		case PLAY_DURING_NONE:
			/** \todo convert to public values */
			break;
		}

		/** \todo mix & repeat */
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo play-during */
	}

	return CSS_OK;
}

css_error set_play_during_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_play_during(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_play_during(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_play_during(void *bytecode)
{
	return generic_destroy_uri(bytecode);
}

css_error cascade_position(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_POSITION_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case POSITION_STATIC:
			value = CSS_POSITION_STATIC;
			break;
		case POSITION_RELATIVE:
			value = CSS_POSITION_RELATIVE;
			break;
		case POSITION_ABSOLUTE:
			value = CSS_POSITION_ABSOLUTE;
			break;
		case POSITION_FIXED:
			value = CSS_POSITION_FIXED;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_position(state->result, value);
	}

	return CSS_OK;
}

css_error set_position_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_position(style, hint->status);
}

css_error initial_position(css_select_state *state)
{
	return set_position(state->result, CSS_POSITION_STATIC);
}

css_error compose_position(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_position(child);

	if (type == CSS_POSITION_INHERIT) {
		type = get_position(parent);
	}

	return set_position(result, type);
}

uint32_t destroy_position(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_quotes(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_QUOTES_INHERIT;
	lwc_string **quotes = NULL;
	uint32_t n_quotes = 0;

	if (isInherit(opv) == false) {
		uint32_t v = getValue(opv);

		value = CSS_QUOTES_STRING;

		while (v != QUOTES_NONE) {
			lwc_string *open, *close;
			lwc_string **temp;

			open = *((lwc_string **) style->bytecode);
			advance_bytecode(style, sizeof(lwc_string *));

			close = *((lwc_string **) style->bytecode);
			advance_bytecode(style, sizeof(lwc_string *));

			temp = state->result->alloc(quotes, 
					(n_quotes + 2) * sizeof(lwc_string *), 
					state->result->pw);
			if (temp == NULL) {
				if (quotes != NULL) {
					state->result->alloc(quotes, 0,
							state->result->pw);
				}
				return CSS_NOMEM;
			}

			quotes = temp;

			quotes[n_quotes++] = open;
			quotes[n_quotes++] = close;

			v = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(v));
		}
	}

	/* Terminate array, if required */
	if (n_quotes > 0) {
		lwc_string **temp;

		temp = state->result->alloc(quotes, 
				(n_quotes + 1) * sizeof(lwc_string *), 
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(quotes, 0, state->result->pw);
			return CSS_NOMEM;
		}

		quotes = temp;

		quotes[n_quotes] = NULL;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		css_error error;

		error = set_quotes(state->result, value, quotes);
		if (error != CSS_OK && quotes != NULL)
			state->result->alloc(quotes, 0, state->result->pw);

		return error;
	} else {
		if (quotes != NULL)
			state->result->alloc(quotes, 0, state->result->pw);
	}

	return CSS_OK;
}

css_error set_quotes_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	lwc_string **item;
	css_error error;
		
	error = set_quotes(style, hint->status, hint->data.strings);

	for (item = hint->data.strings;
			item != NULL && (*item) != NULL; item++) {
		lwc_string_unref(*item);
	}

	if (error != CSS_OK && hint->data.strings != NULL)
		style->alloc(hint->data.strings, 0, style->pw);

	return error;
}

css_error initial_quotes(css_select_state *state)
{
	css_hint hint;
	css_error error;

	error = state->handler->ua_default_for_property(state->pw,
			CSS_PROP_QUOTES, &hint);
	if (error != CSS_OK)
		return error;

	return set_quotes_from_hint(&hint, state->result);
}

css_error compose_quotes(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_error error;
	lwc_string **quotes = NULL;
	uint8_t type = get_quotes(child, &quotes);

	if (type == CSS_QUOTES_INHERIT || result != child) {
		size_t n_quotes = 0;
		lwc_string **copy = NULL;

		if (type == CSS_QUOTES_INHERIT) {
			type = get_quotes(parent, &quotes);
		}

		if (quotes != NULL) {
			lwc_string **i;

			for (i = quotes; (*i) != NULL; i++)
				n_quotes++;

			copy = result->alloc(NULL, (n_quotes + 1) * 
					sizeof(lwc_string *),
					result->pw);
			if (copy == NULL)
				return CSS_NOMEM;

			memcpy(copy, quotes, (n_quotes + 1) * 
					sizeof(lwc_string *));
		}

		error = set_quotes(result, type, copy);
		if (error != CSS_OK && copy != NULL)
			result->alloc(copy, 0, result->pw);

		return error;
	}

	return CSS_OK;
}

uint32_t destroy_quotes(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	while (value == QUOTES_STRING) {
		lwc_string **str = ((lwc_string **)bytecode);
		consumed += sizeof(lwc_string*) * 2;
		bytecode = ((uint8_t*)bytecode) + (sizeof(lwc_string*) * 2);
		lwc_string_unref(str[0]);
		lwc_string_unref(str[1]);
			
		consumed += sizeof(uint32_t);
		value = *((uint32_t*)bytecode);
		bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	}
	
	return consumed;
}

css_error cascade_richness(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo richness */
	return cascade_number(opv, style, state, NULL);
}

css_error set_richness_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_richness(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_richness(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_richness(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_right(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_right);
}

css_error set_right_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_right(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_right(css_select_state *state)
{
	return set_right(state->result, CSS_RIGHT_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_right(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_right(child, &length, &unit);

	if (type == CSS_RIGHT_INHERIT) {
		type = get_right(parent, &length, &unit);
	}

	return set_right(result, type, length, unit);
}

uint32_t destroy_right(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_speak_header(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case SPEAK_HEADER_ONCE:
		case SPEAK_HEADER_ALWAYS:
			/** \todo convert to public values */
			value = 0;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo speak-header */
	}

	return CSS_OK;
}

css_error set_speak_header_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_speak_header(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_speak_header(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_speak_header(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_speak_numeral(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case SPEAK_NUMERAL_DIGITS:
		case SPEAK_NUMERAL_CONTINUOUS:
			/** \todo convert to public values */
			value = 0;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo speak-numeral */
	}

	return CSS_OK;
}

css_error set_speak_numeral_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_speak_numeral(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_speak_numeral(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_speak_numeral(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_speak_punctuation( 
		uint32_t opv, css_style *style, css_select_state *state)
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case SPEAK_PUNCTUATION_CODE:
		case SPEAK_PUNCTUATION_NONE:
			/** \todo convert to public values */
			value = 0;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo speak-punctuation */
	}

	return CSS_OK;
}

css_error set_speak_punctuation_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_speak_punctuation(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_speak_punctuation(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_speak_punctuation(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_speak(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case SPEAK_NORMAL:
		case SPEAK_NONE:
		case SPEAK_SPELL_OUT:
			/** \todo convert to public values */
			value = 0;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo speak */
	}

	return CSS_OK;
}

css_error set_speak_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_speak(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_speak(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_speak(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_speech_rate(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	css_fixed rate = 0;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case SPEECH_RATE_SET:
			value = 0;

			rate = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(rate));
			break;
		case SPEECH_RATE_X_SLOW:
		case SPEECH_RATE_SLOW:
		case SPEECH_RATE_MEDIUM:
		case SPEECH_RATE_FAST:
		case SPEECH_RATE_X_FAST:
		case SPEECH_RATE_FASTER:
		case SPEECH_RATE_SLOWER:
			/** \todo convert to public values */
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo speech-rate */
	}

	return CSS_OK;
}

css_error set_speech_rate_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_speech_rate(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_speech_rate(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_speech_rate(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_stress(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo stress */
	return cascade_number(opv, style, state, NULL);
}

css_error set_stress_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_stress(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_stress(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_stress(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_table_layout(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_TABLE_LAYOUT_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case TABLE_LAYOUT_AUTO:
			value = CSS_TABLE_LAYOUT_AUTO;
			break;
		case TABLE_LAYOUT_FIXED:
			value = CSS_TABLE_LAYOUT_FIXED;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_table_layout(state->result, value);
	}

	return CSS_OK;
}

css_error set_table_layout_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_table_layout(style, hint->status);
}

css_error initial_table_layout(css_select_state *state)
{
	return set_table_layout(state->result, CSS_TABLE_LAYOUT_AUTO);
}

css_error compose_table_layout(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_table_layout(child);

	if (type == CSS_TABLE_LAYOUT_INHERIT) {
		type = get_table_layout(parent);
	}

	return set_table_layout(result, type);
}

uint32_t destroy_table_layout(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_text_align(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_ALIGN_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case TEXT_ALIGN_LEFT:
			value = CSS_TEXT_ALIGN_LEFT;
			break;
		case TEXT_ALIGN_RIGHT:
			value = CSS_TEXT_ALIGN_RIGHT;
			break;
		case TEXT_ALIGN_CENTER:
			value = CSS_TEXT_ALIGN_CENTER;
			break;
		case TEXT_ALIGN_JUSTIFY:
			value = CSS_TEXT_ALIGN_JUSTIFY;
			break;
		case TEXT_ALIGN_LIBCSS_LEFT:
			value = CSS_TEXT_ALIGN_LIBCSS_LEFT;
			break;
		case TEXT_ALIGN_LIBCSS_CENTER:
			value = CSS_TEXT_ALIGN_LIBCSS_CENTER;
			break;
		case TEXT_ALIGN_LIBCSS_RIGHT:
			value = CSS_TEXT_ALIGN_LIBCSS_RIGHT;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_text_align(state->result, value);
	}

	return CSS_OK;
}

css_error set_text_align_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_align(style, hint->status);
}

css_error initial_text_align(css_select_state *state)
{
	return set_text_align(state->result, CSS_TEXT_ALIGN_DEFAULT);
}

css_error compose_text_align(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_text_align(child);

	if (type == CSS_TEXT_ALIGN_INHERIT) {
		type = get_text_align(parent);
	} else if (type == CSS_TEXT_ALIGN_INHERIT_IF_NON_MAGIC) {
		/* This is purely for the benefit of HTML tables */
		type = get_text_align(parent);

		/* If the parent's text-align is a magical one, 
		 * then reset to the default value. Otherwise, 
		 * inherit as normal. */
		if (type == CSS_TEXT_ALIGN_LIBCSS_LEFT ||
				type == CSS_TEXT_ALIGN_LIBCSS_CENTER ||
				type == CSS_TEXT_ALIGN_LIBCSS_RIGHT)
			type = CSS_TEXT_ALIGN_DEFAULT;
	}

	return set_text_align(result, type);
}

uint32_t destroy_text_align(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_text_decoration(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_DECORATION_INHERIT;
	
	UNUSED(style);

	if (isInherit(opv) == false) {
		if (getValue(opv) == TEXT_DECORATION_NONE) {
			value = CSS_TEXT_DECORATION_NONE;
		} else {
			assert(value == 0);

			if (getValue(opv) & TEXT_DECORATION_UNDERLINE)
				value |= CSS_TEXT_DECORATION_UNDERLINE;
			if (getValue(opv) & TEXT_DECORATION_OVERLINE)
				value |= CSS_TEXT_DECORATION_OVERLINE;
			if (getValue(opv) & TEXT_DECORATION_LINE_THROUGH)
				value |= CSS_TEXT_DECORATION_LINE_THROUGH;
			if (getValue(opv) & TEXT_DECORATION_BLINK)
				value |= CSS_TEXT_DECORATION_BLINK;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_text_decoration(state->result, value);
	}

	return CSS_OK;
}

css_error set_text_decoration_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_decoration(style, hint->status);
}

css_error initial_text_decoration(css_select_state *state)
{
	return set_text_decoration(state->result, CSS_TEXT_DECORATION_NONE);
}

css_error compose_text_decoration(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_text_decoration(child);

	if (type == CSS_TEXT_DECORATION_INHERIT) {
		type = get_text_decoration(parent);
	}

	return set_text_decoration(result, type);
}

uint32_t destroy_text_decoration(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_text_indent(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length(opv, style, state, set_text_indent);
}

css_error set_text_indent_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_indent(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_text_indent(css_select_state *state)
{
	return set_text_indent(state->result, CSS_TEXT_INDENT_SET, 
			0, CSS_UNIT_PX);
}

css_error compose_text_indent(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_text_indent(child, &length, &unit);

	if (type == CSS_TEXT_INDENT_INHERIT) {
		type = get_text_indent(parent, &length, &unit);
	}

	return set_text_indent(result, type, length, unit);
}

uint32_t destroy_text_indent(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_text_transform(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_TRANSFORM_INHERIT;
	
	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case TEXT_TRANSFORM_CAPITALIZE:
			value = CSS_TEXT_TRANSFORM_CAPITALIZE;
			break;
		case TEXT_TRANSFORM_UPPERCASE:
			value = CSS_TEXT_TRANSFORM_UPPERCASE;
			break;
		case TEXT_TRANSFORM_LOWERCASE:
			value = CSS_TEXT_TRANSFORM_LOWERCASE;
			break;
		case TEXT_TRANSFORM_NONE:
			value = CSS_TEXT_TRANSFORM_NONE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_text_transform(state->result, value);
	}

	return CSS_OK;
}

css_error set_text_transform_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_transform(style, hint->status);
}

css_error initial_text_transform(css_select_state *state)
{
	return set_text_transform(state->result, CSS_TEXT_TRANSFORM_NONE);
}

css_error compose_text_transform(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_text_transform(child);

	if (type == CSS_TEXT_TRANSFORM_INHERIT) {
		type = get_text_transform(parent);
	}

	return set_text_transform(result, type);
}

uint32_t destroy_text_transform(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_top(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_top);
}

css_error set_top_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_top(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_top(css_select_state *state)
{
	return set_top(state->result, CSS_TOP_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_top(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_top(child, &length, &unit);

	if (type == CSS_TOP_INHERIT) {
		type = get_top(parent, &length, &unit);
	}

	return set_top(result, type, length, unit);
}

uint32_t destroy_top(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_unicode_bidi(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_UNICODE_BIDI_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case UNICODE_BIDI_NORMAL:
			value = CSS_UNICODE_BIDI_NORMAL;
			break;
		case UNICODE_BIDI_EMBED:
			value = CSS_UNICODE_BIDI_EMBED;
			break;
		case UNICODE_BIDI_BIDI_OVERRIDE:
			value = CSS_UNICODE_BIDI_BIDI_OVERRIDE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_unicode_bidi(state->result, value);
	}

	return CSS_OK;
}

css_error set_unicode_bidi_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_unicode_bidi(style, hint->status);
}

css_error initial_unicode_bidi(css_select_state *state)
{
	return set_unicode_bidi(state->result, CSS_UNICODE_BIDI_NORMAL);
}

css_error compose_unicode_bidi(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_unicode_bidi(child);

	if (type == CSS_UNICODE_BIDI_INHERIT) {
		type = get_unicode_bidi(parent);
	}

	return set_unicode_bidi(result, type);
}

uint32_t destroy_unicode_bidi(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_vertical_align(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_VERTICAL_ALIGN_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case VERTICAL_ALIGN_SET:
			value = CSS_VERTICAL_ALIGN_SET;

			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case VERTICAL_ALIGN_BASELINE:
			value = CSS_VERTICAL_ALIGN_BASELINE;
			break;
		case VERTICAL_ALIGN_SUB:
			value = CSS_VERTICAL_ALIGN_SUB;
			break;
		case VERTICAL_ALIGN_SUPER:
			value = CSS_VERTICAL_ALIGN_SUPER;
			break;
		case VERTICAL_ALIGN_TOP:
			value = CSS_VERTICAL_ALIGN_TOP;
			break;
		case VERTICAL_ALIGN_TEXT_TOP:
			value = CSS_VERTICAL_ALIGN_TEXT_TOP;
			break;
		case VERTICAL_ALIGN_MIDDLE:
			value = CSS_VERTICAL_ALIGN_MIDDLE;
			break;
		case VERTICAL_ALIGN_BOTTOM:
			value = CSS_VERTICAL_ALIGN_BOTTOM;
			break;
		case VERTICAL_ALIGN_TEXT_BOTTOM:
			value = CSS_VERTICAL_ALIGN_TEXT_BOTTOM;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_vertical_align(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error set_vertical_align_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_vertical_align(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_vertical_align(css_select_state *state)
{
	return set_vertical_align(state->result, CSS_VERTICAL_ALIGN_BASELINE,
			0, CSS_UNIT_PX);
}

css_error compose_vertical_align(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_vertical_align(child, &length, &unit);

	if (type == CSS_VERTICAL_ALIGN_INHERIT) {
		type = get_vertical_align(parent, &length, &unit);
	}

	return set_vertical_align(result, type, length, unit);
}

uint32_t destroy_vertical_align(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_visibility(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_VISIBILITY_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case VISIBILITY_VISIBLE:
			value = CSS_VISIBILITY_VISIBLE;
			break;
		case VISIBILITY_HIDDEN:
			value = CSS_VISIBILITY_HIDDEN;
			break;
		case VISIBILITY_COLLAPSE:
			value = CSS_VISIBILITY_COLLAPSE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_visibility(state->result, value);
	}

	return CSS_OK;
}

css_error set_visibility_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_visibility(style, hint->status);
}

css_error initial_visibility(css_select_state *state)
{
	return set_visibility(state->result, CSS_VISIBILITY_VISIBLE);
}

css_error compose_visibility(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_visibility(child);

	if (type == CSS_VISIBILITY_INHERIT) {
		type = get_visibility(parent);
	}

	return set_visibility(result, type);
}

uint32_t destroy_visibility(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_voice_family(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	lwc_string **voices = NULL;
	uint32_t n_voices = 0;

	if (isInherit(opv) == false) {
		uint32_t v = getValue(opv);

		while (v != VOICE_FAMILY_END) {
			lwc_string *voice = NULL;
			lwc_string **temp;

			switch (v) {
			case VOICE_FAMILY_STRING:
			case VOICE_FAMILY_IDENT_LIST:
				voice = *((lwc_string **) 
						style->bytecode);
				advance_bytecode(style, sizeof(voice));
				break;
			case VOICE_FAMILY_MALE:
				if (value == 0)
					value = 1;
				break;
			case VOICE_FAMILY_FEMALE:
				if (value == 0)
					value = 1;
				break;
			case VOICE_FAMILY_CHILD:
				if (value == 0)
					value = 1;
				break;
			}

			/* Only use family-names which occur before the first
			 * generic-family. Any values which occur after the
			 * first generic-family are ignored. */
			/** \todo Do this at bytecode generation time? */
			if (value == 0 && voice != NULL) {
				temp = state->result->alloc(voices, 
					(n_voices + 1) * sizeof(lwc_string *), 
					state->result->pw);
				if (temp == NULL) {
					if (voices != NULL) {
						state->result->alloc(voices, 0,
							state->result->pw);
					}
					return CSS_NOMEM;
				}

				voices = temp;

				voices[n_voices] = voice;

				n_voices++;
			}

			v = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(v));
		}
	}

	/* Terminate array with blank entry, if needed */
	if (n_voices > 0) {
		lwc_string **temp;

		temp = state->result->alloc(voices, 
				(n_voices + 1) * sizeof(lwc_string *), 
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(voices, 0, state->result->pw);
			return CSS_NOMEM;
		}

		voices = temp;

		voices[n_voices] = NULL;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo voice-family */
		if (n_voices > 0)
			state->result->alloc(voices, 0, state->result->pw);
	} else {
		if (n_voices > 0)
			state->result->alloc(voices, 0, state->result->pw);
	}

	return CSS_OK;
}

css_error set_voice_family_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_voice_family(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_voice_family(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_voice_family(void *bytecode)
{
	uint32_t consumed = sizeof(uint32_t);
	uint32_t value = getValue(*((uint32_t*)bytecode));
	bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	
	while (value != VOICE_FAMILY_END) {
		if (value == VOICE_FAMILY_STRING || value == VOICE_FAMILY_IDENT_LIST) {
			lwc_string *str = *((lwc_string **)bytecode);
			consumed += sizeof(lwc_string*);
			bytecode = ((uint8_t*)bytecode) + sizeof(lwc_string*);
			lwc_string_unref(str);
		}
		
		consumed += sizeof(uint32_t);
		value = *((uint32_t*)bytecode);
		bytecode = ((uint8_t*)bytecode) + sizeof(uint32_t);
	}
	
	return consumed;
}

css_error cascade_volume(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = 0;
	css_fixed val = 0;
	uint32_t unit = UNIT_PCT;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case VOLUME_NUMBER:
			value = 0;

			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));
			break;
		case VOLUME_DIMENSION:
			value = 0;

			val = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(val));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case VOLUME_SILENT:
		case VOLUME_X_SOFT:
		case VOLUME_SOFT:
		case VOLUME_MEDIUM:
		case VOLUME_LOUD:
		case VOLUME_X_LOUD:
			/** \todo convert to public values */
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		/** \todo volume */
	}

	return CSS_OK;
}

css_error set_volume_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_volume(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_volume(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_volume(void *bytecode)
{
	uint32_t value = getValue(*((uint32_t*)bytecode));
	uint32_t additional = 0;
	if (value == VOLUME_NUMBER)
		additional = sizeof(css_fixed);
	else if (value == VOLUME_DIMENSION)
		additional = sizeof(css_fixed) + sizeof(uint32_t);
	
	return sizeof(uint32_t) + additional;
}

css_error cascade_white_space(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_WHITE_SPACE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case WHITE_SPACE_NORMAL:
			value = CSS_WHITE_SPACE_NORMAL;
			break;
		case WHITE_SPACE_PRE:
			value = CSS_WHITE_SPACE_PRE;
			break;
		case WHITE_SPACE_NOWRAP:
			value = CSS_WHITE_SPACE_NOWRAP;
			break;
		case WHITE_SPACE_PRE_WRAP:
			value = CSS_WHITE_SPACE_PRE_WRAP;
			break;
		case WHITE_SPACE_PRE_LINE:
			value = CSS_WHITE_SPACE_PRE_LINE;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_white_space(state->result, value);
	}

	return CSS_OK;
}

css_error set_white_space_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_white_space(style, hint->status);
}

css_error initial_white_space(css_select_state *state)
{
	return set_white_space(state->result, CSS_WHITE_SPACE_NORMAL);
}

css_error compose_white_space(const css_computed_style *parent,	
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_white_space(child);

	if (type == CSS_WHITE_SPACE_INHERIT) {
		type = get_white_space(parent);
	}

	return set_white_space(result, type);
}

uint32_t destroy_white_space(void *bytecode)
{
	UNUSED(bytecode);
	
	return sizeof(uint32_t);
}

css_error cascade_widows(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	/** \todo widows */
	return cascade_number(opv, style, state, NULL);
}

css_error set_widows_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	UNUSED(hint);
	UNUSED(style);

	return CSS_OK;
}

css_error initial_widows(css_select_state *state)
{
	UNUSED(state);

	return CSS_OK;
}

css_error compose_widows(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	UNUSED(parent);
	UNUSED(child);
	UNUSED(result);

	return CSS_OK;
}

uint32_t destroy_widows(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

css_error cascade_width(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_auto(opv, style, state, set_width);
}

css_error set_width_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_width(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_width(css_select_state *state)
{
	return set_width(state->result, CSS_WIDTH_AUTO, 0, CSS_UNIT_PX);
}

css_error compose_width(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_width(child, &length, &unit);

	if (type == CSS_WIDTH_INHERIT) {
		type = get_width(parent, &length, &unit);
	}

	return set_width(result, type, length, unit);
}

uint32_t destroy_width(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_word_spacing(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	return cascade_length_normal(opv, style, state, set_word_spacing);
}

css_error set_word_spacing_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_word_spacing(style, hint->status,
			hint->data.length.value, hint->data.length.unit);
}

css_error initial_word_spacing(css_select_state *state)
{
	return set_word_spacing(state->result, CSS_WORD_SPACING_NORMAL, 
			0, CSS_UNIT_PX);
}

css_error compose_word_spacing(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed length = 0;
	css_unit unit = CSS_UNIT_PX;
	uint8_t type = get_word_spacing(child, &length, &unit);

	if ((child->uncommon == NULL && parent->uncommon != NULL) ||
			type == CSS_WORD_SPACING_INHERIT ||
			(child->uncommon != NULL && result != child)) {
		if ((child->uncommon == NULL && parent->uncommon != NULL) ||
				type == CSS_WORD_SPACING_INHERIT) {
			type = get_word_spacing(parent, &length, &unit);
		}

		return set_word_spacing(result, type, length, unit);
	}

	return CSS_OK;
}

uint32_t destroy_word_spacing(void *bytecode)
{
	return generic_destroy_length(bytecode);
}

css_error cascade_z_index(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_Z_INDEX_INHERIT;
	css_fixed index = 0;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case Z_INDEX_SET:
			value = CSS_Z_INDEX_SET;

			index = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(index));
			break;
		case Z_INDEX_AUTO:
			value = CSS_Z_INDEX_AUTO;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_z_index(state->result, value, index);
	}

	return CSS_OK;
}

css_error set_z_index_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_z_index(style, hint->status, hint->data.integer);
}

css_error initial_z_index(css_select_state *state)
{
	return set_z_index(state->result, CSS_Z_INDEX_AUTO, 0);
}

css_error compose_z_index(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	int32_t index = 0;
	uint8_t type = get_z_index(child, &index);

	if (type == CSS_Z_INDEX_INHERIT) {
		type = get_z_index(parent, &index);
	}

	return set_z_index(result, type, index);
}

uint32_t destroy_z_index(void *bytecode)
{
	return generic_destroy_number(bytecode);
}

/******************************************************************************
 * Utilities below here							      *
 ******************************************************************************/
css_error cascade_bg_border_color(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t, css_color))
{
	uint16_t value = CSS_BACKGROUND_COLOR_INHERIT;
	css_color color = 0;

	assert(CSS_BACKGROUND_COLOR_INHERIT == CSS_BORDER_COLOR_INHERIT);
	assert(CSS_BACKGROUND_COLOR_TRANSPARENT == 
			CSS_BORDER_COLOR_TRANSPARENT);
	assert(CSS_BACKGROUND_COLOR_COLOR == CSS_BORDER_COLOR_COLOR);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BACKGROUND_COLOR_TRANSPARENT:
			value = CSS_BACKGROUND_COLOR_TRANSPARENT;
			break;
		case BACKGROUND_COLOR_SET:
			value = CSS_BACKGROUND_COLOR_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value, color);
	}

	return CSS_OK;
}

css_error cascade_uri_none(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, 
				lwc_string *))
{
	uint16_t value = CSS_BACKGROUND_IMAGE_INHERIT;
	lwc_string *uri = NULL;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BACKGROUND_IMAGE_NONE:
			value = CSS_BACKGROUND_IMAGE_NONE;
			break;
		case BACKGROUND_IMAGE_URI:
			value = CSS_BACKGROUND_IMAGE_IMAGE;
			uri = *((lwc_string **) style->bytecode);
			advance_bytecode(style, sizeof(uri));
			break;
		}
	}

	/** \todo lose fun != NULL once all properties have set routines */
	if (fun != NULL && outranks_existing(getOpcode(opv), 
			isImportant(opv), state, isInherit(opv))) {
		return fun(state->result, value, uri);
	}

	return CSS_OK;
}

css_error cascade_border_style(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t))
{
	uint16_t value = CSS_BORDER_STYLE_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BORDER_STYLE_NONE:
			value = CSS_BORDER_STYLE_NONE;
			break;
		case BORDER_STYLE_HIDDEN:
			value = CSS_BORDER_STYLE_HIDDEN;
			break;
		case BORDER_STYLE_DOTTED:
			value = CSS_BORDER_STYLE_DOTTED;
			break;
		case BORDER_STYLE_DASHED:
			value = CSS_BORDER_STYLE_DASHED;
			break;
		case BORDER_STYLE_SOLID:
			value = CSS_BORDER_STYLE_SOLID;
			break;
		case BORDER_STYLE_DOUBLE:
			value = CSS_BORDER_STYLE_DOUBLE;
			break;
		case BORDER_STYLE_GROOVE:
			value = CSS_BORDER_STYLE_GROOVE;
			break;
		case BORDER_STYLE_RIDGE:
			value = CSS_BORDER_STYLE_RIDGE;
			break;
		case BORDER_STYLE_INSET:
			value = CSS_BORDER_STYLE_INSET;
			break;
		case BORDER_STYLE_OUTSET:
			value = CSS_BORDER_STYLE_OUTSET;
			break;
		}
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value);
	}

	return CSS_OK;
}

css_error cascade_border_width(uint32_t opv, css_style *style,
		css_select_state *state, 
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed, 
				css_unit))
{
	uint16_t value = CSS_BORDER_WIDTH_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BORDER_WIDTH_SET:
			value = CSS_BORDER_WIDTH_WIDTH;
			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case BORDER_WIDTH_THIN:
			value = CSS_BORDER_WIDTH_THIN;
			break;
		case BORDER_WIDTH_MEDIUM:
			value = CSS_BORDER_WIDTH_MEDIUM;
			break;
		case BORDER_WIDTH_THICK:
			value = CSS_BORDER_WIDTH_THICK;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error cascade_length_auto(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit))
{
	uint16_t value = CSS_BOTTOM_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case BOTTOM_SET:
			value = CSS_BOTTOM_SET;
			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case BOTTOM_AUTO:
			value = CSS_BOTTOM_AUTO;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error cascade_length_normal(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit))
{
	uint16_t value = CSS_LETTER_SPACING_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case LETTER_SPACING_SET:
			value = CSS_LETTER_SPACING_SET;
			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case LETTER_SPACING_NORMAL:
			value = CSS_LETTER_SPACING_NORMAL;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error cascade_length_none(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit))
{
	uint16_t value = CSS_MAX_HEIGHT_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case MAX_HEIGHT_SET:
			value = CSS_MAX_HEIGHT_SET;
			length = *((css_fixed *) style->bytecode);
			advance_bytecode(style, sizeof(length));
			unit = *((uint32_t *) style->bytecode);
			advance_bytecode(style, sizeof(unit));
			break;
		case MAX_HEIGHT_NONE:
			value = CSS_MAX_HEIGHT_NONE;
			break;
		}
	}

	unit = to_css_unit(unit);

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return fun(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error cascade_length(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed,
				css_unit))
{
	uint16_t value = CSS_MIN_HEIGHT_INHERIT;
	css_fixed length = 0;
	uint32_t unit = UNIT_PX;

	if (isInherit(opv) == false) {
		value = CSS_MIN_HEIGHT_SET;
		length = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(length));
		unit = *((uint32_t *) style->bytecode);
		advance_bytecode(style, sizeof(unit));
	}

	unit = to_css_unit(unit);

	/** \todo lose fun != NULL once all properties have set routines */
	if (fun != NULL && outranks_existing(getOpcode(opv), 
			isImportant(opv), state, isInherit(opv))) {
		return fun(state->result, value, length, unit);
	}

	return CSS_OK;
}

css_error cascade_number(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t, css_fixed))
{
	uint16_t value = 0;
	css_fixed length = 0;

	/** \todo values */

	if (isInherit(opv) == false) {
		value = 0;
		length = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(length));
	}

	/** \todo lose fun != NULL once all properties have set routines */
	if (fun != NULL && outranks_existing(getOpcode(opv), 
			isImportant(opv), state, isInherit(opv))) {
		return fun(state->result, value, length);
	}

	return CSS_OK;
}

css_error cascade_page_break_after_before(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t))
{
	uint16_t value = 0;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case PAGE_BREAK_AFTER_AUTO:
		case PAGE_BREAK_AFTER_ALWAYS:
		case PAGE_BREAK_AFTER_AVOID:
		case PAGE_BREAK_AFTER_LEFT:
		case PAGE_BREAK_AFTER_RIGHT:
			/** \todo convert to public values */
			break;
		}
	}

	/** \todo lose fun != NULL */
	if (fun != NULL && outranks_existing(getOpcode(opv), 
			isImportant(opv), state, isInherit(opv))) {
		return fun(state->result, value);
	}

	return CSS_OK;
}

css_error cascade_counter_increment_reset(uint32_t opv, css_style *style,
		css_select_state *state,
		css_error (*fun)(css_computed_style *, uint8_t,
				css_computed_counter *))
{
	uint16_t value = CSS_COUNTER_INCREMENT_INHERIT;
	css_computed_counter *counters = NULL;
	uint32_t n_counters = 0;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case COUNTER_INCREMENT_NAMED:
		{
			uint32_t v = getValue(opv);

			while (v != COUNTER_INCREMENT_NONE) {
				css_computed_counter *temp;
				lwc_string *name;
				css_fixed val = 0;

				name = *((lwc_string **)
						style->bytecode);
				advance_bytecode(style, sizeof(name));

				val = *((css_fixed *) style->bytecode);
				advance_bytecode(style, sizeof(val));

				temp = state->result->alloc(counters,
						(n_counters + 1) * 
						sizeof(css_computed_counter),
						state->result->pw);
				if (temp == NULL) {
					if (counters != NULL) {
						state->result->alloc(counters, 
							0, state->result->pw);
					}
					return CSS_NOMEM;
				}

				counters = temp;

				counters[n_counters].name = name;
				counters[n_counters].value = val;

				n_counters++;

				v = *((uint32_t *) style->bytecode);
				advance_bytecode(style, sizeof(v));
			}
		}
			break;
		case COUNTER_INCREMENT_NONE:
			value = CSS_COUNTER_INCREMENT_NONE;
			break;
		}
	}

	/* If we have some counters, terminate the array with a blank entry */
	if (n_counters > 0) {
		css_computed_counter *temp;

		temp = state->result->alloc(counters, 
				(n_counters + 1) * sizeof(css_computed_counter),
				state->result->pw);
		if (temp == NULL) {
			state->result->alloc(counters, 0, state->result->pw);
			return CSS_NOMEM;
		}

		counters = temp;

		counters[n_counters].name = NULL;
		counters[n_counters].value = 0;
	}

	if (outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		css_error error;

		error = fun(state->result, value, counters);
		if (error != CSS_OK && n_counters > 0)
			state->result->alloc(counters, 0, state->result->pw);

		return error;
	} else if (n_counters > 0) {
		state->result->alloc(counters, 0, state->result->pw);
	}

	return CSS_OK;
}

