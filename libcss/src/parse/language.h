/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_parse_language_h_
#define css_parse_language_h_

#include <parserutils/utils/stack.h>
#include <parserutils/utils/vector.h>

#include <libcss/functypes.h>
#include <libcss/types.h>

#include "lex/lex.h"
#include "parse/parse.h"
#include "parse/propstrings.h"

/**
 * Context for a CSS language parser
 */
typedef struct css_language {
	css_stylesheet *sheet;		/**< The stylesheet to parse for */

#define STACK_CHUNK 32
	parserutils_stack *context;	/**< Context stack */

	enum {
		BEFORE_CHARSET,
		BEFORE_RULES,
		HAD_RULE
	} state;			/**< State flag, for at-rule handling */

	/** \todo These should be statically allocated */
	/** Interned strings */
	lwc_string *strings[LAST_KNOWN];

	css_allocator_fn alloc;		/**< Memory (de)allocation function */
	void *pw;			/**< Client's private data */
} css_language;

css_error css_language_create(css_stylesheet *sheet, css_parser *parser,
		css_allocator_fn alloc, void *pw, void **language);
css_error css_language_destroy(css_language *language);

/******************************************************************************
 * Helper functions                                                           *
 ******************************************************************************/

/**
 * Consume all leading whitespace tokens
 *
 * \param vector  The vector to consume from
 * \param ctx     The vector's context
 */
static inline void consumeWhitespace(const parserutils_vector *vector, int *ctx)
{
	const css_token *token = NULL;

	while ((token = parserutils_vector_peek(vector, *ctx)) != NULL &&
			token->type == CSS_TOKEN_S)
		parserutils_vector_iterate(vector, ctx);
}

/**
 * Determine if a token is a character
 *
 * \param token  The token to consider
 * \param c      The character to match (lowercase ASCII only)
 * \return True if the token matches, false otherwise
 */
static inline bool tokenIsChar(const css_token *token, uint8_t c)
{
	bool result = false;

	if (token != NULL && token->type == CSS_TOKEN_CHAR && 
	                lwc_string_length(token->idata) == 1) {
		char d = lwc_string_data(token->idata)[0];

		/* Ensure lowercase comparison */
		if ('A' <= d && d <= 'Z')
			d += 'a' - 'A';

		result = (d == c);
	}

	return result;
}

#endif

