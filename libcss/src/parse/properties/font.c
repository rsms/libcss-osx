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

/**
 * Parse font
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
css_error parse_font(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *style = NULL;
	css_style *variant = NULL;
	css_style *weight = NULL;
	css_style *size = NULL;
	css_style *line_height = NULL;
	css_style *family = NULL;
	css_style *ret = NULL;
	uint32_t required_size;
	bool match;
	int svw;
	css_error error;

	/* Firstly, handle inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		uint32_t *bytecode;

		error = css_stylesheet_style_create(c->sheet, 
				6 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_FONT_STYLE, 
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_FONT_VARIANT,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_FONT_WEIGHT,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_FONT_SIZE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_LINE_HEIGHT,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_FONT_FAMILY,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse the optional style, variant, and weight */
	for (svw = 0; svw < 3; svw++) {
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

		if (style == NULL && 
				(error = parse_font_style(c, vector,
				ctx, &style)) == CSS_OK) {
		} else if (variant == NULL && 
				(error = parse_font_variant(c, vector, ctx,
				&variant)) == CSS_OK) {
		} else if (weight == NULL && 
				(error = parse_font_weight(c, vector, ctx,
				&weight)) == CSS_OK) {
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);
		} else {
			break;
		}

		if (*ctx == prev_ctx)
			break;
	}

	consumeWhitespace(vector, ctx);

	/* Ensure that we're not about to parse another inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Now expect a font-size */
	error = parse_font_size(c, vector, ctx, &size);
	if (error != CSS_OK)
		goto cleanup;

	consumeWhitespace(vector, ctx);

	/* Potential line-height */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && tokenIsChar(token, '/')) {
		parserutils_vector_iterate(vector, ctx);

		consumeWhitespace(vector, ctx);

		/* Ensure that we're not about to parse another inherit */
		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && token->type == CSS_TOKEN_IDENT &&
				(lwc_string_caseless_isequal(
				token->idata, c->strings[INHERIT],
				&match) == lwc_error_ok && match)) {
			error = CSS_INVALID;
			goto cleanup;
		}

		error = parse_line_height(c, vector, ctx, &line_height);
		if (error != CSS_OK)
			goto cleanup;
	}

	consumeWhitespace(vector, ctx);

	/* Ensure that we're not about to parse another inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Now expect a font-family */
	error = parse_font_family(c, vector, ctx, &family);
	if (error != CSS_OK)
		goto cleanup;

	/* Must have size and family */
	assert(size != NULL);
	assert(family != NULL);

	/* Calculate the required size of the resultant style,
	 * defaulting the unspecified properties to their initial values */
	required_size = 0;

	if (style)
		required_size += style->length;
	else
		required_size += sizeof(uint32_t);

	if (variant)
		required_size += variant->length;
	else
		required_size += sizeof(uint32_t);

	if (weight)
		required_size += weight->length;
	else
		required_size += sizeof(uint32_t);

	required_size += size->length;

	if (line_height)
		required_size += line_height->length;
	else
		required_size += sizeof(uint32_t);

	required_size += family->length;

	/* Create and populate it */
	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (style) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				style->bytecode, style->length);
		required_size += style->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_FONT_STYLE,
				0, FONT_STYLE_NORMAL);
		required_size += sizeof(uint32_t);
	}

	if (variant) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				variant->bytecode, variant->length);
		required_size += variant->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_FONT_VARIANT,
				0, FONT_VARIANT_NORMAL);
		required_size += sizeof(uint32_t);
	}

	if (weight) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				weight->bytecode, weight->length);
		required_size += weight->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_FONT_WEIGHT,
				0, FONT_WEIGHT_NORMAL);
		required_size += sizeof(uint32_t);
	}

	memcpy(((uint8_t *) ret->bytecode) + required_size,
			size->bytecode, size->length);
	required_size += size->length;

	if (line_height) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				line_height->bytecode, line_height->length);
		required_size += line_height->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_LINE_HEIGHT,
				0, LINE_HEIGHT_NORMAL);
		required_size += sizeof(uint32_t);
	}

	memcpy(((uint8_t *) ret->bytecode) + required_size,
			family->bytecode, family->length);
	required_size += family->length;

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (style)
		css_stylesheet_style_destroy(c->sheet, style, error == CSS_OK);
	if (variant)
		css_stylesheet_style_destroy(c->sheet, variant, error == CSS_OK);
	if (weight)
		css_stylesheet_style_destroy(c->sheet, weight, error == CSS_OK);
	if (size)
		css_stylesheet_style_destroy(c->sheet, size, error == CSS_OK);
	if (line_height)
		css_stylesheet_style_destroy(c->sheet, line_height, error == CSS_OK);
	if (family)
		css_stylesheet_style_destroy(c->sheet, family, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Determine if a given font-family ident is reserved
 *
 * \param c	 Parsing context
 * \param ident	 IDENT to consider
 * \return True if IDENT is reserved, false otherwise
 */
static bool font_family_reserved(css_language *c, const css_token *ident)
{
	bool match;

	return (lwc_string_caseless_isequal(
			ident->idata, c->strings[SERIF],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[SANS_SERIF],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[CURSIVE],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[FANTASY],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[MONOSPACE],
			&match) == lwc_error_ok && match);
}

/**
 * Convert a font-family token into a bytecode value
 *
 * \param c	 Parsing context
 * \param token	 Token to consider
 * \return Bytecode value
 */
static uint16_t font_family_value(css_language *c, const css_token *token)
{
	uint16_t value;
	bool match;

	if (token->type == CSS_TOKEN_IDENT) {
		if ((lwc_string_caseless_isequal(
				token->idata, c->strings[SERIF],
				&match) == lwc_error_ok && match))
			value = FONT_FAMILY_SERIF;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[SANS_SERIF],
				&match) == lwc_error_ok && match))
			value = FONT_FAMILY_SANS_SERIF;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[CURSIVE],
				&match) == lwc_error_ok && match))
			value = FONT_FAMILY_CURSIVE;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[FANTASY],
				&match) == lwc_error_ok && match))
			value = FONT_FAMILY_FANTASY;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[MONOSPACE],
				&match) == lwc_error_ok && match))
			value = FONT_FAMILY_MONOSPACE;
		else
			value = FONT_FAMILY_IDENT_LIST;
	} else {
		value = FONT_FAMILY_STRING;
	}

	return value;
}

/**
 * Parse font-family
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
css_error parse_font_family(css_language *c, 
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

	/* [ IDENT+ | STRING ] [ ',' [ IDENT+ | STRING ] ]* | IDENT(inherit)
	 * 
	 * In the case of IDENT+, any whitespace between tokens is collapsed to
	 * a single space
	 *
	 * \todo Mozilla makes the comma optional. 
	 * Perhaps this is a quirk we should inherit?
	 */

	/* Pass 1: validate input and calculate space */
	token = parserutils_vector_iterate(vector, &temp_ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_STRING)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags = FLAG_INHERIT;
	} else {
		uint32_t list_size;

		value = font_family_value(c, token);

		error = comma_list_length(c, vector, &temp_ctx,
				token, font_family_reserved, &list_size);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		required_size += list_size;
	}

	opv = buildOPV(CSS_PROP_FONT_FAMILY, flags, value);

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

	/* Pass 2: populate bytecode */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_STRING)) {
		css_stylesheet_style_destroy(c->sheet, *result, false);
		*result = NULL;
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		/* Nothing to do */
	} else {
		error = comma_list_to_bytecode(c, vector, ctx, token,
				font_family_reserved, font_family_value,
				&ptr);
		if (error != CSS_OK) {
			css_stylesheet_style_destroy(c->sheet, *result, false);
			*result = NULL;
			*ctx = orig_ctx;
			return error;
		}

		/* Write terminator */
		opv = FONT_FAMILY_END;
		memcpy(ptr, &opv, sizeof(opv));
		ptr += sizeof(opv);
	}

	return CSS_OK;
}

/**
 * Parse font-size
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
css_error parse_font_size(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed length = 0;
	uint32_t unit = 0;
	uint32_t required_size;
	bool match;

	/* length | percentage | IDENT(xx-small, x-small, small, medium,
	 * large, x-large, xx-large, larger, smaller, inherit) */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		flags = FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[XX_SMALL],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_XX_SMALL;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[X_SMALL],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_X_SMALL;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[SMALL],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_SMALL;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[MEDIUM],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_MEDIUM;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[LARGE],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_LARGE;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[X_LARGE],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_X_LARGE;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[XX_LARGE],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_XX_LARGE;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[LARGER],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_LARGER;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[SMALLER],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = FONT_SIZE_SMALLER;
	} else {
		error = parse_unit_specifier(c, vector, ctx, UNIT_PX,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if (unit & UNIT_ANGLE || unit & UNIT_TIME || unit & UNIT_FREQ) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Negative values are not permitted */
		if (length < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = FONT_SIZE_DIMENSION;
	}

	opv = buildOPV(CSS_PROP_FONT_SIZE, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == FONT_SIZE_DIMENSION)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == FONT_SIZE_DIMENSION) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

/**
 * Parse font-style
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
css_error parse_font_style(css_language *c, 
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

	/* IDENT (normal, italic, oblique, inherit) */
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
			ident->idata, c->strings[NORMAL],
			&match) == lwc_error_ok && match)) {
		value = FONT_STYLE_NORMAL;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[ITALIC],
			&match) == lwc_error_ok && match)) {
		value = FONT_STYLE_ITALIC;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[OBLIQUE],
			&match) == lwc_error_ok && match)) {
		value = FONT_STYLE_OBLIQUE;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_FONT_STYLE, flags, value);

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
 * Parse font-variant
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
css_error parse_font_variant(css_language *c, 
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

	/* IDENT (normal, small-caps, inherit) */
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
			ident->idata, c->strings[NORMAL],
			&match) == lwc_error_ok && match)) {
		value = FONT_VARIANT_NORMAL;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SMALL_CAPS],
			&match) == lwc_error_ok && match)) {
		value = FONT_VARIANT_SMALL_CAPS;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_FONT_VARIANT, flags, value);

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
 * Parse font-weight
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
css_error parse_font_weight(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	bool match;

	/* NUMBER (100, 200, 300, 400, 500, 600, 700, 800, 900) | 
	 * IDENT (normal, bold, bolder, lighter, inherit) */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_NUMBER)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags |= FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_NUMBER) {
		size_t consumed = 0;
		css_fixed num = number_from_lwc_string(token->idata, 
				true, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		switch (FIXTOINT(num)) {
		case 100: value = FONT_WEIGHT_100; break;
		case 200: value = FONT_WEIGHT_200; break;
		case 300: value = FONT_WEIGHT_300; break;
		case 400: value = FONT_WEIGHT_400; break;
		case 500: value = FONT_WEIGHT_500; break;
		case 600: value = FONT_WEIGHT_600; break;
		case 700: value = FONT_WEIGHT_700; break;
		case 800: value = FONT_WEIGHT_800; break;
		case 900: value = FONT_WEIGHT_900; break;
		default: *ctx = orig_ctx; return CSS_INVALID;
		}
	} else if ((lwc_string_caseless_isequal(
			token->idata, c->strings[NORMAL],
			&match) == lwc_error_ok && match)) {
		value = FONT_WEIGHT_NORMAL;
	} else if ((lwc_string_caseless_isequal(
			token->idata, c->strings[BOLD],
			&match) == lwc_error_ok && match)) {
		value = FONT_WEIGHT_BOLD;
	} else if ((lwc_string_caseless_isequal(
			token->idata, c->strings[BOLDER],
			&match) == lwc_error_ok && match)) {
		value = FONT_WEIGHT_BOLDER;
	} else if ((lwc_string_caseless_isequal(
			token->idata, c->strings[LIGHTER],
			&match) == lwc_error_ok && match)) {
		value = FONT_WEIGHT_LIGHTER;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_FONT_WEIGHT, flags, value);

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

