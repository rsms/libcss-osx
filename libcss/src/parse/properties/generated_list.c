/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

static css_error parse_list_style_type_value(css_language *c,
		const css_token *token, uint16_t *value);
static css_error parse_content_list(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t *value, uint8_t *buffer, uint32_t *buflen);
static css_error parse_counter_common(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result);

/**
 * Parse content
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_content(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t required_size = sizeof(opv);
	int temp_ctx = *ctx;
	uint8_t *ptr;
	bool match;

	/* IDENT(normal, none, inherit) |
	 * [
	 *	IDENT(open-quote, close-quote, no-open-quote, no-close-quote) |
	 *	STRING | URI |
	 *	FUNCTION(attr) IDENT ')' |
	 *	FUNCTION(counter) IDENT IDENT? ')' |
	 *	FUNCTION(counters) IDENT STRING IDENT? ')'
	 * ]+
	 */

	/* Pass 1: Calculate required size & validate input */
	token = parserutils_vector_peek(vector, temp_ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags = FLAG_INHERIT;
		parserutils_vector_iterate(vector, &temp_ctx);
	} else if (token->type == CSS_TOKEN_IDENT &&
			 (lwc_string_caseless_isequal(
			token->idata, c->strings[NORMAL],
			&match) == lwc_error_ok && match)) {
		value = CONTENT_NORMAL;
		parserutils_vector_iterate(vector, &temp_ctx);
	} else if (token->type == CSS_TOKEN_IDENT &&
			 (lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = CONTENT_NONE;
		parserutils_vector_iterate(vector, &temp_ctx);
	} else {
		uint32_t len;

		error = parse_content_list(c, vector, &temp_ctx, &value,
				NULL, &len);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		required_size += len;
	}

	opv = buildOPV(CSS_PROP_CONTENT, flags, value);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy OPV to bytecode */
	ptr = (*result)->bytecode;
	memcpy(ptr, &opv, sizeof(opv));
	ptr += sizeof(opv);

	/* Pass 2: construct bytecode */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			((lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match) ||
			 (lwc_string_caseless_isequal(
			token->idata, c->strings[NORMAL],
			&match) == lwc_error_ok && match) ||
			 (lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match))) {
		parserutils_vector_iterate(vector, ctx);
	} else {
		error = parse_content_list(c, vector, ctx, NULL, ptr, NULL);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}
	}

	return CSS_OK;
}

/**
 * Parse counter-increment
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_counter_increment(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_counter_common(c, vector, ctx, 
			CSS_PROP_COUNTER_INCREMENT, result);
}

/**
 * Parse counter-reset
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_counter_reset(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_counter_common(c, vector, ctx,
			CSS_PROP_COUNTER_RESET, result);
}

/**
 * Parse list-style
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_list_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *image = NULL;
	css_style *position = NULL;
	css_style *type = NULL;
	css_style *ret = NULL;
	uint32_t required_size;
	bool match;
	css_error error;

	/* Firstly, handle inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		uint32_t *bytecode;

		error = css_stylesheet_style_create(c->sheet, 
				3 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_LIST_STYLE_IMAGE, 
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_LIST_STYLE_POSITION,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_LIST_STYLE_TYPE,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse the various longhand properties */
	do {
		prev_ctx = *ctx;
		error = CSS_OK;

		/* Ensure that we're not about to parse another inherit */
		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[INHERIT],
				&match) == lwc_error_ok && match)) {
			error = CSS_INVALID;
			goto cleanup;
		}

		if (type == NULL && (error = parse_list_style_type(c, vector,
				ctx, &type)) == CSS_OK) {
		} else if (position == NULL && 
				(error = parse_list_style_position(c, vector, 
				ctx, &position)) == CSS_OK) {
		} else if (image == NULL && 
				(error = parse_list_style_image(c, vector, ctx,
				&image)) == CSS_OK) {
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	/* Calculate the required size of the resultant style,
	 * defaulting the unspecified properties to their initial values */
	required_size = 0;

	if (image)
		required_size += image->length;
	else
		required_size += sizeof(uint32_t);

	if (position)
		required_size += position->length;
	else
		required_size += sizeof(uint32_t);

	if (type)
		required_size += type->length;
	else
		required_size += sizeof(uint32_t);

	/* Create and populate it */
	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (image) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				image->bytecode, image->length);
		required_size += image->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_LIST_STYLE_IMAGE,
				0, LIST_STYLE_IMAGE_NONE);
		required_size += sizeof(uint32_t);
	}

	if (position) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				position->bytecode, position->length);
		required_size += position->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_LIST_STYLE_POSITION,
				0, LIST_STYLE_POSITION_OUTSIDE);
		required_size += sizeof(uint32_t);
	}

	if (type) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				type->bytecode, type->length);
		required_size += type->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_LIST_STYLE_TYPE,
				0, LIST_STYLE_TYPE_DISC);
		required_size += sizeof(uint32_t);
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (image)
		css_stylesheet_style_destroy(c->sheet, image, error == CSS_OK);
	if (position)
		css_stylesheet_style_destroy(c->sheet, position, error == CSS_OK);
	if (type)
		css_stylesheet_style_destroy(c->sheet, type, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse list-style-image
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_list_style_image(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t required_size;
	bool match;
	lwc_string *uri = NULL;

	/* URI | IDENT (none, inherit) */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_URI)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT && 
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags |= FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT && 
			(lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = LIST_STYLE_IMAGE_NONE;
	} else if (token->type == CSS_TOKEN_URI) {
		value = LIST_STYLE_IMAGE_URI;

		error = c->sheet->resolve(c->sheet->resolve_pw,
				c->sheet->url,
				token->idata, &uri);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_LIST_STYLE_IMAGE, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == LIST_STYLE_IMAGE_URI)
		required_size += sizeof(lwc_string *);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == LIST_STYLE_IMAGE_URI) {
		/* Don't ref URI -- we want to pass ownership to the bytecode */
		memcpy((uint8_t *) (*result)->bytecode + sizeof(opv),
				&uri, sizeof(lwc_string *));
	}

	return CSS_OK;
}

/**
 * Parse list-style-position
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_list_style_position(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *ident;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	bool match;

	/* IDENT (inside, outside, inherit) */
	ident = parserutils_vector_iterate(vector, ctx);
	if (ident == NULL || ident->type != CSS_TOKEN_IDENT) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags |= FLAG_INHERIT;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[INSIDE],
			&match) == lwc_error_ok && match)) {
		value = LIST_STYLE_POSITION_INSIDE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[OUTSIDE],
			&match) == lwc_error_ok && match)) {
		value = LIST_STYLE_POSITION_OUTSIDE;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_LIST_STYLE_POSITION, flags, value);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, sizeof(opv), result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));

	return CSS_OK;
}

/**
 * Parse list-style-type
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_list_style_type(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *ident;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	bool match;

	/* IDENT (disc, circle, square, decimal, decimal-leading-zero,
	 *	  lower-roman, upper-roman, lower-greek, lower-latin,
	 *	  upper-latin, armenian, georgian, lower-alpha, upper-alpha,
	 *	  none, inherit)
	 */
	ident = parserutils_vector_iterate(vector, ctx);
	if (ident == NULL || ident->type != CSS_TOKEN_IDENT) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags |= FLAG_INHERIT;
	} else {
		error = parse_list_style_type_value(c, ident, &value);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}
	}

	opv = buildOPV(CSS_PROP_LIST_STYLE_TYPE, flags, value);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, sizeof(opv), result);
	if (error != CSS_OK) {	
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));

	return CSS_OK;
}

/**
 * Parse quotes
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_quotes(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t required_size = sizeof(opv);
	int temp_ctx = *ctx;
	uint8_t *ptr;
	bool match;

	/* [ STRING STRING ]+ | IDENT(none,inherit) */ 

	/* Pass 1: validate input and calculate bytecode size */
	token = parserutils_vector_iterate(vector, &temp_ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_STRING)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT) {
		if ((lwc_string_caseless_isequal(
				token->idata, c->strings[INHERIT],
				&match) == lwc_error_ok && match)) {
			flags = FLAG_INHERIT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[NONE],
				&match) == lwc_error_ok && match)) {
			value = QUOTES_NONE;
		} else {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}
	} else {
		bool first = true;

		/* [ STRING STRING ] + */
		while (token != NULL && token->type == CSS_TOKEN_STRING) {
			lwc_string *open = token->idata;
			lwc_string *close;

			consumeWhitespace(vector, &temp_ctx);

			token = parserutils_vector_peek(vector, temp_ctx);
			if (token == NULL || token->type != CSS_TOKEN_STRING) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			close = token->idata;

			parserutils_vector_iterate(vector, &temp_ctx);

			consumeWhitespace(vector, &temp_ctx);

			if (first == false) {
				required_size += sizeof(opv);
			} else {
				value = QUOTES_STRING;
			}
			required_size += sizeof(open) + sizeof(close);

			first = false;

			token = parserutils_vector_peek(vector, temp_ctx);
			if (token == NULL || token->type != CSS_TOKEN_STRING)
				break;
			token = parserutils_vector_iterate(vector, &temp_ctx);
		}

		/* Terminator */
		required_size += sizeof(opv);
	}

	opv = buildOPV(CSS_PROP_QUOTES, flags, value);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy OPV to bytecode */
	ptr = (*result)->bytecode;
	memcpy(ptr, &opv, sizeof(opv));
	ptr += sizeof(opv);

	/* Pass 2: construct bytecode */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_STRING)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT) {
		/* Nothing to do */
	} else {
		bool first = true;

		/* [ STRING STRING ]+ */
		while (token != NULL && token->type == CSS_TOKEN_STRING) {
			lwc_string *open = token->idata;
			lwc_string *close;

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || token->type != CSS_TOKEN_STRING) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			close = token->idata;

			parserutils_vector_iterate(vector, ctx);

			consumeWhitespace(vector, ctx);

			if (first == false) {
				opv = QUOTES_STRING;
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}
			
			lwc_string_ref(open);
			memcpy(ptr, &open, sizeof(open));
			ptr += sizeof(open);
			
			lwc_string_ref(close);
			memcpy(ptr, &close, sizeof(close));
			ptr += sizeof(close);

			first = false;

			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || token->type != CSS_TOKEN_STRING)
				break;
			token = parserutils_vector_iterate(vector, ctx);
		}

		/* Terminator */
		opv = QUOTES_NONE;
		memcpy(ptr, &opv, sizeof(opv));
		ptr += sizeof(opv);
	}

	return CSS_OK;
}

/**
 * Parse list-style-type value
 *
 * \param c	 Parsing context
 * \param ident	 Identifier to consider
 * \param value	 Pointer to location to receive value
 * \return CSS_OK on success,
 *	   CSS_INVALID if the input is not valid
 */
css_error parse_list_style_type_value(css_language *c, const css_token *ident,
		uint16_t *value)
{
	bool match;

	/* IDENT (disc, circle, square, decimal, decimal-leading-zero,
	 *	  lower-roman, upper-roman, lower-greek, lower-latin,
	 *	  upper-latin, armenian, georgian, lower-alpha, upper-alpha,
	 *	  none)
	 */
	if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DISC],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_DISC;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[CIRCLE],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_CIRCLE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SQUARE],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_SQUARE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DECIMAL],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_DECIMAL;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DECIMAL_LEADING_ZERO],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LOWER_ROMAN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_LOWER_ROMAN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[UPPER_ROMAN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_UPPER_ROMAN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LOWER_GREEK],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_LOWER_GREEK;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LOWER_LATIN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_LOWER_LATIN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[UPPER_LATIN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_UPPER_LATIN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[ARMENIAN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_ARMENIAN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[GEORGIAN],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_GEORGIAN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LOWER_ALPHA],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_LOWER_ALPHA;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[UPPER_ALPHA],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_UPPER_ALPHA;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		*value = LIST_STYLE_TYPE_NONE;
	} else
		return CSS_INVALID;

	return CSS_OK;
}

/**
 * Parse content list
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param value	  Pointer to location to receive value
 * \param buffer  Pointer to output buffer, or NULL to read required length
 * \param buflen  Pointer to location to receive buffer length
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_content_list(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t *value, uint8_t *buffer, uint32_t *buflen)
{
	int orig_ctx = *ctx;
	int prev_ctx = *ctx;
	css_error error;
	const css_token *token;
	bool first = true;
	uint32_t offset = 0;
	uint32_t opv;
	bool match;

	/* [
	 *	IDENT(open-quote, close-quote, no-open-quote, no-close-quote) |
	 *	STRING | URI |
	 *	FUNCTION(attr) IDENT ')' |
	 *	FUNCTION(counter) IDENT (',' IDENT)? ')' |
	 *	FUNCTION(counters) IDENT ',' STRING (',' IDENT)? ')'
	 * ]+
	 */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	while (token != NULL) {
		if (token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[OPEN_QUOTE],
				&match) == lwc_error_ok && match)) {
			opv = CONTENT_OPEN_QUOTE;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			} 
		} else if (token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[CLOSE_QUOTE],
				&match) == lwc_error_ok && match)) {
			opv = CONTENT_CLOSE_QUOTE;

			if (first == false) {				
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}
		} else if (token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[NO_OPEN_QUOTE],
				&match) == lwc_error_ok && match)) {
			opv = CONTENT_NO_OPEN_QUOTE;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}
		} else if (token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[NO_CLOSE_QUOTE],
				&match) == lwc_error_ok && match)) {
			opv = CONTENT_NO_CLOSE_QUOTE;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}
		} else if (token->type == CSS_TOKEN_STRING) {
			opv = CONTENT_STRING;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}

			if (buffer != NULL) {
				lwc_string_ref(token->idata);
				memcpy(buffer + offset, &token->idata,
						sizeof(token->idata));
			}

			offset += sizeof(token->idata);
		} else if (token->type == CSS_TOKEN_URI) {
			lwc_string *uri;

			opv = CONTENT_URI;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}

			if (buffer != NULL) {
				error = c->sheet->resolve(c->sheet->resolve_pw,
					c->sheet->url,
					token->idata, &uri);
				if (error != CSS_OK) {
					*ctx = orig_ctx;
					return error;
				}

				/* Don't ref URI -- we want to pass ownership 
				 * to the bytecode */
				memcpy(buffer + offset, &uri, sizeof(uri));
			}

			offset += sizeof(uri);
		} else if (token->type == CSS_TOKEN_FUNCTION &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[ATTR],
				&match) == lwc_error_ok && match)) {
			opv = CONTENT_ATTR;

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}

			consumeWhitespace(vector, ctx);

			/* Expect IDENT */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || token->type != CSS_TOKEN_IDENT) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (buffer != NULL) {
				lwc_string_ref(token->idata);
				memcpy(buffer + offset, &token->idata, 
						sizeof(token->idata));
			}

			offset += sizeof(token->idata);

			consumeWhitespace(vector, ctx);

			/* Expect ')' */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || tokenIsChar(token, ')') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (token->type == CSS_TOKEN_FUNCTION &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[COUNTER],
				&match) == lwc_error_ok && match)) {
			lwc_string *name;

			opv = CONTENT_COUNTER;

			consumeWhitespace(vector, ctx);

			/* Expect IDENT */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || token->type != CSS_TOKEN_IDENT) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			name = token->idata;

			consumeWhitespace(vector, ctx);

			/* Possible ',' */
			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || 
					(tokenIsChar(token, ',') == false &&
					tokenIsChar(token, ')') == false)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (tokenIsChar(token, ',')) {
				uint16_t v;

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);

				/* Expect IDENT */
				token = parserutils_vector_peek(vector, *ctx);
				if (token == NULL || token->type != 
						CSS_TOKEN_IDENT) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				error = parse_list_style_type_value(c,
						token, &v);
				if (error != CSS_OK) {
					*ctx = orig_ctx;
					return error;
				}

				opv |= v << CONTENT_COUNTER_STYLE_SHIFT;

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);
			} else {
				opv |= LIST_STYLE_TYPE_DECIMAL << 
						CONTENT_COUNTER_STYLE_SHIFT;
			}

			/* Expect ')' */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || tokenIsChar(token,	')') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}

			if (buffer != NULL) {
				lwc_string_ref(name);
				memcpy(buffer + offset, &name, sizeof(name));
			}

			offset += sizeof(name);
		} else if (token->type == CSS_TOKEN_FUNCTION &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[COUNTERS],
				&match) == lwc_error_ok && match)) {
			lwc_string *name;
			lwc_string *sep;

			opv = CONTENT_COUNTERS;

			consumeWhitespace(vector, ctx);

			/* Expect IDENT */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || token->type != CSS_TOKEN_IDENT) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			name = token->idata;

			consumeWhitespace(vector, ctx);

			/* Expect ',' */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || tokenIsChar(token, ',') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			consumeWhitespace(vector, ctx);

			/* Expect STRING */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || token->type != CSS_TOKEN_STRING) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			sep = token->idata;

			consumeWhitespace(vector, ctx);

			/* Possible ',' */
			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || 
					(tokenIsChar(token, ',') == false && 
					tokenIsChar(token, ')') == false)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (tokenIsChar(token, ',')) {
				uint16_t v;

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);

				/* Expect IDENT */
				token = parserutils_vector_peek(vector, *ctx);
				if (token == NULL || token->type != 
						CSS_TOKEN_IDENT) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				error = parse_list_style_type_value(c,
						token, &v);
				if (error != CSS_OK) {
					*ctx = orig_ctx;
					return error;
				}

				opv |= v << CONTENT_COUNTERS_STYLE_SHIFT;

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);
			} else {
				opv |= LIST_STYLE_TYPE_DECIMAL <<
						CONTENT_COUNTERS_STYLE_SHIFT;
			}

			/* Expect ')' */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || tokenIsChar(token, ')') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (first == false) {
				if (buffer != NULL) {
					memcpy(buffer + offset, 
							&opv, sizeof(opv));
				}

				offset += sizeof(opv);
			}

			if (buffer != NULL) {
				lwc_string_ref(name);
				memcpy(buffer + offset, &name, sizeof(name));
			}

			offset += sizeof(name);

			if (buffer != NULL) {
				lwc_string_ref(sep);
				memcpy(buffer + offset, &sep, sizeof(sep));
			}

			offset += sizeof(sep);
		} else if (first) {
			/* Invalid if this is the first iteration */
			*ctx = orig_ctx;
			return CSS_INVALID;
		} else {
			/* Give up, ensuring current token is reprocessed */
			*ctx = prev_ctx;
			break;
		}

		if (first && value != NULL) {
			*value = opv;
		}
		first = false;

		consumeWhitespace(vector, ctx);

		prev_ctx = *ctx;
		token = parserutils_vector_iterate(vector, ctx);
	}

	/* Write list terminator */
	opv = CONTENT_NORMAL;

	if (buffer != NULL) {
		memcpy(buffer + offset, &opv, sizeof(opv));
	}

	offset += sizeof(opv);

	if (buflen != NULL) {
		*buflen = offset;
	}

	return CSS_OK;
}

/**
 * Common parser for counter-increment and counter-reset
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param op	  Opcode to parse for
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_counter_common(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		uint16_t op, css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t required_size = sizeof(opv);
	int temp_ctx = *ctx;
	uint8_t *ptr;
	bool match;

	/* [IDENT <integer>? ]+ | IDENT(none, inherit) */

	/* Pass 1: validate input and calculate bytecode size */
	token = parserutils_vector_iterate(vector, &temp_ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags = FLAG_INHERIT;
	} else if ((lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = COUNTER_INCREMENT_NONE;
	} else {
		bool first = true;

		value = COUNTER_INCREMENT_NAMED;

		while (token != NULL) {
			lwc_string *name = token->idata;
			css_fixed increment = 
					(op == CSS_PROP_COUNTER_INCREMENT) 
					? INTTOFIX(1) : INTTOFIX(0);

			consumeWhitespace(vector, &temp_ctx);

			/* Optional integer */
			token = parserutils_vector_peek(vector, temp_ctx);
			if (token != NULL && token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_NUMBER) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (token != NULL && token->type == CSS_TOKEN_NUMBER) {
				size_t consumed = 0;

				increment = number_from_lwc_string(
						token->idata, true, &consumed);

				if (consumed != lwc_string_length(
						token->idata)) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				parserutils_vector_iterate(vector, &temp_ctx);

				consumeWhitespace(vector, &temp_ctx);
			}

			if (first == false) {
				required_size += sizeof(opv);
			}
			required_size += sizeof(name) + sizeof(increment);

			first = false;

			token = parserutils_vector_peek(vector, temp_ctx);
			if (token != NULL && token->type != CSS_TOKEN_IDENT) {
				break;
			}

			token = parserutils_vector_iterate(vector, &temp_ctx);
		}

		/* And for the terminator */
		required_size += sizeof(opv);
	}

	opv = buildOPV(op, flags, value);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy OPV to bytecode */
	ptr = (*result)->bytecode;
	memcpy(ptr, &opv, sizeof(opv));
	ptr += sizeof(opv);

	/* Pass 2: construct bytecode */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match) ||
			(lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		/* Nothing to do */
	} else {
		bool first = true;

		opv = COUNTER_INCREMENT_NAMED;

		while (token != NULL) {
			lwc_string *name = token->idata;
			css_fixed increment = 
					(op == CSS_PROP_COUNTER_INCREMENT) 
					? INTTOFIX(1) : INTTOFIX(0);

			consumeWhitespace(vector, ctx);

			/* Optional integer */
			token = parserutils_vector_peek(vector, *ctx);
			if (token != NULL && token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_NUMBER) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (token != NULL && token->type == CSS_TOKEN_NUMBER) {
				size_t consumed = 0;

				increment = number_from_lwc_string(
						token->idata, true, &consumed);

				if (consumed != lwc_string_length(
						token->idata)) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);
			}

			if (first == false) {
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}
			
			lwc_string_ref(name);
			memcpy(ptr, &name, sizeof(name));
			ptr += sizeof(name);
			
			memcpy(ptr, &increment, sizeof(increment));
			ptr += sizeof(increment);

			first = false;

			token = parserutils_vector_peek(vector, *ctx);
			if (token != NULL && token->type != CSS_TOKEN_IDENT) {
				break;
			}

			token = parserutils_vector_iterate(vector, ctx);
		}

		/* And for the terminator */
		opv = COUNTER_INCREMENT_NONE;
		memcpy(ptr, &opv, sizeof(opv));
		ptr += sizeof(opv);
	}

	return CSS_OK;
}

