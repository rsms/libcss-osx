/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include <parserutils/utils/stack.h>

#include "stylesheet.h"
#include "lex/lex.h"
#include "parse/important.h"
#include "parse/language.h"
#include "parse/parse.h"
#include "parse/propstrings.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

#include "utils/parserutilserror.h"
#include "utils/utils.h"

typedef struct context_entry {
	css_parser_event type;		/**< Type of entry */
	void *data;			/**< Data for context */
} context_entry;

/* Event handlers */
static css_error language_handle_event(css_parser_event type, 
		const parserutils_vector *tokens, void *pw);
static css_error handleStartStylesheet(css_language *c, 
		const parserutils_vector *vector);
static css_error handleEndStylesheet(css_language *c, 
		const parserutils_vector *vector);
static css_error handleStartRuleset(css_language *c, 
		const parserutils_vector *vector);
static css_error handleEndRuleset(css_language *c, 
		const parserutils_vector *vector);
static css_error handleStartAtRule(css_language *c, 
		const parserutils_vector *vector);
static css_error handleEndAtRule(css_language *c, 
		const parserutils_vector *vector);
static css_error handleStartBlock(css_language *c, 
		const parserutils_vector *vector);
static css_error handleEndBlock(css_language *c, 
		const parserutils_vector *vector);
static css_error handleBlockContent(css_language *c, 
		const parserutils_vector *vector);
static css_error handleDeclaration(css_language *c, 
		const parserutils_vector *vector);

/* At-rule parsing */
static css_error parseMediaList(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint64_t *media);

/* Selector list parsing */
static css_error parseClass(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static css_error parseAttrib(css_language *c, 
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static css_error parsePseudo(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector_detail *specific);
static css_error parseSpecific(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent);
static css_error parseSelectorSpecifics(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent);
static css_error parseSimpleSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result);
static css_error parseCombinator(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_combinator *result);
static css_error parseSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result);
static css_error parseSelectorList(css_language *c, 
		const parserutils_vector *vector, css_rule *rule);

/* Declaration parsing */
static css_error parseProperty(css_language *c,
		const css_token *property, const parserutils_vector *vector,
		int *ctx, css_rule *rule);

/**
 * Create a CSS language parser
 *
 * \param sheet	    The stylesheet object to parse for
 * \param parser    The core parser object to use
 * \param alloc	    Memory (de)allocation function
 * \param pw	    Pointer to client-specific private data
 * \param language  Pointer to location to receive parser object
 * \return CSS_OK on success,
 *	   CSS_BADPARM on bad parameters,
 *	   CSS_NOMEM on memory exhaustion
 */
css_error css_language_create(css_stylesheet *sheet, css_parser *parser,
		css_allocator_fn alloc, void *pw, void **language)
{
	css_language *c;
	css_parser_optparams params;
	parserutils_error perror;
	lwc_error lerror;
	css_error error;
	int i;

	if (sheet == NULL || parser == NULL || alloc == NULL || 
			language == NULL)
		return CSS_BADPARM;

	c = alloc(NULL, sizeof(css_language), pw);
	if (c == NULL)
		return CSS_NOMEM;

	perror = parserutils_stack_create(sizeof(context_entry), 
			STACK_CHUNK, (parserutils_alloc) alloc, pw, 
			&c->context);
	if (perror != PARSERUTILS_OK) {
		alloc(c, 0, pw);
		return css_error_from_parserutils_error(perror);
	}

	/* Intern all known strings */
	for (i = 0; i < LAST_KNOWN; i++) {
		lerror = lwc_intern_string(stringmap[i].data,
					    stringmap[i].len,
					    &(c->strings[i]));
		if (lerror != lwc_error_ok) {
			parserutils_stack_destroy(c->context);
			alloc(c, 0, pw);
			return CSS_NOMEM;
		}
	}

	params.event_handler.handler = language_handle_event;
	params.event_handler.pw = c;
	error = css_parser_setopt(parser, CSS_PARSER_EVENT_HANDLER, &params);
	if (error != CSS_OK) {
		parserutils_stack_destroy(c->context);
		alloc(c, 0, pw);
		return error;
	}

	c->sheet = sheet;
	c->state = BEFORE_CHARSET;
	c->alloc = alloc;
	c->pw = pw;

	*language = c;

	return CSS_OK;
}

/**
 * Destroy a CSS language parser
 *
 * \param language  The parser to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_language_destroy(css_language *language)
{
	int i;
	
	if (language == NULL)
		return CSS_BADPARM;

	parserutils_stack_destroy(language->context);
	
	for (i = 0; i < LAST_KNOWN; ++i) {
		lwc_string_unref(language->strings[i]);
	}
	
	language->alloc(language, 0, language->pw);

	return CSS_OK;
}

/**
 * Handler for core parser events
 *
 * \param type	  The event type
 * \param tokens  Vector of tokens read since last event, or NULL
 * \param pw	  Pointer to handler context
 * \return CSS_OK on success, CSS_INVALID to indicate parse error, 
 *	   appropriate error otherwise.
 */
css_error language_handle_event(css_parser_event type, 
		const parserutils_vector *tokens, void *pw)
{
	css_language *language = (css_language *) pw;

	switch (type) {
	case CSS_PARSER_START_STYLESHEET:
		return handleStartStylesheet(language, tokens);
	case CSS_PARSER_END_STYLESHEET:
		return handleEndStylesheet(language, tokens);
	case CSS_PARSER_START_RULESET:
		return handleStartRuleset(language, tokens);
	case CSS_PARSER_END_RULESET:
		return handleEndRuleset(language, tokens);
	case CSS_PARSER_START_ATRULE:
		return handleStartAtRule(language, tokens);
	case CSS_PARSER_END_ATRULE:
		return handleEndAtRule(language, tokens);
	case CSS_PARSER_START_BLOCK:
		return handleStartBlock(language, tokens);
	case CSS_PARSER_END_BLOCK:
		return handleEndBlock(language, tokens);
	case CSS_PARSER_BLOCK_CONTENT:
		return handleBlockContent(language, tokens);
	case CSS_PARSER_DECLARATION:
		return handleDeclaration(language, tokens);
	}

	return CSS_OK;
}

/******************************************************************************
 * Parser stages							      *
 ******************************************************************************/

css_error handleStartStylesheet(css_language *c, 
		const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry entry = { CSS_PARSER_START_STYLESHEET, NULL };

	UNUSED(vector);

	assert(c != NULL);

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleEndStylesheet(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_STYLESHEET)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartRuleset(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	css_error error;
	context_entry entry = { CSS_PARSER_START_RULESET, NULL };
	context_entry *cur;
	css_rule *parent_rule = NULL;
	css_rule *rule = NULL;

	assert(c != NULL);

	/* Retrieve parent rule from stack, if any */
	cur = parserutils_stack_get_current(c->context);
	if (cur != NULL && cur->type != CSS_PARSER_START_STYLESHEET)
		parent_rule = cur->data;

	error = css_stylesheet_rule_create(c->sheet, CSS_RULE_SELECTOR, &rule);
	if (error != CSS_OK)
		return error;

	if (vector != NULL) {
		/* Parse selectors, if there are any */
		error = parseSelectorList(c, vector, rule);
		if (error != CSS_OK) {
			css_stylesheet_rule_destroy(c->sheet, rule);
			return error;
		}
	}

	entry.data = rule;

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		css_stylesheet_rule_destroy(c->sheet, rule);
		return css_error_from_parserutils_error(perror);
	}

	error = css_stylesheet_add_rule(c->sheet, rule, parent_rule);
	if (error != CSS_OK) {
		parserutils_stack_pop(c->context, NULL);
		css_stylesheet_rule_destroy(c->sheet, rule);
		return error;
	}

	/* Flag that we've had a valid rule, so @import/@charset have 
	 * no effect. */
	c->state = HAD_RULE;

	/* Rule is now owned by the sheet, so no need to destroy it */

	return CSS_OK;
}

css_error handleEndRuleset(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_RULESET)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartAtRule(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry entry = { CSS_PARSER_START_ATRULE, NULL };
	const css_token *token = NULL;
	const css_token *atkeyword = NULL;
	int32_t ctx = 0;
	bool match = false;
	css_rule *rule;
	css_error error;

	/* vector contains: ATKEYWORD ws any0 */

	assert(c != NULL);

	atkeyword = parserutils_vector_iterate(vector, &ctx);

	consumeWhitespace(vector, &ctx);

	/* We now have an ATKEYWORD and the context for the start of any0, if 
	 * there is one */
	assert(atkeyword != NULL && atkeyword->type == CSS_TOKEN_ATKEYWORD);

	if (lwc_string_caseless_isequal(atkeyword->idata, c->strings[CHARSET], 
			&match) == lwc_error_ok && match) {
		if (c->state == BEFORE_CHARSET) {
			const css_token *charset;

			/* any0 = STRING */
			if (ctx == 0)
				return CSS_INVALID;

			charset = parserutils_vector_iterate(vector, &ctx);
			if (charset == NULL || 
					charset->type != CSS_TOKEN_STRING)
				return CSS_INVALID;

			token = parserutils_vector_iterate(vector, &ctx);
			if (token != NULL)
				return CSS_INVALID;

			error = css_stylesheet_rule_create(c->sheet, 
					CSS_RULE_CHARSET, &rule);
			if (error != CSS_OK)
				return error;

			error = css_stylesheet_rule_set_charset(c->sheet, rule,
					charset->idata);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			error = css_stylesheet_add_rule(c->sheet, rule, NULL);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* Rule is now owned by the sheet, 
			 * so no need to destroy it */

			c->state = BEFORE_RULES;
		} else {
			return CSS_INVALID;
		}
	} else if (lwc_string_caseless_isequal(atkeyword->idata, c->strings[IMPORT], 
			&match) == lwc_error_ok && match) {
		if (c->state != HAD_RULE) {
			lwc_string *url;
			uint64_t media = 0;

			/* any0 = (STRING | URI) ws 
			 *	  (IDENT ws (',' ws IDENT ws)* )? */
			const css_token *uri = 
				parserutils_vector_iterate(vector, &ctx);
			if (uri == NULL || (uri->type != CSS_TOKEN_STRING && 
					uri->type != CSS_TOKEN_URI))
				return CSS_INVALID;

			consumeWhitespace(vector, &ctx);

			/* Parse media list */
			error = parseMediaList(c, vector, &ctx, &media);
			if (error != CSS_OK)
				return error;

			/* Create rule */
			error = css_stylesheet_rule_create(c->sheet, 
					CSS_RULE_IMPORT, &rule);
			if (error != CSS_OK)
				return error;

			/* Resolve import URI */
			error = c->sheet->resolve(c->sheet->resolve_pw,
					c->sheet->url,
					uri->idata, &url);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* Inform rule of it */
			error = css_stylesheet_rule_set_nascent_import(c->sheet,
					rule, url, media);
			if (error != CSS_OK) {
				lwc_string_unref(url);
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* No longer care about url */
			lwc_string_unref(url);

			/* Add rule to sheet */
			error = css_stylesheet_add_rule(c->sheet, rule, NULL);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			/* Rule is now owned by the sheet, 
			 * so no need to destroy it */

			c->state = BEFORE_RULES;
		} else {
			return CSS_INVALID;
		}
	} else if (lwc_string_caseless_isequal(atkeyword->idata, c->strings[MEDIA], 
			&match) == lwc_error_ok && match) {
		uint64_t media = 0;

		/* any0 = IDENT ws (',' ws IDENT ws)* */

		error = parseMediaList(c, vector, &ctx, &media);
		if (error != CSS_OK)
			return error;

		error = css_stylesheet_rule_create(c->sheet, 
				CSS_RULE_MEDIA, &rule);
		if (error != CSS_OK)
			return error;

		error = css_stylesheet_rule_set_media(c->sheet, rule, media);
		if (error != CSS_OK) {
			css_stylesheet_rule_destroy(c->sheet, rule);
			return error;
		}

		error = css_stylesheet_add_rule(c->sheet, rule, NULL);
		if (error != CSS_OK) {
			css_stylesheet_rule_destroy(c->sheet, rule);
			return error;
		}

		/* Rule is now owned by the sheet, 
		 * so no need to destroy it */

		c->state = HAD_RULE;
	} else if (lwc_string_caseless_isequal(atkeyword->idata, c->strings[PAGE], 
			&match) == lwc_error_ok && match) {
		const css_token *token;

		/* any0 = (':' IDENT)? ws */

		error = css_stylesheet_rule_create(c->sheet,
				CSS_RULE_PAGE, &rule);
		if (error != CSS_OK)
			return error;

		consumeWhitespace(vector, &ctx);

		token = parserutils_vector_peek(vector, ctx);
		if (token != NULL) {
			css_selector *sel = NULL;

			error = parseSelector(c, vector, &ctx, &sel);
			if (error != CSS_OK) {
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}

			error = css_stylesheet_rule_set_page_selector(c->sheet,
					rule, sel);
			if (error != CSS_OK) {
				css_stylesheet_selector_destroy(c->sheet, sel);
				css_stylesheet_rule_destroy(c->sheet, rule);
				return error;
			}
		}

		error = css_stylesheet_add_rule(c->sheet, rule, NULL);
		if (error != CSS_OK) {
			css_stylesheet_rule_destroy(c->sheet, rule);
			return error;
		}

		/* Rule is now owned by the sheet, 
		 * so no need to destroy it */

		c->state = HAD_RULE;
	} else {
		return CSS_INVALID;
	}

	entry.data = rule;

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleEndAtRule(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;

	UNUSED(vector);

	assert(c != NULL);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_ATRULE)
		return CSS_INVALID;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleStartBlock(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry entry = { CSS_PARSER_START_BLOCK, NULL };
	context_entry *cur;

	UNUSED(vector);

	/* If the current item on the stack isn't a block, 
	 * then clone its data field. This ensures that the relevant rule
	 * is available when parsing the block contents. */
	cur = parserutils_stack_get_current(c->context);
	if (cur != NULL && cur->type != CSS_PARSER_START_BLOCK)
		entry.data = cur->data;

	perror = parserutils_stack_push(c->context, (void *) &entry);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	return CSS_OK;
}

css_error handleEndBlock(css_language *c, const parserutils_vector *vector)
{
	parserutils_error perror;
	context_entry *entry;
	css_rule *rule;

	UNUSED(vector);

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->type != CSS_PARSER_START_BLOCK)
		return CSS_INVALID;

	rule = entry->data;

	perror = parserutils_stack_pop(c->context, NULL);
	if (perror != PARSERUTILS_OK) {
		return css_error_from_parserutils_error(perror);
	}

	/* If the block we just popped off the stack was associated with a 
	 * non-block stack entry, and that entry is not a top-level statement,
	 * then report the end of that entry, too. */
	if (rule != NULL && rule->ptype != CSS_RULE_PARENT_STYLESHEET) {
		if (rule->type == CSS_RULE_SELECTOR)
			return handleEndRuleset(c, vector);
	}

	return CSS_OK;
}

css_error handleBlockContent(css_language *c, const parserutils_vector *vector)
{
	context_entry *entry;
	css_rule *rule;

	/* In CSS 2.1, block content comprises either declarations (if the 
	 * current block is associated with @page or a selector), or rulesets 
	 * (if the current block is associated with @media). */

	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->data == NULL)
		return CSS_INVALID;

	rule = entry->data;
	if (rule == NULL || (rule->type != CSS_RULE_SELECTOR && 
			rule->type != CSS_RULE_PAGE &&
			rule->type != CSS_RULE_MEDIA))
		return CSS_INVALID;

	if (rule->type == CSS_RULE_MEDIA) {
		/* Expect rulesets */
		return handleStartRuleset(c, vector);
	} else {
		/* Expect declarations */
		return handleDeclaration(c, vector);
	}

	return CSS_OK;
}

css_error handleDeclaration(css_language *c, const parserutils_vector *vector)
{
	css_error error;
	const css_token *token, *ident;
	int ctx = 0;
	context_entry *entry;
	css_rule *rule;

	/* Locations where declarations are permitted:
	 *
	 * + In @page
	 * + In ruleset
	 */
	entry = parserutils_stack_get_current(c->context);
	if (entry == NULL || entry->data == NULL)
		return CSS_INVALID;

	rule = entry->data;
	if (rule == NULL || (rule->type != CSS_RULE_SELECTOR && 
			rule->type != CSS_RULE_PAGE))
		return CSS_INVALID;

	/* Strip any leading whitespace (can happen if in nested block) */
	consumeWhitespace(vector, &ctx);

	/* IDENT ws ':' ws value 
	 * 
	 * In CSS 2.1, value is any1, so '{' or ATKEYWORD => parse error
	 */
	ident = parserutils_vector_iterate(vector, &ctx);
	if (ident == NULL || ident->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	consumeWhitespace(vector, &ctx);

	token = parserutils_vector_iterate(vector, &ctx);
	if (token == NULL || tokenIsChar(token, ':') == false)
		return CSS_INVALID;

	consumeWhitespace(vector, &ctx);

	error = parseProperty(c, ident, vector, &ctx, rule);
	if (error != CSS_OK)
		return error;

	return CSS_OK;
}

/******************************************************************************
 * At-rule parsing functions						      *
 ******************************************************************************/
css_error parseMediaList(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint64_t *media)
{
	uint64_t ret = 0;
	bool match = false;
	const css_token *token;

	token = parserutils_vector_iterate(vector, ctx);

	while (token != NULL) {
		if (token->type != CSS_TOKEN_IDENT)
			return CSS_INVALID;

		if (lwc_string_caseless_isequal(token->idata, c->strings[AURAL], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_AURAL;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[BRAILLE], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_BRAILLE;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[EMBOSSED], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_EMBOSSED;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[HANDHELD], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_HANDHELD;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[PRINT], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_PRINT;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[PROJECTION], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_PROJECTION;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[SCREEN], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_SCREEN;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[SPEECH], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_SPEECH;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[TTY], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_TTY;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[TV], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_TV;
		} else if (lwc_string_caseless_isequal(
				token->idata, c->strings[ALL], 
				&match) == lwc_error_ok && match) {
			ret |= CSS_MEDIA_ALL;
		} else
			return CSS_INVALID;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token != NULL && tokenIsChar(token, ',') == false)
			return CSS_INVALID;

		consumeWhitespace(vector, ctx);
	}

	/* If, after parsing the media list, we still have no media, 
	 * then it must be ALL. */
	if (ret == 0)
		ret = CSS_MEDIA_ALL;

	*media = ret;

	return CSS_OK;
}

/******************************************************************************
 * Selector list parsing functions					      *
 ******************************************************************************/

css_error parseClass(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token;

	/* class     -> '.' IDENT */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '.') == false)
		return CSS_INVALID;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	return css_stylesheet_selector_detail_init(c->sheet, 
			CSS_SELECTOR_CLASS, token->idata, NULL, specific);
}

css_error parseAttrib(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token, *name, *value = NULL;
	css_selector_type type = CSS_SELECTOR_ATTRIBUTE;

	/* attrib    -> '[' ws IDENT ws [
	 *		       [ '=' | INCLUDES | DASHMATCH ] ws
	 *		       [ IDENT | STRING ] ws ]? ']'
	 */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '[') == false)
		return CSS_INVALID;

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT)
		return CSS_INVALID;

	name = token;

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (tokenIsChar(token, ']') == false) {
		if (tokenIsChar(token, '='))
			type = CSS_SELECTOR_ATTRIBUTE_EQUAL;
		else if (token->type == CSS_TOKEN_INCLUDES)
			type = CSS_SELECTOR_ATTRIBUTE_INCLUDES;
		else if (token->type == CSS_TOKEN_DASHMATCH)
			type = CSS_SELECTOR_ATTRIBUTE_DASHMATCH;
		else
			return CSS_INVALID;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
				token->type != CSS_TOKEN_STRING))
			return CSS_INVALID;

		value = token;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token == NULL || tokenIsChar(token, ']') == false)
			return CSS_INVALID;
	}

	return css_stylesheet_selector_detail_init(c->sheet, type, 
			name->idata, value != NULL ? value->idata : NULL,
			specific);
}

css_error parsePseudo(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector_detail *specific)
{
	const css_token *token, *name, *value = NULL;
	bool match = false;
	css_selector_type type;

	/* pseudo    -> ':' [ IDENT | FUNCTION ws IDENT? ws ')' ] */

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, ':') == false)
		return CSS_INVALID;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT && 
			token->type != CSS_TOKEN_FUNCTION))
		return CSS_INVALID;

	name = token;

	if (token->type == CSS_TOKEN_FUNCTION) {
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);

		if (token != NULL && token->type == CSS_TOKEN_IDENT) {
			value = token;

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_iterate(vector, ctx);
		}

		if (token == NULL || tokenIsChar(token, ')') == false)
			return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
				name->idata, c->strings[FIRST_CHILD], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[LINK], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[VISITED], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[HOVER], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[ACTIVE], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[FOCUS], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[LANG], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[LEFT], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[RIGHT], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[FIRST], 
				&match) == lwc_error_ok && match))
		type = CSS_SELECTOR_PSEUDO_CLASS;
	else if ((lwc_string_caseless_isequal(
				name->idata, c->strings[FIRST_LINE], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[FIRST_LETTER], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[BEFORE], 
				&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
				name->idata, c->strings[AFTER], 
				&match) == lwc_error_ok && match))
		type = CSS_SELECTOR_PSEUDO_ELEMENT;
	else
		return CSS_INVALID;

	return css_stylesheet_selector_detail_init(c->sheet, 
			type, name->idata, value != NULL ? value->idata : NULL,
			specific);
}

css_error parseSpecific(css_language *c, 
		const parserutils_vector *vector, int *ctx,
		css_selector **parent)
{
	css_error error;
	const css_token *token;
	css_selector_detail specific;

	/* specific  -> [ HASH | class | attrib | pseudo ] */

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (token->type == CSS_TOKEN_HASH) {
		error = css_stylesheet_selector_detail_init(c->sheet,
				CSS_SELECTOR_ID, token->idata, NULL, &specific);
		if (error != CSS_OK)
			return error;

		parserutils_vector_iterate(vector, ctx);
	} else if (tokenIsChar(token, '.')) {
		error = parseClass(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else if (tokenIsChar(token, '[')) {
		error = parseAttrib(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else if (tokenIsChar(token, ':')) {
		error = parsePseudo(c, vector, ctx, &specific);
		if (error != CSS_OK)
			return error;
	} else {
		return CSS_INVALID;
	}

	return css_stylesheet_selector_append_specific(c->sheet, parent, 
			&specific);
}

css_error parseSelectorSpecifics(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_selector **parent)
{
	css_error error;
	const css_token *token;

	/* specifics -> specific* */
	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			token->type != CSS_TOKEN_S && 
			tokenIsChar(token, '+') == false &&
			tokenIsChar(token, '>') == false &&
			tokenIsChar(token, ',') == false) {
		error = parseSpecific(c, vector, ctx, parent);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

css_error parseSimpleSelector(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_selector **result)
{
	css_error error;
	const css_token *token;
	css_selector *selector;

	/* simple_selector -> element_name specifics
	 *		   -> specific specifics
	 * element_name	   -> IDENT | '*'
	 */

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL)
		return CSS_INVALID;

	if (token->type == CSS_TOKEN_IDENT || tokenIsChar(token, '*')) {
		/* Have element name */
		error = css_stylesheet_selector_create(c->sheet,
				token->idata, &selector);
		if (error != CSS_OK)
			return error;

		parserutils_vector_iterate(vector, ctx);
	} else {
		/* Universal selector */
		error = css_stylesheet_selector_create(c->sheet,
				c->strings[UNIVERSAL], &selector);
		if (error != CSS_OK)
			return error;

		/* Ensure we have at least one specific selector */
		error = parseSpecific(c, vector, ctx, &selector);
		if (error != CSS_OK) {
			css_stylesheet_selector_destroy(c->sheet, selector);
			return error;
		}
	}

	error = parseSelectorSpecifics(c, vector, ctx, &selector);
	if (error != CSS_OK) {
		css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	*result = selector;

	return CSS_OK;
}

css_error parseCombinator(css_language *c, const parserutils_vector *vector,
		int *ctx, css_combinator *result)
{
	const css_token *token;
	css_combinator comb = CSS_COMBINATOR_NONE;

	/* combinator	   -> ws '+' ws | ws '>' ws | ws1 */

	UNUSED(c);

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL) {
		if (tokenIsChar(token, '+'))
			comb = CSS_COMBINATOR_SIBLING;
		else if (tokenIsChar(token, '>'))
			comb = CSS_COMBINATOR_PARENT;
		else if (token->type == CSS_TOKEN_S)
			comb = CSS_COMBINATOR_ANCESTOR;
		else
			break;

		parserutils_vector_iterate(vector, ctx);

		/* If we've seen a '+' or '>', we're done. */
		if (comb != CSS_COMBINATOR_ANCESTOR)
			break;
	}

	/* No valid combinator found */
	if (comb == CSS_COMBINATOR_NONE)
		return CSS_INVALID;

	/* Consume any trailing whitespace */
	consumeWhitespace(vector, ctx);

	*result = comb;

	return CSS_OK;
}

css_error parseSelector(css_language *c, const parserutils_vector *vector, 
		int *ctx, css_selector **result)
{
	css_error error;
	const css_token *token = NULL;
	css_selector *selector = NULL;

	/* selector -> simple_selector [ combinator simple_selector ]* ws
	 * 
	 * Note, however, that, as combinator can be wholly whitespace,
	 * there's an ambiguity as to whether "ws" has been reached. We 
	 * resolve this by attempting to extract a combinator, then 
	 * recovering when we detect that we've reached the end of the
	 * selector.
	 */

	error = parseSimpleSelector(c, vector, ctx, &selector);
	if (error != CSS_OK)
		return error;
	*result = selector;

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			tokenIsChar(token, ',') == false) {
		css_combinator comb = CSS_COMBINATOR_NONE;
		css_selector *other = NULL;

		error = parseCombinator(c, vector, ctx, &comb);
		if (error != CSS_OK)
			return error;

		/* In the case of "html , body { ... }", the whitespace after
		 * "html" and "body" will be considered an ancestor combinator.
		 * This clearly is not the case, however. Therefore, as a 
		 * special case, if we've got an ancestor combinator and there 
		 * are no further tokens, or if the next token is a comma,
		 * we ignore the supposed combinator and continue. */
		if (comb == CSS_COMBINATOR_ANCESTOR && 
				((token = parserutils_vector_peek(vector, 
					*ctx)) == NULL || 
				tokenIsChar(token, ',')))
			continue;

		error = parseSimpleSelector(c, vector, ctx, &other);
		if (error != CSS_OK)
			return error;

		*result = other;

		error = css_stylesheet_selector_combine(c->sheet,
				comb, selector, other);
		if (error != CSS_OK) {
			css_stylesheet_selector_destroy(c->sheet, selector);
			return error;
		}

		selector = other;
	}

	return CSS_OK;
}

css_error parseSelectorList(css_language *c, const parserutils_vector *vector,
		css_rule *rule)
{
	css_error error;
	const css_token *token = NULL;
	css_selector *selector = NULL;
	int ctx = 0;

	/* Strip any leading whitespace (can happen if in nested block) */
	consumeWhitespace(vector, &ctx);

	/* selector_list   -> selector [ ',' ws selector ]* */

	error = parseSelector(c, vector, &ctx, &selector);
	if (error != CSS_OK) {
		if (selector != NULL)
			css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	assert(selector != NULL);

	error = css_stylesheet_rule_add_selector(c->sheet, rule, selector);
	if (error != CSS_OK) {
		css_stylesheet_selector_destroy(c->sheet, selector);
		return error;
	}

	while (parserutils_vector_peek(vector, ctx) != NULL) {
		token = parserutils_vector_iterate(vector, &ctx);
		if (tokenIsChar(token, ',') == false)
			return CSS_INVALID;

		consumeWhitespace(vector, &ctx);

		selector = NULL;

		error = parseSelector(c, vector, &ctx, &selector);
		if (error != CSS_OK) {
			if (selector != NULL) {
				css_stylesheet_selector_destroy(c->sheet, 
						selector);
			}
			return error;
		}

		assert(selector != NULL);

		error = css_stylesheet_rule_add_selector(c->sheet, rule, 
				selector);
		if (error != CSS_OK) {
			css_stylesheet_selector_destroy(c->sheet, selector);
			return error;
		}
	}

	return CSS_OK;
}

/******************************************************************************
 * Property parsing functions						      *
 ******************************************************************************/

css_error parseProperty(css_language *c, const css_token *property, 
		const parserutils_vector *vector, int *ctx, css_rule *rule)
{
	css_error error;
	css_prop_handler handler = NULL;
	int i = 0;
	uint8_t flags = 0;
	css_style *style = NULL;
	const css_token *token;

	/* Find property index */
	/** \todo improve on this linear search */
	for (i = FIRST_PROP; i <= LAST_PROP; i++) {
		bool match = false;

		if (lwc_string_caseless_isequal(property->idata, c->strings[i],
				&match) == lwc_error_ok && match)
			break;
	}
	if (i == LAST_PROP + 1)
		return CSS_INVALID;

	/* Get handler */
	handler = property_handlers[i - FIRST_PROP];
	assert(handler != NULL);

	/* Call it */
	error = handler(c, vector, ctx, &style);
	if (error != CSS_OK)
		return error;

	assert (style != NULL);

	/* Determine if this declaration is important or not */
	error = parse_important(c, vector, ctx, &flags);
	if (error != CSS_OK) {
	  css_stylesheet_style_destroy(c->sheet, style, false);
		return error;
	}

	/* Ensure that we've exhausted all the input */
	consumeWhitespace(vector, ctx);
	token = parserutils_vector_iterate(vector, ctx);
	if (token != NULL) {
		/* Trailing junk, so discard declaration */
                css_stylesheet_style_destroy(c->sheet, style, false);
		return CSS_INVALID;
	}

	/* If it's important, then mark the style appropriately */
	if (flags != 0)
		make_style_important(style);

	/* Append style to rule */
	error = css_stylesheet_rule_append_style(c->sheet, rule, style);
	if (error != CSS_OK) {
                css_stylesheet_style_destroy(c->sheet, style, false);
		return error;
	}

	/* Style owned or destroyed by stylesheet, so forget about it */

	return CSS_OK;
}

