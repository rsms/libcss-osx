/*
 * This file is part of LibCSS
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include <libcss/select.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "stylesheet.h"
#include "select/computed.h"
#include "select/dispatch.h"
#include "select/hash.h"
#include "select/propset.h"
#include "select/select.h"
#include "utils/parserutilserror.h"
#include "utils/utils.h"

/* Define this to enable verbose messages when matching selector chains */
#undef DEBUG_CHAIN_MATCHING

/**
 * Container for stylesheet selection info
 */
typedef struct css_select_sheet {
	const css_stylesheet *sheet;	/**< Stylesheet */
	css_origin origin;		/**< Stylesheet origin */
	uint64_t media;			/**< Applicable media */
} css_select_sheet;

/**
 * CSS selection context
 */
struct css_select_ctx {
	uint32_t n_sheets;		/**< Number of sheets */

	css_select_sheet *sheets;	/**< Array of sheets */

	css_allocator_fn alloc;		/**< Allocation routine */
	void *pw;			/**< Client-specific private data */
};

static css_error set_hint(css_select_state *state, uint32_t i);
static css_error set_initial(css_select_state *state, uint32_t i, void *parent);

static css_error select_from_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, css_origin origin,
		css_select_state *state);
static css_error intern_strings_for_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, css_select_state *state);
static css_error match_selectors_in_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, css_select_state *state);
static css_error match_selector_chain(css_select_ctx *ctx, 
		const css_selector *selector, css_select_state *state);
static css_error match_named_combinator(css_select_ctx *ctx, 
		css_combinator type, const css_selector *selector, 
		css_select_state *state, void *node, void **next_node);
static css_error match_universal_combinator(css_select_ctx *ctx, 
		css_combinator type, const css_selector *selector, 
		css_select_state *state, void *node, void **next_node);
static css_error match_details(css_select_ctx *ctx, void *node, 
		const css_selector_detail *detail, css_select_state *state, 
		bool *match);
static css_error match_detail(css_select_ctx *ctx, void *node, 
		const css_selector_detail *detail, css_select_state *state, 
		bool *match);
static css_error cascade_style(const css_style *style, css_select_state *state);

#ifdef DEBUG_CHAIN_MATCHING
static void dump_chain(const css_selector *selector);
#endif


/**
 * Create a selection context
 *
 * \param alloc   Memory (de)allocation function
 * \param pw      Client-specific private data
 * \param result  Pointer to location to receive created context
 * \return CSS_OK on success, appropriate error otherwise.
 */
css_error css_select_ctx_create(css_allocator_fn alloc, void *pw,
		css_select_ctx **result)
{
	css_select_ctx *c;

	if (alloc == NULL || result == NULL)
		return CSS_BADPARM;

	c = alloc(NULL, sizeof(css_select_ctx), pw);
	if (c == NULL)
		return CSS_NOMEM;

	c->n_sheets = 0;
	c->sheets = NULL;

	c->alloc = alloc;
	c->pw = pw;

	*result = c;

	return CSS_OK;
}

/**
 * Destroy a selection context
 *
 * \param ctx  The context to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_destroy(css_select_ctx *ctx)
{
	if (ctx == NULL)
		return CSS_BADPARM;

	if (ctx->sheets != NULL)
		ctx->alloc(ctx->sheets, 0, ctx->pw);

	ctx->alloc(ctx, 0, ctx->pw);

	return CSS_OK;
}

/**
 * Append a stylesheet to a selection context
 *
 * \param ctx     The context to append to
 * \param sheet   The sheet to append
 * \param origin  Origin of the sheet
 * \param media   Media types to which the sheet applies
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_append_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, css_origin origin,
		uint64_t media)
{
	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	return css_select_ctx_insert_sheet(ctx, sheet, ctx->n_sheets,
			origin, media);
}

/**
 * Insert a stylesheet into a selection context
 * 
 * \param ctx    The context to insert into
 * \param sheet  Sheet to insert
 * \param index  Index in context to insert sheet
 * \param origin  Origin of the sheet
 * \param media   Media types to which the sheet applies
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_insert_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, uint32_t index,
		css_origin origin, uint64_t media)
{
	css_select_sheet *temp;

	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	/* Inline styles cannot be inserted into a selection context */
	if (sheet->inline_style)
		return CSS_INVALID;

	/* Index must be in the range [0, n_sheets]
	 * The latter being equivalent to append */
	if (index > ctx->n_sheets)
		return CSS_INVALID;

	temp = ctx->alloc(ctx->sheets, 
			(ctx->n_sheets + 1) * sizeof(css_select_sheet),
			ctx->pw);
	if (temp == NULL)
		return CSS_NOMEM;

	ctx->sheets = temp;

	if (index < ctx->n_sheets) {
		memmove(&ctx->sheets[index + 1], &ctx->sheets[index],
			(ctx->n_sheets - index) * sizeof(css_select_sheet));
	}

	ctx->sheets[index].sheet = sheet;
	ctx->sheets[index].origin = origin;
	ctx->sheets[index].media = media;

	ctx->n_sheets++;

	return CSS_OK;
}

/**
 * Remove a sheet from a selection context
 *
 * \param ctx    The context to remove from
 * \param sheet  Sheet to remove
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_remove_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet)
{
	uint32_t index;

	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	for (index = 0; index < ctx->n_sheets; index++) {
		if (ctx->sheets[index].sheet == sheet)
			break;
	}

	if (index == ctx->n_sheets)
		return CSS_INVALID;

	memmove(&ctx->sheets[index], &ctx->sheets[index + 1],
			(ctx->n_sheets - index) * sizeof(css_select_sheet));

	ctx->n_sheets--;

	return CSS_OK;

}

/**
 * Count the number of top-level sheets in a selection context
 *
 * \param ctx    Context to consider
 * \param count  Pointer to location to receive count of sheets
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_count_sheets(css_select_ctx *ctx, uint32_t *count)
{
	if (ctx == NULL || count == NULL)
		return CSS_BADPARM;

	*count = ctx->n_sheets;

	return CSS_OK;
}

/**
 * Retrieve a sheet from a selection context
 *
 * \param ctx    Context to look in
 * \param index  Index in context to look
 * \param sheet  Pointer to location to receive sheet
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_get_sheet(css_select_ctx *ctx, uint32_t index,
		const css_stylesheet **sheet)
{
	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	if (index > ctx->n_sheets)
		return CSS_INVALID;

	*sheet = ctx->sheets[index].sheet;

	return CSS_OK;
}

/**
 * Select a style for the given node
 *
 * \param ctx             Selection context to use
 * \param node            Node to select style for
 * \param pseudo_element  Pseudo element to select for, instead
 * \param media           Currently active media types
 * \param inline_style    Corresponding inline style for node, or NULL
 * \param result          Pointer to style to populate (assumed clean)
 * \param handler         Dispatch table of handler functions
 * \param pw              Client-specific private data for handler functions
 * \return CSS_OK on success, appropriate error otherwise.
 *
 * In computing the style, no reference is made to the parent node's
 * style. Therefore, the resultant computed style is not ready for
 * immediate use, as some properties may be marked as inherited.
 * Use css_computed_style_compose() to obtain a fully computed style.
 *
 * This two-step approach to style computation is designed to allow
 * the client to store the partially computed style and efficiently
 * update the fully computed style for a node when layout changes.
 */
css_error css_select_style(css_select_ctx *ctx, void *node,
		uint32_t pseudo_element, uint64_t media,
		const css_stylesheet *inline_style,
		css_computed_style *result,
		css_select_handler *handler, void *pw)
{
	uint32_t i;
	css_error error;
	css_select_state state;
	void *parent = NULL;

	if (ctx == NULL || node == NULL || result == NULL || handler == NULL)
		return CSS_BADPARM;

	/* Set up the selection state */
	memset(&state, 0, sizeof(css_select_state));
	state.node = node;
	state.pseudo_element = pseudo_element;
	state.media = media;
	state.result = result;
	state.handler = handler;
	state.pw = pw;

	error = handler->parent_node(pw, node, &parent);
	if (error)
		return error;

	/* Iterate through the top-level stylesheets, selecting styles
	 * from those which apply to our current media requirements and
	 * are not disabled */
	for (i = 0; i < ctx->n_sheets; i++) {
		if ((ctx->sheets[i].media & media) != 0 &&
				ctx->sheets[i].sheet->disabled == false) {
			error = select_from_sheet(ctx, ctx->sheets[i].sheet, 
					ctx->sheets[i].origin, &state);
			if (error != CSS_OK)
                                goto cleanup;
		}
	}

	/* Consider any inline style for the node */
	if (inline_style != NULL) {
		css_rule_selector *sel = 
				(css_rule_selector *) inline_style->rule_list;

		/* Sanity check style */
		if (inline_style->rule_count != 1 ||
			inline_style->rule_list->type != CSS_RULE_SELECTOR || 
				inline_style->rule_list->items != 0) {
			error = CSS_INVALID;
			goto cleanup;
		}

		/* No bytecode if input was empty or wholly invalid */
		if (sel->style != NULL) {
			error = cascade_style(sel->style, &state);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* Take account of presentational hints and fix up any remaining
	 * unset properties. */
	for (i = 0; i < CSS_N_PROPERTIES; i++) {
		/* If the existing property value came from an author 
		 * stylesheet or a user sheet using !important, then leave 
		 * it alone. */
		if (state.props[i].set == false ||
				(state.props[i].origin != CSS_ORIGIN_AUTHOR &&
				state.props[i].important == false)) {
			error = set_hint(&state, i);
			if (error != CSS_OK)
				goto cleanup;
		}

		/* If the property is still unset or it's set to inherit and 
		 * we're the root element, then set it to its initial value. */
		if (state.props[i].set == false || (parent == NULL && 
				state.props[i].inherit == true)) {
			error = set_initial(&state, i, parent);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* If this is the root element, then we must ensure that all
	 * length values are absolute, display and float are correctly 
	 * computed, and the default border-{top,right,bottom,left}-color 
	 * is set to the computed value of color. */
	if (parent == NULL) {
		error = compute_absolute_values(NULL, result, 
				handler->compute_font_size, pw);
		if (error != CSS_OK)
			goto cleanup;
	}

	error = CSS_OK;
cleanup:
        if (ctx->n_sheets > 0 && ctx->sheets[0].sheet != NULL) {
                if (state.universal != NULL)
                        lwc_string_unref(state.universal);
                if (state.first_child != NULL)
                        lwc_string_unref(state.first_child);
                if (state.link != NULL)
                        lwc_string_unref(state.link);
                if (state.visited != NULL)
                        lwc_string_unref(state.visited);
                if (state.hover != NULL)
                        lwc_string_unref(state.hover);
                if (state.active != NULL)
                        lwc_string_unref(state.active);
                if (state.focus != NULL)
                        lwc_string_unref(state.focus);
                if (state.first_line != NULL)
                        lwc_string_unref(state.first_line);
                if (state.first_letter != NULL)
                        lwc_string_unref(state.first_letter);
                if (state.before != NULL)
                        lwc_string_unref(state.before);
                if (state.after != NULL)
                        lwc_string_unref(state.after);
        }
        return error;
}

/******************************************************************************
 * Selection engine internals below here                                      *
 ******************************************************************************/

css_error set_hint(css_select_state *state, uint32_t i)
{
	css_hint hint;
	css_error error;

	/* Initialise hint */
	memset(&hint, 0, sizeof(css_hint));

	/* Retrieve this property's hint from the client */
	error = state->handler->node_presentational_hint(state->pw, 
			state->node, i, &hint);
	if (error != CSS_OK)
		return (error == CSS_PROPERTY_NOT_SET) ? CSS_OK : error;

	/* Hint defined -- set it in the result */
	error = prop_dispatch[i].set_from_hint(&hint, state->result);
	if (error != CSS_OK)
		return error;

	/* Keep selection state in sync with reality */
	state->props[i].set = 1;
	state->props[i].specificity = 0;
	state->props[i].origin = CSS_ORIGIN_AUTHOR;
	state->props[i].important = 0;
	state->props[i].inherit = (hint.status == 0);

	return CSS_OK;
}

css_error set_initial(css_select_state *state, uint32_t i, void *parent)
{
	css_error error;

	/* Do nothing if this property is inherited (the default state 
	 * of a clean computed style is for everything to be set to inherit)
	 *
	 * If the node is tree root, everything should be defaulted.
	 */
	if (prop_dispatch[i].inherited == false || parent == NULL) {
		/* Remaining properties are neither inherited nor 
		 * already set. Thus, we set them to their initial 
		 * values here. Except, however, if the property in 
		 * question resides in one of the extension blocks and 
		 * the extension block has yet to be allocated. In that 
		 * case, we do nothing and leave it to the property 
		 * accessors to return the initial values for the 
		 * property.
		 */
		if (prop_dispatch[i].group == GROUP_NORMAL) {
			error = prop_dispatch[i].initial(state);
			if (error != CSS_OK)
				return error;
		} else if (prop_dispatch[i].group == GROUP_UNCOMMON &&
				state->result->uncommon != NULL) {
			error = prop_dispatch[i].initial(state);
			if (error != CSS_OK)
				return error;
		} else if (prop_dispatch[i].group == GROUP_PAGE &&
				state->result->page != NULL) {
			error = prop_dispatch[i].initial(state);
			if (error != CSS_OK)
				return error;
		} else if (prop_dispatch[i].group == GROUP_AURAL &&
				state->result->aural != NULL) {
			error = prop_dispatch[i].initial(state);
			if (error != CSS_OK)
				return error;
		}
	}

	return CSS_OK;
}

css_error select_from_sheet(css_select_ctx *ctx, const css_stylesheet *sheet, 
		css_origin origin, css_select_state *state)
{
	const css_stylesheet *s = sheet;
	const css_rule *rule = s->rule_list;
	uint32_t sp = 0;
#define IMPORT_STACK_SIZE 256
	const css_rule *import_stack[IMPORT_STACK_SIZE];

	do {
		/* Find first non-charset rule, if we're at the list head */
		if (rule == s->rule_list) {
			for (; rule != NULL; rule = rule->next) {
				if (rule->type != CSS_RULE_CHARSET)
					break;
			}
		}

		if (rule != NULL && rule->type == CSS_RULE_IMPORT) {
			/* Current rule is an import */
			const css_rule_import *import = 
					(const css_rule_import *) rule;

			if (import->sheet != NULL &&
					(import->media & state->media) != 0) {
				/* It's applicable, so process it */
				assert(sp < IMPORT_STACK_SIZE - 1);

				import_stack[sp++] = rule;

				s = import->sheet;
				rule = s->rule_list;
			} else {
				/* Not applicable; skip over it */
				rule = rule->next;
			}
		} else {
			/* Gone past import rules in this sheet */
			css_error error;

			/* Process this sheet */
			state->sheet = s;
			state->current_origin = origin;

			error = intern_strings_for_sheet(ctx, s, state);
			if (error != CSS_OK)
				return error;

			error = match_selectors_in_sheet(ctx, s, state);
			if (error != CSS_OK)
				return error;

			/* Find next sheet to process */
			if (sp > 0) {
				sp--;
				rule = import_stack[sp]->next;
				s = import_stack[sp]->parent;
			} else {
				s = NULL;
			}
		}
	} while (s != NULL);

	return CSS_OK;
}

css_error intern_strings_for_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, css_select_state *state)
{
	lwc_error error;

	UNUSED(ctx);
        UNUSED(sheet);

	/* Universal selector */
        if (state->universal != NULL)
                return CSS_OK;

	error = lwc_intern_string("*", SLEN("*"), &state->universal);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	/* Pseudo classes */
	error = lwc_intern_string(
			"first-child", SLEN("first-child"), 
			&state->first_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"link", SLEN("link"), 
			&state->link);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"visited", SLEN("visited"), 
			&state->visited);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"hover", SLEN("hover"), 
			&state->hover);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"active", SLEN("active"), 
			&state->active);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"focus", SLEN("focus"), 
			&state->focus);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	/* Pseudo elements */
	error = lwc_intern_string(
			"first-line", SLEN("first-line"), 
			&state->first_line);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"first_letter", SLEN("first-letter"),
			&state->first_letter);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"before", SLEN("before"), 
			&state->before);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"after", SLEN("after"), 
			&state->after);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	return CSS_OK;
}

static bool _selectors_pending(const css_selector **node,
		const css_selector **id, const css_selector ***classes,
		uint32_t n_classes, const css_selector **univ)
{
	bool pending = false;
	uint32_t i;

	pending |= *node != NULL;
	pending |= *id != NULL;
	pending |= *univ != NULL;

	if (classes != NULL && n_classes > 0) {
		for (i = 0; i < n_classes; i++)
			pending |= *(classes[i]) != NULL;
	}

	return pending;
}

static inline bool _selector_less_specific(const css_selector *ref, 
		const css_selector *cand)
{
	bool result = true;

	if (cand == NULL)
		return false;

	if (ref == NULL)
		return true;

	/* Sort by specificity */
	if (cand->specificity < ref->specificity) {
		result = true;
	} else if (ref->specificity < cand->specificity) {
		result = false;
	} else {
		/* Then by rule index -- earliest wins */
		if (cand->rule->index < ref->rule->index)
			result = true;
		else
			result = false;
	}

	return result;
}

static const css_selector *_selector_next(const css_selector **node,
		const css_selector **id, const css_selector ***classes,
		uint32_t n_classes, const css_selector **univ)
{
	const css_selector *ret = NULL;

	if (_selector_less_specific(ret, *node))
		ret = *node;

	if (_selector_less_specific(ret, *id))
		ret = *id;

	if (_selector_less_specific(ret, *univ))
		ret = *univ;

	if (classes != NULL && n_classes > 0) {
		uint32_t i;

		for (i = 0; i < n_classes; i++) {
			if (_selector_less_specific(ret, *(classes[i])))
				ret = *(classes[i]);
		}
	}

	return ret;
}

css_error match_selectors_in_sheet(css_select_ctx *ctx, 
		const css_stylesheet *sheet, css_select_state *state)
{
	static const css_selector *empty_selector = NULL;
	lwc_string *element = NULL;
	lwc_string *id = NULL;
	lwc_string **classes = NULL;
	uint32_t n_classes = 0, i = 0;
	const css_selector **node_selectors = &empty_selector;
	css_selector_hash_iterator node_iterator;
	const css_selector **id_selectors = &empty_selector;
	css_selector_hash_iterator id_iterator;
	const css_selector ***class_selectors = NULL;
	css_selector_hash_iterator class_iterator;
	const css_selector **univ_selectors = &empty_selector;
	css_selector_hash_iterator univ_iterator;
	css_error error;

	/* Get node's name */
	error = state->handler->node_name(state->pw, state->node, 
			&element);
	if (error != CSS_OK)
		return error;

	/* Get node's ID, if any */
	error = state->handler->node_id(state->pw, state->node,
			&id);
	if (error != CSS_OK)
		goto cleanup;

	/* Get node's classes, if any */
	/** \todo Do we really want to force the client to allocate a new array 
	 * every time we call this? It seems hugely inefficient, given they can 
	 * cache the data. */
	error = state->handler->node_classes(state->pw, state->node,
			&classes, &n_classes);
	if (error != CSS_OK)
		goto cleanup;

	/* Find hash chain that applies to current node */
	error = css_selector_hash_find(sheet->selectors, element, 
			&node_iterator, &node_selectors);
	if (error != CSS_OK)
                goto cleanup;

	if (classes != NULL && n_classes > 0) {
		/* Find hash chains for node classes */
		class_selectors = ctx->alloc(NULL, 
				n_classes * sizeof(css_selector **), ctx->pw);
		if (class_selectors == NULL) {
			error = CSS_NOMEM;
			goto cleanup;
		}

		for (i = 0; i < n_classes; i++) {
			error = css_selector_hash_find_by_class(
					sheet->selectors, classes[i],
					&class_iterator, &class_selectors[i]);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	if (id != NULL) {
		/* Find hash chain for node ID */
		error = css_selector_hash_find_by_id(sheet->selectors, id,
				&id_iterator, &id_selectors);
		if (error != CSS_OK)
			goto cleanup;
	}

	/* Find hash chain for universal selector */
	error = css_selector_hash_find_universal(sheet->selectors,  
			&univ_iterator, &univ_selectors);
	if (error != CSS_OK)
		goto cleanup;

	/* Process matching selectors, if any */
	while (_selectors_pending(node_selectors, id_selectors, 
			class_selectors, n_classes, univ_selectors)) {
		const css_selector *selector;
		css_rule *rule, *parent;
		bool process = true;

		/* Selectors must be matched in ascending order of specificity
		 * and rule index. (c.f. outranks_existing())
		 *
		 * Pick the least specific/earliest occurring selector.
		 */
		selector = _selector_next(node_selectors, id_selectors,
				class_selectors, n_classes, univ_selectors);

		/* Ignore any selectors contained in rules which are a child 
		 * of an @media block that doesn't match the current media 
		 * requirements. */
		for (rule = selector->rule; rule != NULL; rule = parent) {
			if (rule->type == CSS_RULE_MEDIA && 
					(((css_rule_media *) rule)->media & 
					state->media) == 0) {
				process = false;
				break;
			}

			if (rule->ptype != CSS_RULE_PARENT_STYLESHEET)
				parent = rule->parent;
			else
				parent = NULL;
		}

		if (process) {
			error = match_selector_chain(ctx, selector, state);
			if (error != CSS_OK)
				goto cleanup;
		}

		/* Advance to next selector in whichever chain we extracted 
		 * the processed selector from. */
		if (selector == *node_selectors) {
			error = node_iterator(sheet->selectors, 
					node_selectors,	&node_selectors);
		} else if (selector == *id_selectors) {
			error = id_iterator(sheet->selectors,
					id_selectors, &id_selectors);
		} else if (selector == *univ_selectors) {
			error = univ_iterator(sheet->selectors,
					univ_selectors, &univ_selectors);
		} else {
			for (i = 0; i < n_classes; i++) {
				if (selector == *(class_selectors[i])) {
					error = class_iterator(sheet->selectors,
							class_selectors[i], 
							&class_selectors[i]);
					break;
				}
			}
		}

		if (error != CSS_OK)
			goto cleanup;
	}

        error = CSS_OK;
cleanup:
	if (class_selectors != NULL)
		ctx->alloc(class_selectors, 0, ctx->pw);

	if (classes != NULL) {
		for (i = 0; i < n_classes; i++)
			lwc_string_unref(classes[i]);

		ctx->alloc(classes, 0, ctx->pw);
	}

	if (id != NULL)
		lwc_string_unref(id);

        lwc_string_unref(element);
        return error;
}

css_error match_selector_chain(css_select_ctx *ctx, 
		const css_selector *selector, css_select_state *state)
{
	const css_selector *s = selector;
	void *node = state->node;
	css_error error;

#ifdef DEBUG_CHAIN_MATCHING
	fprintf(stderr, "matching: ");
	dump_chain(selector);
	fprintf(stderr, "\n");
#endif

	do {
		void *next_node = NULL;
		const css_selector_detail *detail = &s->data;
		bool match = false;

		/* If this is the first selector in the chain, we must match 
		 * its details. The details of subsequent selectors will be 
		 * matched when processing the combinator. */
		if (s == selector) {
			/* Match details on this selector */
			error = match_details(ctx, node, detail, state, &match);
			if (error != CSS_OK)
				return error;

			/* Details don't match, so reject selector chain */
			if (match == false)
				return CSS_OK;

		}

		/* Consider any combinator on this selector */
		if (s->data.comb != CSS_COMBINATOR_NONE &&
				s->combinator->data.name != state->universal) {
			/* Named combinator */
			error = match_named_combinator(ctx, s->data.comb, 
					s->combinator, state, node, &next_node);
			if (error != CSS_OK)
				return error;

			/* No match for combinator, so reject selector chain */
			if (next_node == NULL)
				return CSS_OK;
		} else if (s->data.comb != CSS_COMBINATOR_NONE &&
				s->combinator->data.name == state->universal) {
			/* Universal combinator */
			error = match_universal_combinator(ctx, s->data.comb, 
					s->combinator, state, node, &next_node);
			if (error != CSS_OK)
				return error;

			/* No match for combinator, so reject selector chain */
			if (next_node == NULL)
				return CSS_OK;
		}

		/* Details matched, so progress to combining selector */
		s = s->combinator;
		node = next_node;
	} while (s != NULL);

	/* If we got here, then the entire selector chain matched, so cascade */
	state->current_specificity = selector->specificity;

	/* No bytecode if rule body is empty or wholly invalid */
	if (((css_rule_selector *) selector->rule)->style == NULL)
		return CSS_OK;

	return cascade_style(((css_rule_selector *) selector->rule)->style, 
			state);
}

css_error match_named_combinator(css_select_ctx *ctx, css_combinator type,
		const css_selector *selector, css_select_state *state, 
		void *node, void **next_node)
{
	const css_selector_detail *detail = &selector->data;
	void *n = node;
	css_error error;

	do {
		bool match = false;

		/* Find candidate node */
		switch (type) {
		case CSS_COMBINATOR_ANCESTOR:
			error = state->handler->named_ancestor_node(state->pw, 
					n, selector->data.name, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_PARENT:
			error = state->handler->named_parent_node(state->pw, 
					n, selector->data.name, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_SIBLING:
			error = state->handler->named_sibling_node(state->pw, 
					n, selector->data.name, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_NONE:
			break;
		}

		if (n != NULL) {
			/* Match its details */
			error = match_details(ctx, n, detail, state, &match);
			if (error != CSS_OK)
				return error;

			/* If we found a match, use it */
			if (match == true)
				break;

			/* For parent and sibling selectors, only adjacent 
			 * nodes are valid. Thus, if we failed to match, 
			 * give up. */
			if (type == CSS_COMBINATOR_PARENT ||
					type == CSS_COMBINATOR_SIBLING)
				n = NULL;
		}
	} while (n != NULL);

	*next_node = n;

	return CSS_OK;
}

css_error match_universal_combinator(css_select_ctx *ctx, css_combinator type,
		const css_selector *selector, css_select_state *state,
		void *node, void **next_node)
{
	const css_selector_detail *detail = &selector->data;
	void *n = node;
	css_error error;

	do {
		bool match = false;

		/* Find candidate node */
		switch (type) {
		case CSS_COMBINATOR_ANCESTOR:
		case CSS_COMBINATOR_PARENT:
			error = state->handler->parent_node(state->pw, n, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_SIBLING:
			error = state->handler->sibling_node(state->pw, n, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_NONE:
			break;
		}

		if (n != NULL) {
			/* Match its details */
			error = match_details(ctx, n, detail, state, &match);
			if (error != CSS_OK)
				return error;

			/* If we found a match, use it */
			if (match == true)
				break;

			/* For parent and sibling selectors, only adjacent
			 * nodes are valid. Thus, if we failed to match,
			 * give up. */
			if (type == CSS_COMBINATOR_PARENT ||
					type == CSS_COMBINATOR_SIBLING)
				n = NULL;
		}
	} while (n != NULL);

	*next_node = n;

	return CSS_OK;
}

css_error match_details(css_select_ctx *ctx, void *node, 
		const css_selector_detail *detail, css_select_state *state, 
		bool *match)
{
	css_error error;

	/* We match by default (if there are no details than the element
	 * selector, then we must match) */
	*match = true;

	/** \todo Some details are easier to test than others (e.g. dashmatch 
	 * actually requires looking at data rather than simply comparing 
	 * pointers). Should we consider sorting the detail list such that the 
	 * simpler details come first (and thus the expensive match routines 
	 * can be avoided unless absolutely necessary)? */

	do {
		error = match_detail(ctx, node, detail, state, match);
		if (error != CSS_OK)
			return error;

		/* Detail doesn't match, so reject selector chain */
		if (*match == false)
			return CSS_OK;

		if (detail->next)
			detail++;
		else
			detail = NULL;
	} while (detail != NULL);

	return CSS_OK;
}

css_error match_detail(css_select_ctx *ctx, void *node, 
		const css_selector_detail *detail, css_select_state *state, 
		bool *match)
{
	css_error error = CSS_OK;

	UNUSED(ctx);

	switch (detail->type) {
	case CSS_SELECTOR_ELEMENT:
		if (lwc_string_length(detail->name) == 1 &&
				lwc_string_data(detail->name)[0] == '*') {
			*match = true;
		} else {
			error = state->handler->node_has_name(state->pw, node,
					detail->name, match);
		}
		break;
	case CSS_SELECTOR_CLASS:
		error = state->handler->node_has_class(state->pw, node,
				detail->name, match);
		break;
	case CSS_SELECTOR_ID:
		error = state->handler->node_has_id(state->pw, node,
				detail->name, match);
		break;
	case CSS_SELECTOR_PSEUDO_CLASS:
		if (detail->name == state->first_child) {
			error = state->handler->node_is_first_child(state->pw, 
					node, match);
		} else if (detail->name == state->link) {
			error = state->handler->node_is_link(state->pw,
					node, match);
		} else if (detail->name == state->visited) {
			error = state->handler->node_is_visited(state->pw,
					node, match);
		} else if (detail->name == state->hover) {
			error = state->handler->node_is_hover(state->pw,
					node, match);
		} else if (detail->name == state->active) {
			error = state->handler->node_is_active(state->pw,
					node, match);
		} else if (detail->name == state->focus) {
			error = state->handler->node_is_focus(state->pw,
					node, match);
		} else
			*match = false;
		break;
	case CSS_SELECTOR_PSEUDO_ELEMENT:
		if (detail->name == state->first_line && 
				state->pseudo_element ==
				CSS_PSEUDO_ELEMENT_FIRST_LINE)
			*match = true;
		else if (detail->name == state->first_letter &&
				state->pseudo_element ==
				CSS_PSEUDO_ELEMENT_FIRST_LETTER)
			*match = true;
		else if (detail->name == state->before &&
				state->pseudo_element ==
				CSS_PSEUDO_ELEMENT_BEFORE)
			*match = true;
		else if (detail->name == state->after &&
				state->pseudo_element ==
				CSS_PSEUDO_ELEMENT_AFTER)
			*match = true;
		else
			*match = false;
		break;
	case CSS_SELECTOR_ATTRIBUTE:
		error = state->handler->node_has_attribute(state->pw, node,
				detail->name, match);
		break;
	case CSS_SELECTOR_ATTRIBUTE_EQUAL:
		error = state->handler->node_has_attribute_equal(state->pw, 
				node, detail->name, detail->value, match);
		break;
	case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
		error = state->handler->node_has_attribute_dashmatch(state->pw,
				node, detail->name, detail->value, match);
		break;
	case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
		error = state->handler->node_has_attribute_includes(state->pw, 
				node, detail->name, detail->value, match);
		break;
	}

	return error;
}

css_error cascade_style(const css_style *style, css_select_state *state)
{
	css_style s = *style;

	while (s.length > 0) {
		opcode_t op;
		css_error error;
		uint32_t opv = *((uint32_t *) s.bytecode);

		advance_bytecode(&s, sizeof(opv));

		op = getOpcode(opv);

		error = prop_dispatch[op].cascade(opv, &s, state);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

bool outranks_existing(uint16_t op, bool important, css_select_state *state,
		bool inherit)
{
	prop_state *existing = &state->props[op];
	bool outranks = false;

	/* Sorting on origin & importance gives the following:
	 * 
	 *           | UA, - | UA, i | USER, - | USER, i | AUTHOR, - | AUTHOR, i
	 *           |----------------------------------------------------------
	 * UA    , - |   S       S       Y          Y         Y           Y
	 * UA    , i |   S       S       Y          Y         Y           Y
	 * USER  , - |   -       -       S          Y         Y           Y
	 * USER  , i |   -       -       -          S         -           -
	 * AUTHOR, - |   -       -       -          Y         S           Y
	 * AUTHOR, i |   -       -       -          Y         -           S
	 *
	 * Where the columns represent the origin/importance of the property 
	 * being considered and the rows represent the origin/importance of 
	 * the existing property.
	 *
	 * - means that the existing property must be preserved
	 * Y means that the new property must be applied
	 * S means that the specificities of the rules must be considered.
	 *
	 * If specificities are considered, the highest specificity wins.
	 * If specificities are equal, then the rule defined last wins.
	 *
	 * We have no need to explicitly consider the ordering of rules if
	 * the specificities are the same because:
	 *
	 * a) We process stylesheets in order
	 * b) The selector hash chains within a sheet are ordered such that 
	 *    more specific rules come after less specific ones and, when
	 *    specificities are identical, rules defined later occur after
	 *    those defined earlier.
	 *
	 * Therefore, where we consider specificity, below, the property 
	 * currently being considered will always be applied if its specificity
	 * is greater than or equal to that of the existing property.
	 */

	if (existing->set == 0) {
		/* Property hasn't been set before, new one wins */
		outranks = true;
	} else {
		assert(CSS_ORIGIN_UA < CSS_ORIGIN_USER);
		assert(CSS_ORIGIN_USER < CSS_ORIGIN_AUTHOR);

		if (existing->origin < state->current_origin) {
			/* New origin has more weight than existing one.
			 * Thus, new property wins, except when the existing 
			 * one is USER, i. */
			if (existing->important == 0 ||
					existing->origin != CSS_ORIGIN_USER) {
				outranks = true;
			}
		} else if (existing->origin == state->current_origin) {
			/* Origins are identical, consider importance, except 
			 * for UA stylesheets, when specificity is always 
			 * considered (as importance is meaningless) */
			if (existing->origin == CSS_ORIGIN_UA) {
				if (state->current_specificity >=
						existing->specificity) {
					outranks = true;
				}
			} else if (existing->important == 0 && important) {
				/* New is more important than old. */
				outranks = true;
			} else if (existing->important && important == false) {
				/* Old is more important than new */
			} else {
				/* Same importance, consider specificity */
				if (state->current_specificity >=
						existing->specificity) {
					outranks = true;
				}
			}
		} else {
			/* Existing origin has more weight than new one.
			 * Thus, existing property wins, except when the new
			 * one is USER, i. */
			if (state->current_origin == CSS_ORIGIN_USER &&
					important) {
				outranks = true;
			}
		}
	}

	if (outranks) {
		/* The new property is about to replace the old one.
		 * Update our state to reflect this. */
		existing->set = 1;
		existing->specificity = state->current_specificity;
		existing->origin = state->current_origin;
		existing->important = important;
		existing->inherit = inherit;
	}

	return outranks;
}

/******************************************************************************
 * Debug helpers                                                              *
 ******************************************************************************/
#ifdef DEBUG_CHAIN_MATCHING
void dump_chain(const css_selector *selector)
{
	const css_selector_detail *detail = &selector->data;

	if (selector->data.comb != CSS_COMBINATOR_NONE)
		dump_chain(selector->combinator);

	if (selector->data.comb == CSS_COMBINATOR_ANCESTOR)
		fprintf(stderr, " ");
	else if (selector->data.comb == CSS_COMBINATOR_SIBLING)
		fprintf(stderr, " + ");
	else if (selector->data.comb == CSS_COMBINATOR_PARENT)
		fprintf(stderr, " > ");

	do {
		switch (detail->type) {
		case CSS_SELECTOR_ELEMENT:
			if (lwc_string_length(detail->name) == 1 && 
				lwc_string_data(detail->name)[0] == '*' &&
					detail->next == 1) {
				break;
			}
			fprintf(stderr, "%.*s",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name));
			break;
		case CSS_SELECTOR_CLASS:
			fprintf(stderr, ".%.*s",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name));
			break;
		case CSS_SELECTOR_ID:
			fprintf(stderr, "#%.*s",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name));
			break;
		case CSS_SELECTOR_PSEUDO_CLASS:
		case CSS_SELECTOR_PSEUDO_ELEMENT:
			fprintf(stderr, ":%.*s",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name));

			if (detail->value != NULL) {
				fprintf(stderr, "(%.*s)",
					(int) lwc_string_length(detail->value),
					lwc_string_data(detail->value));
			}
			break;
		case CSS_SELECTOR_ATTRIBUTE:
			fprintf(stderr, "[%.*s]",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name));
			break;
		case CSS_SELECTOR_ATTRIBUTE_EQUAL:
			fprintf(stderr, "[%.*s=\"%.*s\"]",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name),
					(int) lwc_string_length(detail->value),
					lwc_string_data(detail->value));
			break;
		case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
			fprintf(stderr, "[%.*s|=\"%.*s\"]",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name),
					(int) lwc_string_length(detail->value),
					lwc_string_data(detail->value));
			break;
		case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
			fprintf(stderr, "[%.*s~=\"%.*s\"]",
					(int) lwc_string_length(detail->name),
					lwc_string_data(detail->name),
					(int) lwc_string_length(detail->value),
					lwc_string_data(detail->value));
			break;
		}

		if (detail->next)
			detail++;
		else
			detail = NULL;
	} while (detail);
}
#endif

