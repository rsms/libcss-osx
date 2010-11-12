/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2007 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <parserutils/parserutils.h>

#include <libcss/libcss.h>

#include "utils/parserutilserror.h"

/**
 * Initialise the CSS library for use.
 *
 * This _must_ be called before using any LibCSS functions
 *
 * \param aliases_file  Pointer to name of file containing encoding alias data
 * \param alloc         Pointer to (de)allocation function
 * \param pw            Pointer to client-specific private data (may be NULL)
 * \return CSS_OK on success, applicable error otherwise.
 */
css_error css_initialise(const char *aliases_file,
		css_allocator_fn alloc, void *pw)
{
	if (aliases_file == NULL || alloc == NULL)
		return CSS_BADPARM;

	return css_error_from_parserutils_error(
			parserutils_initialise(aliases_file, alloc, pw));
}

/**
 * Clean up after LibCSS
 *
 * \param alloc  Pointer to (de)allocation function
 * \param pw     Pointer to client-specific private data (may be NULL)
 * \return CSS_OK on success, applicable error otherwise.
 */
css_error css_finalise(css_allocator_fn alloc, void *pw)
{
	if (alloc == NULL)
		return CSS_BADPARM;

	return css_error_from_parserutils_error(
			parserutils_finalise(alloc, pw));
}


