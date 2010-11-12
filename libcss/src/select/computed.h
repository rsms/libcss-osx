/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_select_computed_h_
#define css_select_computed_h_

#include <libcss/computed.h>
#include <libcss/hint.h>

css_error compute_absolute_values(const css_computed_style *parent,
		css_computed_style *style,
		css_error (*compute_font_size)(void *pw, 
			const css_hint *parent, css_hint *size),
		void *pw);

#endif
