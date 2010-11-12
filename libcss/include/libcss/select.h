/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef libcss_select_h_
#define libcss_select_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <libwapcaplet/libwapcaplet.h>

#include <libcss/errors.h>
#include <libcss/functypes.h>
#include <libcss/hint.h>
#include <libcss/types.h>

enum css_pseudo_element {
	CSS_PSEUDO_ELEMENT_NONE         = 0,
	CSS_PSEUDO_ELEMENT_FIRST_LINE   = 1,
	CSS_PSEUDO_ELEMENT_FIRST_LETTER = 2,
	CSS_PSEUDO_ELEMENT_BEFORE       = 3,
	CSS_PSEUDO_ELEMENT_AFTER        = 4
};

typedef struct css_select_handler {
	css_error (*node_name)(void *pw, void *node,
			lwc_string **name);
	css_error (*node_classes)(void *pw, void *node,
			lwc_string ***classes,
			uint32_t *n_classes);
	css_error (*node_id)(void *pw, void *node,
			lwc_string **id);

	css_error (*named_ancestor_node)(void *pw, void *node,
			lwc_string *name, void **ancestor);
	css_error (*named_parent_node)(void *pw, void *node,
			lwc_string *name, void **parent);
	css_error (*named_sibling_node)(void *pw, void *node,
			lwc_string *name, void **sibling);

	css_error (*parent_node)(void *pw, void *node, void **parent);
	css_error (*sibling_node)(void *pw, void *node, void **sibling);

	css_error (*node_has_name)(void *pw, void *node,
			lwc_string *name, bool *match);
	css_error (*node_has_class)(void *pw, void *node,
			lwc_string *name, bool *match);
	css_error (*node_has_id)(void *pw, void *node,
			lwc_string *name, bool *match);
	css_error (*node_has_attribute)(void *pw, void *node,
			lwc_string *name, bool *match);
	css_error (*node_has_attribute_equal)(void *pw, void *node,
			lwc_string *name, lwc_string *value,
			bool *match);
	css_error (*node_has_attribute_dashmatch)(void *pw, void *node,
			lwc_string *name, lwc_string *value,
			bool *match);
	css_error (*node_has_attribute_includes)(void *pw, void *node,
			lwc_string *name, lwc_string *value,
			bool *match);

	css_error (*node_is_first_child)(void *pw, void *node, bool *match);
	css_error (*node_is_link)(void *pw, void *node, bool *match);
	css_error (*node_is_visited)(void *pw, void *node, bool *match);
	css_error (*node_is_hover)(void *pw, void *node, bool *match);
	css_error (*node_is_active)(void *pw, void *node, bool *match);
	css_error (*node_is_focus)(void *pw, void *node, bool *match);
	css_error (*node_is_lang)(void *pw, void *node,
			lwc_string *lang, bool *match);

	css_error (*node_presentational_hint)(void *pw, void *node, 
			uint32_t property, css_hint *hint);

	css_error (*ua_default_for_property)(void *pw, uint32_t property,
			css_hint *hint);

	css_error (*compute_font_size)(void *pw, const css_hint *parent,
			css_hint *size);
} css_select_handler;

css_error css_select_ctx_create(css_allocator_fn alloc, void *pw,
		css_select_ctx **result);
css_error css_select_ctx_destroy(css_select_ctx *ctx);

css_error css_select_ctx_append_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, 
		css_origin origin, uint64_t media);
css_error css_select_ctx_insert_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, uint32_t index,
		css_origin origin, uint64_t media);
css_error css_select_ctx_remove_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet);

css_error css_select_ctx_count_sheets(css_select_ctx *ctx, uint32_t *count);
css_error css_select_ctx_get_sheet(css_select_ctx *ctx, uint32_t index,
		const css_stylesheet **sheet);

css_error css_select_style(css_select_ctx *ctx, void *node,
		uint32_t pseudo_element, uint64_t media, 
		const css_stylesheet *inline_style,
		css_computed_style *result,
		css_select_handler *handler, void *pw);

#ifdef __cplusplus
}
#endif

#endif
