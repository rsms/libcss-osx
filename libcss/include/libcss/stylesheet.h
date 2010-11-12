/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef libcss_stylesheet_h_
#define libcss_stylesheet_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <libcss/errors.h>
#include <libcss/types.h>

/**
 * Callback to resolve an URL
 *
 * \param pw  Client data
 * \param dict  String internment context
 * \param base  Base URI (absolute)
 * \param rel   URL to resolve, either absolute or relative to base
 * \param abs   Pointer to location to receive result
 * \return CSS_OK on success, appropriate error otherwise.
 */
typedef css_error (*css_url_resolution_fn)(void *pw,
		const char *base, lwc_string *rel, lwc_string **abs);

css_error css_stylesheet_create(css_language_level level,
		const char *charset, const char *url, const char *title,
		bool allow_quirks, bool inline_style,
		css_allocator_fn alloc, void *alloc_pw, 
		css_url_resolution_fn resolve, void *resolve_pw,
		css_stylesheet **stylesheet);
css_error css_stylesheet_destroy(css_stylesheet *sheet);

css_error css_stylesheet_append_data(css_stylesheet *sheet,
		const uint8_t *data, size_t len);
css_error css_stylesheet_data_done(css_stylesheet *sheet);

css_error css_stylesheet_next_pending_import(css_stylesheet *parent,
		lwc_string **url, uint64_t *media);
css_error css_stylesheet_register_import(css_stylesheet *parent,
		css_stylesheet *child);

css_error css_stylesheet_get_language_level(css_stylesheet *sheet,
		css_language_level *level);
css_error css_stylesheet_get_url(css_stylesheet *sheet, const char **url);
css_error css_stylesheet_get_title(css_stylesheet *sheet, const char **title);
css_error css_stylesheet_quirks_allowed(css_stylesheet *sheet, bool *allowed);
css_error css_stylesheet_used_quirks(css_stylesheet *sheet, bool *quirks);

css_error css_stylesheet_get_disabled(css_stylesheet *sheet, bool *disabled);
css_error css_stylesheet_set_disabled(css_stylesheet *sheet, bool disabled);

css_error css_stylesheet_size(css_stylesheet *sheet, size_t *size);

#ifdef __cplusplus
}
#endif

#endif

