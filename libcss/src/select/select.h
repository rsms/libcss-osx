/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_select_select_h_
#define css_select_select_h_

#include <stdbool.h>
#include <stdint.h>

#include <libcss/select.h>

#include "stylesheet.h"

typedef struct prop_state {
	uint32_t specificity;		/* Specificity of property in result */
	unsigned int set       : 1,	/* Whether property is set in result */
	             origin    : 2,	/* Origin of property in result */
	             important : 1,	/* Importance of property in result */
	             inherit   : 1;	/* Property is set to inherit */
} prop_state;

/**
 * Selection state
 */
typedef struct css_select_state {
	void *node;			/* Node we're selecting for */
	uint32_t pseudo_element;	/* Pseudo element to select for */
	uint64_t media;			/* Currently active media types */
	css_computed_style *result;	/* Style to populate */

	css_select_handler *handler;	/* Handler functions */
	void *pw;			/* Client data for handlers */

	const css_stylesheet *sheet;	/* Current sheet being processed */

	css_origin current_origin;	/* Origin of current sheet */
	uint32_t current_specificity;	/* Specificity of current rule */

	/* Useful interned strings */
	lwc_string *universal;
	lwc_string *first_child;
	lwc_string *link;
	lwc_string *visited;
	lwc_string *hover;
	lwc_string *active;
	lwc_string *focus;
	lwc_string *first_line;
	lwc_string *first_letter;
	lwc_string *before;
	lwc_string *after;

	prop_state props[CSS_N_PROPERTIES];
} css_select_state;

static inline void advance_bytecode(css_style *style, uint32_t n_bytes)
{
	style->length -= n_bytes;
	style->bytecode = ((uint8_t *) style->bytecode) + n_bytes;
}

bool outranks_existing(uint16_t op, bool important, css_select_state *state,
		bool inherit);

#endif

