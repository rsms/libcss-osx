/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include "stylesheet.h"
#include "bytecode/bytecode.h"
#include "parse/language.h"
#include "utils/parserutilserror.h"
#include "utils/utils.h"
#include "select/dispatch.h"

static css_error _add_selectors(css_stylesheet *sheet, css_rule *rule);
static css_error _remove_selectors(css_stylesheet *sheet, css_rule *rule);
static size_t _rule_size(const css_rule *rule);

/**
 * Create a stylesheet
 *
 * \param level		   The language level of the stylesheet
 * \param charset	   The charset of the stylesheet data, or NULL to detect
 * \param url		   URL of stylesheet
 * \param title		   Title of stylesheet
 * \param allow_quirks	   Permit quirky parsing of stylesheets
 * \param inline_style	   This stylesheet is an inline style
 * \param alloc		   Memory (de)allocation function
 * \param alloc_pw	   Client private data for alloc
 * \param resolve	   URL resolution function
 * \param resolve_pw	   Client private data for resolve
 * \param stylesheet	   Pointer to location to receive stylesheet
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters,
 *	   CSS_NOMEM on memory exhaustion
 */
css_error css_stylesheet_create(css_language_level level,
		const char *charset, const char *url, const char *title,
		bool allow_quirks, bool inline_style,
		css_allocator_fn alloc, void *alloc_pw, 
		css_url_resolution_fn resolve, void *resolve_pw,
		css_stylesheet **stylesheet)
{
	css_parser_optparams params;
	css_error error;
	css_stylesheet *sheet;
	size_t len;

	if (url == NULL || alloc == NULL || 
			resolve == NULL || stylesheet == NULL)
		return CSS_BADPARM;

	sheet = alloc(NULL, sizeof(css_stylesheet), alloc_pw);
	if (sheet == NULL)
		return CSS_NOMEM;

	memset(sheet, 0, sizeof(css_stylesheet));
	
	sheet->inline_style = inline_style;

	if (inline_style) {
		error = css_parser_create_for_inline_style(charset, 
			charset ? CSS_CHARSET_DICTATED : CSS_CHARSET_DEFAULT,
			alloc, alloc_pw, &sheet->parser);
	} else {
		error = css_parser_create(charset,
			charset ? CSS_CHARSET_DICTATED : CSS_CHARSET_DEFAULT,
			alloc, alloc_pw, &sheet->parser);
	}

	if (error != CSS_OK) {
		alloc(sheet, 0, alloc_pw);
		return error;
	}

	sheet->quirks_allowed = allow_quirks;
	if (allow_quirks) {
		params.quirks = true;

		error = css_parser_setopt(sheet->parser, CSS_PARSER_QUIRKS,
				&params);
		if (error != CSS_OK) {
			css_parser_destroy(sheet->parser);
			alloc(sheet, 0, alloc_pw);
			return error;
		}
	}

	sheet->level = level;
	error = css_language_create(sheet, sheet->parser, alloc, alloc_pw,
			&sheet->parser_frontend);
	if (error != CSS_OK) {
		css_parser_destroy(sheet->parser);
		alloc(sheet, 0, alloc_pw);
		return error;
	}

	error = css_selector_hash_create(alloc, alloc_pw, 
			&sheet->selectors);
	if (error != CSS_OK) {
		css_language_destroy(sheet->parser_frontend);
		css_parser_destroy(sheet->parser);
		alloc(sheet, 0, alloc_pw);
		return error;
	}

	len = strlen(url) + 1;
	sheet->url = alloc(NULL, len, alloc_pw);
	if (sheet->url == NULL) {
		css_selector_hash_destroy(sheet->selectors);
		css_language_destroy(sheet->parser_frontend);
		css_parser_destroy(sheet->parser);
		alloc(sheet, 0, alloc_pw);
		return CSS_NOMEM;
	}
	memcpy(sheet->url, url, len);

	if (title != NULL) {
		len = strlen(title) + 1;
		sheet->title = alloc(NULL, len, alloc_pw);
		if (sheet->title == NULL) {
			alloc(sheet->url, 0, alloc_pw);
			css_selector_hash_destroy(sheet->selectors);
			css_language_destroy(sheet->parser_frontend);
			css_parser_destroy(sheet->parser);
			alloc(sheet, 0, alloc_pw);
			return CSS_NOMEM;
		}
		memcpy(sheet->title, title, len);
	}

	sheet->resolve = resolve;
	sheet->resolve_pw = resolve_pw;

	sheet->alloc = alloc;
	sheet->pw = alloc_pw;

	sheet->size = sizeof(css_stylesheet) + strlen(sheet->url);
	if (sheet->title != NULL)
		sheet->size += strlen(sheet->title);

	*stylesheet = sheet;

	return CSS_OK;
}

/**
 * Destroy a stylesheet
 *
 * \param sheet	 The stylesheet to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_destroy(css_stylesheet *sheet)
{
	uint32_t bucket;
	css_rule *r, *s;

	if (sheet == NULL)
		return CSS_BADPARM;

	if (sheet->title != NULL)
		sheet->alloc(sheet->title, 0, sheet->pw);

	sheet->alloc(sheet->url, 0, sheet->pw);

	for (r = sheet->rule_list; r != NULL; r = s) {
		s = r->next;

		/* Detach from list */
		r->parent = NULL;
		r->prev = NULL;
		r->next = NULL;

		css_stylesheet_rule_destroy(sheet, r);
	}

	css_selector_hash_destroy(sheet->selectors);

	/* Release cached free styles */
	for (bucket = 0; bucket != N_ELEMENTS(sheet->free_styles); bucket++) {
		while (sheet->free_styles[bucket] != NULL) {
			css_style *s = sheet->free_styles[bucket];

			sheet->free_styles[bucket] = s->bytecode;

			sheet->alloc(s, 0, sheet->pw);
		}
	}

	/* These two may have been destroyed when parsing completed */
	if (sheet->parser_frontend != NULL)
		css_language_destroy(sheet->parser_frontend);

	if (sheet->parser != NULL)
		css_parser_destroy(sheet->parser);

	sheet->alloc(sheet, 0, sheet->pw);

	return CSS_OK;
}

/**
 * Append source data to a stylesheet
 *
 * \param sheet	 The stylesheet to append data to
 * \param data	 Pointer to data to append
 * \param len	 Length, in bytes, of data to append
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_append_data(css_stylesheet *sheet,
		const uint8_t *data, size_t len)
{
	if (sheet == NULL || data == NULL)
		return CSS_BADPARM;

	if (sheet->parser == NULL)
		return CSS_INVALID;

	return css_parser_parse_chunk(sheet->parser, data, len);
}

/**
 * Flag that the last of a stylesheet's data has been seen
 *
 * \param sheet	 The stylesheet in question
 * \return CSS_OK on success,
 *	   CSS_IMPORTS_PENDING if there are imports pending,
 *	   appropriate error otherwise
 */
css_error css_stylesheet_data_done(css_stylesheet *sheet)
{
	const css_rule *r;
	uint32_t bucket;
	css_error error;

	if (sheet == NULL)
		return CSS_BADPARM;

	if (sheet->parser == NULL)
		return CSS_INVALID;

	error = css_parser_completed(sheet->parser);
	if (error != CSS_OK)
		return error;

	/* Destroy the parser, as it's no longer needed */
	css_language_destroy(sheet->parser_frontend);
	css_parser_destroy(sheet->parser);

	sheet->parser_frontend = NULL;
	sheet->parser = NULL;

	/* Release cached free styles */
	for (bucket = 0; bucket != N_ELEMENTS(sheet->free_styles); bucket++) {
		while (sheet->free_styles[bucket] != NULL) {
			css_style *s = sheet->free_styles[bucket];

			sheet->free_styles[bucket] = s->bytecode;

			sheet->alloc(s, 0, sheet->pw);
		}
	}

	/* Determine if there are any pending imports */
	for (r = sheet->rule_list; r != NULL; r = r->next) {
		const css_rule_import *i = (const css_rule_import *) r;

		if (r->type != CSS_RULE_UNKNOWN &&
				r->type != CSS_RULE_CHARSET &&
				r->type != CSS_RULE_IMPORT)
			break;

		if (r->type == CSS_RULE_IMPORT && i->sheet == NULL)
			return CSS_IMPORTS_PENDING;
	}

	return CSS_OK;
}

/**
 * Retrieve the next pending import for the parent stylesheet
 *
 * \param parent  Parent stylesheet
 * \param url	  Pointer to object to be populated with details of URL of 
 *		  imported stylesheet (potentially relative)
 * \param media	  Pointer to location to receive applicable media types for
 *		  imported sheet,
 * \return CSS_OK on success,
 *	   CSS_INVALID if there are no pending imports remaining
 *
 * The client must resolve the absolute URL of the imported stylesheet,
 * using the parent's URL as the base. It must then fetch the imported
 * stylesheet, and parse it to completion, including fetching any stylesheets
 * it may import. The resultant sheet must then be registered with the
 * parent using css_stylesheet_register_import().
 *
 * The client must then call this function again, to determine if there
 * are any further imports for the parent stylesheet, and, if so,
 * process them as described above.
 *
 * If the client is unable to fetch an imported stylesheet, it must
 * register an empty stylesheet with the parent in its place.
 */
css_error css_stylesheet_next_pending_import(css_stylesheet *parent,
		lwc_string **url, uint64_t *media)
{
	const css_rule *r;

	if (parent == NULL || url == NULL || media == NULL)
		return CSS_BADPARM;

	for (r = parent->rule_list; r != NULL; r = r->next) {
		const css_rule_import *i = (const css_rule_import *) r;

		if (r->type != CSS_RULE_UNKNOWN &&
				r->type != CSS_RULE_CHARSET &&
				r->type != CSS_RULE_IMPORT)
			break;

		if (r->type == CSS_RULE_IMPORT && i->sheet == NULL) {
			*url = lwc_string_ref(i->url);
			*media = i->media;

			return CSS_OK;
		}
	}

	return CSS_INVALID;
}

/**
 * Register an imported stylesheet with its parent
 *
 * \param parent  Parent stylesheet
 * \param import  Imported sheet
 * \return CSS_OK on success, 
 *	   CSS_INVALID if there are no outstanding imports,
 *	   appropriate error otherwise.
 *
 * Ownership of the imported stylesheet is retained by the client.
 */
css_error css_stylesheet_register_import(css_stylesheet *parent,
		css_stylesheet *import)
{
	css_rule *r;

	if (parent == NULL || import == NULL)
		return CSS_BADPARM;

	for (r = parent->rule_list; r != NULL; r = r->next) {
		css_rule_import *i = (css_rule_import *) r;

		if (r->type != CSS_RULE_UNKNOWN &&
				r->type != CSS_RULE_CHARSET &&
				r->type != CSS_RULE_IMPORT)
			break;

		if (r->type == CSS_RULE_IMPORT && i->sheet == NULL) {
			i->sheet = import;

			return CSS_OK;
		}
	}

	return CSS_INVALID;
}

/**
 * Retrieve the language level of a stylesheet
 *
 * \param sheet	 The stylesheet to retrieve the language level of
 * \param level	 Pointer to location to receive language level
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_get_language_level(css_stylesheet *sheet,
		css_language_level *level)
{
	if (sheet == NULL || level == NULL)
		return CSS_BADPARM;

	*level = sheet->level;

	return CSS_OK;
}

/**
 * Retrieve the URL associated with a stylesheet
 *
 * \param sheet	 The stylesheet to retrieve the URL from
 * \param url	 Pointer to location to receive pointer to URL
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_get_url(css_stylesheet *sheet, const char **url)
{
	if (sheet == NULL || url == NULL)
		return CSS_BADPARM;

	*url = sheet->url;

	return CSS_OK;
}

/**
 * Retrieve the title associated with a stylesheet
 *
 * \param sheet	 The stylesheet to retrieve the title from
 * \param title	 Pointer to location to receive pointer to title
 * \return CSS_Ok on success, appropriate error otherwise
 */
css_error css_stylesheet_get_title(css_stylesheet *sheet, const char **title)
{
	if (sheet == NULL || title == NULL)
		return CSS_BADPARM;

	*title = sheet->title;

	return CSS_OK;
}

/**
 * Determine whether quirky parsing was permitted on a stylesheet
 *
 * \param sheet	  The stylesheet to consider
 * \param quirks  Pointer to location to receive quirkyness
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_quirks_allowed(css_stylesheet *sheet, bool *allowed)
{
	if (sheet == NULL || allowed == NULL)
		return CSS_BADPARM;

	*allowed = sheet->quirks_allowed;

	return CSS_OK;
}


/**
 * Determine whether quirky parsing was used on a stylesheet
 *
 * \param sheet	  The stylesheet to consider
 * \param quirks  Pointer to location to receive quirkyness
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_used_quirks(css_stylesheet *sheet, bool *quirks)
{
	if (sheet == NULL || quirks == NULL)
		return CSS_BADPARM;

	*quirks = sheet->quirks_used;

	return CSS_OK;
}

/**
 * Get disabled status of a stylesheet
 *
 * \param sheet	    The stylesheet to consider
 * \param disabled  Pointer to location to receive disabled state
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_get_disabled(css_stylesheet *sheet, bool *disabled)
{
	if (sheet == NULL || disabled == NULL)
		return CSS_BADPARM;

	*disabled = sheet->disabled;

	return CSS_OK;
}

/**
 * Set a stylesheet's disabled state
 *
 * \param sheet	    The stylesheet to modify
 * \param disabled  The new disabled state
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_set_disabled(css_stylesheet *sheet, bool disabled)
{
	if (sheet == NULL)
		return CSS_BADPARM;

	sheet->disabled = disabled;

	/** \todo needs to trigger some event announcing styles have changed */

	return CSS_OK;
}

/**
 * Determine the memory-resident size of a stylesheet
 *
 * \param sheet	 Sheet to consider
 * \param size	 Pointer to location to receive byte count
 * \return CSS_OK on success.
 *
 * \note The returned size will not include the size of interned strings
 *	 or imported stylesheets.
 */
css_error css_stylesheet_size(css_stylesheet *sheet, size_t *size)
{
	size_t bytes = 0;
	css_error error;

	if (sheet == NULL || size == NULL)
		return CSS_BADPARM;

	bytes = sheet->size;

	/* Selector hash */
	if (sheet->selectors != NULL) {
		size_t hash_size;

		error = css_selector_hash_size(sheet->selectors, &hash_size);
		if (error != CSS_OK)
			return error;

		bytes += hash_size;
	}

	*size = bytes;

	return CSS_OK;
}

/******************************************************************************
 * Library-private API below here					      *
 ******************************************************************************/

/**
 * Create a style
 *
 * \param sheet	 The stylesheet context
 * \param len	 The required length of the style
 * \param style	 Pointer to location to receive style
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters,
 *	   CSS_NOMEM on memory exhaustion
 */
css_error css_stylesheet_style_create(css_stylesheet *sheet, uint32_t len,
		css_style **style)
{
	css_style *s;
	const uint32_t alloclen = ((len + 15) & ~15);
	const uint32_t bucket = (alloclen / N_ELEMENTS(sheet->free_styles)) - 1;

	if (sheet == NULL || len == 0 || style == NULL)
		return CSS_BADPARM;

	if (bucket < N_ELEMENTS(sheet->free_styles) &&
			sheet->free_styles[bucket] != NULL) {
		s = sheet->free_styles[bucket];
		sheet->free_styles[bucket] = s->bytecode;
	} else {
		s = sheet->alloc(NULL, sizeof(css_style) + alloclen, sheet->pw);
		if (s == NULL)
			return CSS_NOMEM;
	}

	/* DIY variable-sized data member */
	s->bytecode = ((uint8_t *) s + sizeof(css_style));
	s->length = len;

	*style = s;

	return CSS_OK;
}

/**
 * Destroy a style
 *
 * \param sheet	 The stylesheet context
 * \param style	 The style to destroy
 * \param suppress_bytecode_cleanup Whether or not to prevent the
 *				    bytecode from unreffing strings
 *				    etc.
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_style_destroy(css_stylesheet *sheet, css_style *style,
				       bool suppress_bytecode_cleanup)
{
	uint32_t alloclen, bucket;
	void *bptr, *eptr;

	if (sheet == NULL || style == NULL)
		return CSS_BADPARM;
	
	if (suppress_bytecode_cleanup == false) {
		bptr = style->bytecode;
		eptr = (uint8_t *) bptr + style->length;
		while (bptr != eptr) {
			uint32_t opcode = getOpcode(*((uint32_t*)bptr));
			uint32_t skip = prop_dispatch[opcode].destroy(bptr);
			bptr = (uint8_t *) bptr + skip;
		}
	}
	
	alloclen = ((style->length + 15) & ~15);
	bucket = (alloclen / N_ELEMENTS(sheet->free_styles)) - 1;

	if (bucket < N_ELEMENTS(sheet->free_styles)) {
		style->bytecode = sheet->free_styles[bucket];
		sheet->free_styles[bucket] = style;
	} else {
		sheet->alloc(style, 0, sheet->pw);
	}

	return CSS_OK;
}

/**
 * Create an element selector
 *
 * \param sheet	    The stylesheet context
 * \param name	    Name of selector
 * \param selector  Pointer to location to receive selector object
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters,
 *	   CSS_NOMEM on memory exhaustion
 */
css_error css_stylesheet_selector_create(css_stylesheet *sheet,
		lwc_string *name, css_selector **selector)
{
	css_selector *sel;

	if (sheet == NULL || name == NULL || selector == NULL)
		return CSS_BADPARM;

	sel = sheet->alloc(NULL, sizeof(css_selector), sheet->pw);
	if (sel == NULL)
		return CSS_NOMEM;

	memset(sel, 0, sizeof(css_selector));

	sel->data.type = CSS_SELECTOR_ELEMENT;
	sel->data.name = lwc_string_ref(name);
	sel->data.value = NULL;

	if (sheet->inline_style) {
		sel->specificity = CSS_SPECIFICITY_A;
	} else {
		/* Initial specificity -- 1 for an element, 0 for universal */
		if (lwc_string_length(name) != 1 || 
				lwc_string_data(name)[0] != '*')
			sel->specificity = CSS_SPECIFICITY_D;
		else
			sel->specificity = 0;
	}

	sel->data.comb = CSS_COMBINATOR_NONE;

	*selector = sel;

	return CSS_OK;
}

/**
 * Destroy a selector object
 *
 * \param sheet	    The stylesheet context
 * \param selector  The selector to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_selector_destroy(css_stylesheet *sheet,
		css_selector *selector)
{
	css_selector *c, *d;
	css_selector_detail *detail;

	if (sheet == NULL || selector == NULL)
		return CSS_BADPARM;

	/* Must not be attached to a rule */
	assert(selector->rule == NULL);

	/* Destroy combinator chain */
	for (c = selector->combinator; c != NULL; c = d) {
		d = c->combinator;

		for (detail = &c->data; detail;) {
			lwc_string_unref(detail->name);

			if (detail->value != NULL) {
				lwc_string_unref(detail->value);
			}

			if (detail->next)
				detail++;
			else
				detail = NULL;
		}
		
		sheet->alloc(c, 0, sheet->pw);
	}
	
	for (detail = &selector->data; detail;) {
		lwc_string_unref(detail->name);

		if (detail->value != NULL) {
			lwc_string_unref(detail->value);
		}

		if (detail->next)
			detail++;
		else
			detail = NULL;
	}
		     
	
	/* Destroy this selector */
	sheet->alloc(selector, 0, sheet->pw);

	return CSS_OK;
}

/**
 * Initialise a selector detail
 *
 * \param sheet	  The stylesheet context
 * \param type	  The type of selector to create
 * \param name	  Name of selector
 * \param value	  Value of selector, or NULL
 * \param detail  Pointer to detail object to initialise
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters
 */
css_error css_stylesheet_selector_detail_init(css_stylesheet *sheet,
		css_selector_type type, lwc_string *name, 
		lwc_string *value, 
		css_selector_detail *detail)
{
	if (sheet == NULL || name == NULL || detail == NULL)
		return CSS_BADPARM;

	memset(detail, 0, sizeof(css_selector_detail));

	detail->type = type;
	detail->name = name;
	detail->value = value;

	return CSS_OK;
}

/**
 * Append a selector to the specifics chain of another selector
 *
 * \param sheet	    The stylesheet context
 * \param parent    Pointer to pointer to the parent selector (updated on exit)
 * \param specific  The selector to append (copied)
 * \return CSS_OK on success, appropriate error otherwise.
 */
css_error css_stylesheet_selector_append_specific(css_stylesheet *sheet,
		css_selector **parent, const css_selector_detail *detail)
{
	css_selector *temp;
	css_selector_detail *d;
	size_t num_details = 0;

	if (sheet == NULL || parent == NULL || 
			*parent == NULL || detail == NULL)
		return CSS_BADPARM;

	/** \todo this may want optimising -- counting blocks is O(n) 
	 * In practice, however, n isn't likely to be large, so may be fine
	 */

	/* Count current number of detail blocks */
	for (d = &(*parent)->data; d->next != 0; d++)
		num_details++;

	/* Grow selector by one detail block */
	temp = sheet->alloc((*parent), sizeof(css_selector) + 
			(num_details + 1) * sizeof(css_selector_detail), 
			sheet->pw);
	if (temp == NULL)
		return CSS_NOMEM;

	/* Copy detail into empty block */
	(&temp->data)[num_details + 1] = *detail;
	/* Flag that there's another block */
	(&temp->data)[num_details].next = 1;
	
	/* Ref the strings */
	lwc_string_ref(detail->name);
	if (detail->value != NULL)
		lwc_string_ref(detail->value);
	
	(*parent) = temp;

	/* Update parent's specificity */
	switch (detail->type) {
	case CSS_SELECTOR_CLASS:
	case CSS_SELECTOR_PSEUDO_CLASS:
	case CSS_SELECTOR_ATTRIBUTE:
	case CSS_SELECTOR_ATTRIBUTE_EQUAL:
	case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
	case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
		(*parent)->specificity += CSS_SPECIFICITY_C;
		break;
	case CSS_SELECTOR_ID:
		(*parent)->specificity += CSS_SPECIFICITY_B;
		break;
	case CSS_SELECTOR_PSEUDO_ELEMENT:
	case CSS_SELECTOR_ELEMENT:
		(*parent)->specificity += CSS_SPECIFICITY_D;
		break;
	}

	return CSS_OK;
}

/**
 * Combine a pair of selectors
 *
 * \param sheet	 The stylesheet context
 * \param type	 The combinator type
 * \param a	 The first operand
 * \param b	 The second operand
 * \return CSS_OK on success, appropriate error otherwise.
 *
 * For example, given A + B, the combinator field of B would point at A, 
 * with a combinator type of CSS_COMBINATOR_SIBLING. Thus, given B, we can
 * find its combinator. It is not possible to find B given A.
 */
css_error css_stylesheet_selector_combine(css_stylesheet *sheet,
		css_combinator type, css_selector *a, css_selector *b)
{
	const css_selector_detail *det;

	if (sheet == NULL || a == NULL || b == NULL)
		return CSS_BADPARM;

	/* Ensure that there is no existing combinator on B */
	assert(b->combinator == NULL);

	/* A must not contain a pseudo element */
	for (det = &a->data; det != NULL; ) {
		if (det->type == CSS_SELECTOR_PSEUDO_ELEMENT)
			return CSS_INVALID;

		det = (det->next != 0) ? det + 1 : NULL;
	}

	b->combinator = a;
	b->data.comb = type;

	/* And propagate A's specificity to B */
	b->specificity += a->specificity;

	return CSS_OK;
}

/**
 * Create a CSS rule
 *
 * \param sheet	 The stylesheet context
 * \param type	 The rule type
 * \param rule	 Pointer to location to receive rule object
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters,
 *	   CSS_NOMEM on memory exhaustion
 */
css_error css_stylesheet_rule_create(css_stylesheet *sheet, css_rule_type type,
		css_rule **rule)
{
	css_rule *r;
	size_t required = 0;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	switch (type) {
	case CSS_RULE_UNKNOWN:
		required = sizeof(css_rule);
		break;
	case CSS_RULE_SELECTOR:
		required = sizeof(css_rule_selector);
		break;
	case CSS_RULE_CHARSET:
		required = sizeof(css_rule_charset);
		break;
	case CSS_RULE_IMPORT:
		required = sizeof(css_rule_import);
		break;
	case CSS_RULE_MEDIA:
		required = sizeof(css_rule_media);
		break;
	case CSS_RULE_FONT_FACE:
		required = sizeof(css_rule_font_face);
		break;
	case CSS_RULE_PAGE:
		required = sizeof(css_rule_page);
		break;
	}

	r = sheet->alloc(NULL, required, sheet->pw);
	if (r == NULL)
		return CSS_NOMEM;

	memset(r, 0, required);

	r->type = type;

	*rule = r;

	return CSS_OK;
}

/**
 * Destroy a CSS rule
 *
 * \param sheet	 The stylesheet context
 * \param rule	 The rule to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_destroy(css_stylesheet *sheet, css_rule *rule)
{
	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	/* Must be detached from parent/siblings */
	assert(rule->parent == NULL && rule->next == NULL && 
			rule->prev == NULL);

	/* Destroy type-specific contents */
	switch (rule->type) {
	case CSS_RULE_UNKNOWN:
		break;
	case CSS_RULE_SELECTOR:
	{
		css_rule_selector *s = (css_rule_selector *) rule;
		uint32_t i;

		for (i = 0; i < rule->items; i++) {
			css_selector *sel = s->selectors[i];

			/* Detach from rule */
			sel->rule = NULL;

			css_stylesheet_selector_destroy(sheet, sel);
		}

		if (s->selectors != NULL)
			sheet->alloc(s->selectors, 0, sheet->pw);

		if (s->style != NULL)
			css_stylesheet_style_destroy(sheet, s->style, false);
	}
		break;
	case CSS_RULE_CHARSET:
	{
		css_rule_charset *charset = (css_rule_charset *) rule;
		lwc_string_unref(charset->encoding);
	}
		break;
	case CSS_RULE_IMPORT:
	{
		css_rule_import *import = (css_rule_import *) rule;
		
		lwc_string_unref(import->url);
		
		/* Do not destroy imported sheet: it is owned by the client */
	}
		break;
	case CSS_RULE_MEDIA:
	{
		css_rule_media *media = (css_rule_media *) rule;
		css_rule *c, *d;

		for (c = media->first_child; c != NULL; c = d) {
			d = c->next;

			/* Detach from list */
			c->parent = NULL;
			c->prev = NULL;
			c->next = NULL;

			css_stylesheet_rule_destroy(sheet, c);
		}
	}
		break;
	case CSS_RULE_FONT_FACE:
	{
		css_rule_font_face *font_face = (css_rule_font_face *) rule;

		if (font_face->style != NULL)
			css_stylesheet_style_destroy(sheet, font_face->style, false);
	}
		break;
	case CSS_RULE_PAGE:
	{
		css_rule_page *page = (css_rule_page *) rule;

		if (page->selector != NULL) {
			page->selector->rule = NULL;
			css_stylesheet_selector_destroy(sheet, page->selector);
		}

		if (page->style != NULL)
			css_stylesheet_style_destroy(sheet, page->style, false);
	}
		break;
	}

	/* Destroy rule */
	sheet->alloc(rule, 0, sheet->pw);

	return CSS_OK;
}

/**
 * Add a selector to a CSS rule
 *
 * \param sheet	    The stylesheet context
 * \param rule	    The rule to add to (must be of type CSS_RULE_SELECTOR)
 * \param selector  The selector to add
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_add_selector(css_stylesheet *sheet, 
		css_rule *rule, css_selector *selector)
{
	css_rule_selector *r = (css_rule_selector *) rule;
	css_selector **sels;

	if (sheet == NULL || rule == NULL || selector == NULL)
		return CSS_BADPARM;

	/* Ensure rule is a CSS_RULE_SELECTOR */
	assert(rule->type == CSS_RULE_SELECTOR);

	sels = sheet->alloc(r->selectors, 
			(r->base.items + 1) * sizeof(css_selector *), 
			sheet->pw);
	if (sels == NULL)
		return CSS_NOMEM;

	/* Insert into rule's selector list */
	sels[r->base.items] = selector;
	r->base.items++;
	r->selectors = sels;

	/* Set selector's rule field */
	selector->rule = rule;
	
	return CSS_OK;
}

/**
 * Append a style to a CSS rule
 *
 * \param sheet	 The stylesheet context
 * \param rule	 The rule to add to (must be CSS_RULE_SELECTOR or CSS_RULE_PAGE)
 * \param style	 The style to add
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_append_style(css_stylesheet *sheet,
		css_rule *rule, css_style *style)
{
	css_style *cur;

	if (sheet == NULL || rule == NULL || style == NULL)
		return CSS_BADPARM;

	assert(rule->type == CSS_RULE_SELECTOR || rule->type == CSS_RULE_PAGE);

	if (rule->type == CSS_RULE_SELECTOR)
		cur = ((css_rule_selector *) rule)->style;
	else
		cur = ((css_rule_page *) rule)->style;

	if (cur != NULL) {
		/* Already have a style, so append to the end of the bytecode */
		const uint32_t reqlen = cur->length + style->length;
		const uint32_t curlen = ((cur->length + 15) & ~15);

		if (curlen < reqlen) {
			css_style *temp;
			css_error error;

			error = css_stylesheet_style_create(sheet, 
					reqlen, &temp);
			if (error != CSS_OK)
				return error;

			memcpy((uint8_t *) temp->bytecode, cur->bytecode, 
					cur->length);
			temp->length = cur->length;

			css_stylesheet_style_destroy(sheet, cur, true);

			cur = temp;
		}

		/** \todo Can we optimise the bytecode here? */
		memcpy((uint8_t *) cur->bytecode + cur->length, 
				style->bytecode, style->length);

		cur->length += style->length;

		/* Add this to the sheet's size */
		sheet->size += style->length;

		/* Done with style */
		css_stylesheet_style_destroy(sheet, style, true);
	} else {
		/* No current style, so use this one */
		cur = style;

		/* Add to the sheet's size */
		sheet->size += style->length;
	}

	if (rule->type == CSS_RULE_SELECTOR)
		((css_rule_selector *) rule)->style = cur;
	else
		((css_rule_page *) rule)->style = cur;

	return CSS_OK;
}

/**
 * Set the charset of a CSS rule
 *
 * \param sheet	   The stylesheet context
 * \param rule	   The rule to add to (must be of type CSS_RULE_CHARSET)
 * \param charset  The charset
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_set_charset(css_stylesheet *sheet,
		css_rule *rule, lwc_string *charset)
{
	css_rule_charset *r = (css_rule_charset *) rule;

	if (sheet == NULL || rule == NULL || charset == NULL)
		return CSS_BADPARM;

	/* Ensure rule is a CSS_RULE_CHARSET */
	assert(rule->type == CSS_RULE_CHARSET);

	/* Set rule's encoding field */
	r->encoding = lwc_string_ref(charset);
	
	return CSS_OK;
}


/**
 * Set the necessary data to import a stylesheet associated with a rule
 *
 * \param sheet	  The stylesheet context
 * \param rule	  The rule to add to (must be of type CSS_RULE_IMPORT)
 * \param url	  The URL of the imported stylesheet
 * \param media	  The applicable media types for the imported stylesheet
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_set_nascent_import(css_stylesheet *sheet,
		css_rule *rule, lwc_string *url, 
		uint64_t media)
{
	css_rule_import *r = (css_rule_import *) rule;

	if (sheet == NULL || rule == NULL || url == NULL)
		return CSS_BADPARM;

	/* Ensure rule is a CSS_RULE_IMPORT */
	assert(rule->type == CSS_RULE_IMPORT);

	/* Set the rule's sheet field */
	r->url = lwc_string_ref(url);
	r->media = media;

	return CSS_OK;
}

/**
 * Set the media of an @media rule
 *
 * \param sheet	 The stylesheet context
 * \param rule	 The rule to add to (must be of type CSS_RULE_MEDIA)
 * \param media	 The applicable media types for the rule
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_set_media(css_stylesheet *sheet,
		css_rule *rule, uint64_t media)
{
	css_rule_media *r = (css_rule_media *) rule;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	/* Ensure rule is a CSS_RULE_MEDIA */
	assert(rule->type == CSS_RULE_MEDIA);

	/* Set the rule's media */
	r->media = media;

	return CSS_OK;
}

/**
 * Set an @page rule selector
 *
 * \param sheet	    The stylesheet context
 * \param rule	    The rule to add to (must be of type CSS_RULE_PAGE)
 * \param selector  The page selector
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_rule_set_page_selector(css_stylesheet *sheet,
		css_rule *rule, css_selector *selector)
{
	css_rule_page *r = (css_rule_page *) rule;

	if (sheet == NULL || rule == NULL || selector == NULL)
		return CSS_BADPARM;

	/* Ensure rule is a CSS_RULE_PAGE */
	assert(rule->type == CSS_RULE_PAGE);

	/** \todo validate selector */

	/* Set the rule's selector */
	r->selector = selector;

	/* Set selector's rule field */
	selector->rule = rule;

	return CSS_OK;
}

/**
 * Add a rule to a stylesheet
 *
 * \param sheet	  The stylesheet to add to
 * \param rule	  The rule to add
 * \param parent  The parent rule, or NULL for a top-level rule
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_add_rule(css_stylesheet *sheet, css_rule *rule,
		css_rule *parent)
{
	css_error error;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	/* Need to fill in rule's index field before adding selectors
	 * because selector chains consider the rule index for sort order
	 */
	rule->index = sheet->rule_count;

	/* Add any selectors to the hash */
	error = _add_selectors(sheet, rule);
	if (error != CSS_OK)
		return error;

	/* Add to the sheet's size */
	sheet->size += _rule_size(rule);

	if (parent != NULL) {
		css_rule_media *media = (css_rule_media *) parent;

		/* Parent must be an @media rule, or NULL */
		assert(parent->type == CSS_RULE_MEDIA);

		/* Add rule to parent */
		rule->ptype = CSS_RULE_PARENT_RULE;
		rule->parent = parent;
		sheet->rule_count++;

		if (media->last_child == NULL) {
			rule->prev = rule->next = NULL;
			media->first_child = media->last_child = rule;
		} else {
			media->last_child->next = rule;
			rule->prev = media->last_child;
			rule->next = NULL;
			media->last_child = rule;
		}
	} else {
		/* Add rule to sheet */
		rule->ptype = CSS_RULE_PARENT_STYLESHEET;
		rule->parent = sheet;
		sheet->rule_count++;

		if (sheet->last_rule == NULL) {
			rule->prev = rule->next = NULL;
			sheet->rule_list = sheet->last_rule = rule;
		} else {
			sheet->last_rule->next = rule;
			rule->prev = sheet->last_rule;
			rule->next = NULL;
			sheet->last_rule = rule;
		}
	}

	/** \todo needs to trigger some event announcing styles have changed */

	return CSS_OK;
}

/**
 * Remove a rule from a stylesheet
 *
 * \param sheet	 The sheet to remove from
 * \param rule	 The rule to remove
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_stylesheet_remove_rule(css_stylesheet *sheet, css_rule *rule)
{
	css_error error;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	error = _remove_selectors(sheet, rule);
	if (error != CSS_OK)
		return error;

	/* Reduce sheet's size */
	sheet->size -= _rule_size(rule);

	if (rule->next == NULL)
		sheet->last_rule = rule->prev;
	else
		rule->next->prev = rule->prev;

	if (rule->prev == NULL)
		sheet->rule_list = rule->next;
	else
		rule->prev->next = rule->next;

	/* Invalidate linkage fields */
	rule->parent = NULL;
	rule->prev = NULL;
	rule->next = NULL;

	/**\ todo renumber subsequent rules? may not be necessary, as there's 
	 * only an expectation that rules which occur later in the stylesheet 
	 * have a higher index than those that appear earlier. There's no 
	 * guarantee that the number space is continuous. */

	/** \todo needs to trigger some event announcing styles have changed */

	return CSS_OK;
}

/******************************************************************************
 * Private API below here						      *
 ******************************************************************************/

/**
 * Add selectors in a rule to the hash
 *
 * \param sheet	 Stylesheet containing hash
 * \param rule	 Rule to consider
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error _add_selectors(css_stylesheet *sheet, css_rule *rule)
{
	css_error error;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	/* Rule must not be in sheet */
	assert(rule->parent == NULL);

	switch (rule->type) {
	case CSS_RULE_SELECTOR:
	{
		css_rule_selector *s = (css_rule_selector *) rule;
		int32_t i;

		for (i = 0; i < rule->items; i++) {
			css_selector *sel = s->selectors[i];

			error = css_selector_hash_insert(sheet->selectors, sel);
			if (error != CSS_OK) {
				/* Failed, revert our changes */
				for (i--; i >= 0; i--) {
					sel = s->selectors[i];

					/* Ignore errors */
					css_selector_hash_remove(
							sheet->selectors, sel);
				}
				
				return error;
			}
		}
	}
		break;
	case CSS_RULE_MEDIA:
	{
		css_rule_media *m = (css_rule_media *) rule;
		css_rule *r;

		for (r = m->first_child; r != NULL; r = r->next) {
			error = _add_selectors(sheet, r);
			if (error != CSS_OK) {
				/* Failed, revert our changes */
				for (r = r->prev; r != NULL; r = r->prev) {
					_remove_selectors(sheet, r);
				}

				return error;
			}
		}
	}
		break;
	}

	return CSS_OK;
}

/**
 * Remove selectors in a rule from the hash
 *
 * \param sheet	 Stylesheet containing hash
 * \param rule	 Rule to consider
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error _remove_selectors(css_stylesheet *sheet, css_rule *rule)
{
	css_error error;

	if (sheet == NULL || rule == NULL)
		return CSS_BADPARM;

	switch (rule->type) {
	case CSS_RULE_SELECTOR:
	{
		css_rule_selector *s = (css_rule_selector *) rule;
		int32_t i;

		for (i = 0; i < rule->items; i++) {
			css_selector *sel = s->selectors[i];

			error = css_selector_hash_remove(sheet->selectors, sel);
			if (error != CSS_OK)
				return error;
		}
	}
		break;
	case CSS_RULE_MEDIA:
	{
		css_rule_media *m = (css_rule_media *) rule;
		css_rule *r;

		for (r = m->first_child; r != NULL; r = r->next) {
			error = _remove_selectors(sheet, r);
			if (error != CSS_OK)
				return error;
		}
	}
		break;
	}

	return CSS_OK;
}

/**
 * Calculate the size of a rule
 *
 * \param r  Rule to consider
 * \return Size in bytes
 *
 * \note The returned size does not include interned strings.
 */
size_t _rule_size(const css_rule *r)
{
	size_t bytes = 0;

	if (r->type == CSS_RULE_SELECTOR) {
		const css_rule_selector *rs = (const css_rule_selector *) r;
		uint32_t i;

		bytes += sizeof(css_rule_selector);

		/* Process selector chains */
		bytes += r->items * sizeof(css_selector *);
		for (i = 0; i < r->items; i++) {
			const css_selector *s = rs->selectors[i];

			do {
				const css_selector_detail *d = &s->data;

				bytes += sizeof(css_selector);

				while (d->next) {
					bytes += sizeof(css_selector_detail);
					d++;
				}

				s = s->combinator;
			} while (s != NULL);
		}

		if (rs->style != NULL)
			bytes += rs->style->length;
	} else if (r->type == CSS_RULE_CHARSET) {
		bytes += sizeof(css_rule_charset);
	} else if (r->type == CSS_RULE_IMPORT) {
		bytes += sizeof(css_rule_import);
	} else if (r->type == CSS_RULE_MEDIA) {
		const css_rule_media *rm = (const css_rule_media *) r;
		const css_rule *c;

		bytes += sizeof(css_rule_media);

		/* Process children */
		for (c = rm->first_child; c != NULL; c = c->next)
			bytes += _rule_size(c);
	} else if (r->type == CSS_RULE_FONT_FACE) {
		const css_rule_font_face *rf = (const css_rule_font_face *) r;

		bytes += sizeof(css_rule_font_face);

		if (rf->style != NULL)
			bytes += rf->style->length;
	} else if (r->type == CSS_RULE_PAGE) {
		const css_rule_page *rp = (const css_rule_page *) r;
		const css_selector *s = rp->selector;

		/* Process selector chain */
		while (s != NULL) {
			const css_selector_detail *d = &s->data;

			bytes += sizeof(css_selector);

			while (d->next) {
				bytes += sizeof(css_selector_detail);
				d++;
			}

			s = s->combinator;	
		}

		if (rp->style != NULL)
			bytes += rp->style->length;
	}

	return bytes;
}

