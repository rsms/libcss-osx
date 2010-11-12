/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2007 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef libcss_h_
#define libcss_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <libwapcaplet/libwapcaplet.h>

#include <libcss/errors.h>
#include <libcss/types.h>
#include <libcss/functypes.h>
#include <libcss/computed.h>
#include <libcss/properties.h>
#include <libcss/select.h>
#include <libcss/stylesheet.h>

/* Initialise the CSS library for use */
css_error css_initialise(const char *aliases_file,
		css_allocator_fn alloc, void *pw);

/* Clean up after LibCSS */
css_error css_finalise(css_allocator_fn alloc, void *pw);

#ifdef __cplusplus
}
#endif

#endif

