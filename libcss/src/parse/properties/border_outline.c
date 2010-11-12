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

enum { SIDE_TOP = 0, SIDE_RIGHT = 1, SIDE_BOTTOM = 2, SIDE_LEFT = 3 };

static css_error parse_border_side(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint32_t side, css_style **result);
static css_error parse_border_side_color(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result);
static css_error parse_border_side_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result);
static css_error parse_border_side_width(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result);

/**
 * Parse border shorthand
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
css_error parse_border(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	css_style *top = NULL;
	css_style *right = NULL;
	css_style *bottom = NULL;
	css_style *left = NULL;
	css_style *ret = NULL;
	uint32_t required_size;
	css_error error;

	error = parse_border_side(c, vector, ctx, SIDE_TOP, &top);
	if (error != CSS_OK)
		goto cleanup;

	*ctx = orig_ctx;
	error = parse_border_side(c, vector, ctx, SIDE_RIGHT, &right);
	if (error != CSS_OK)
		goto cleanup;

	*ctx = orig_ctx;
	error = parse_border_side(c, vector, ctx, SIDE_BOTTOM, &bottom);
	if (error != CSS_OK)
		goto cleanup;

	*ctx = orig_ctx;
	error = parse_border_side(c, vector, ctx, SIDE_LEFT, &left);
	if (error != CSS_OK)
		goto cleanup;

	required_size = top->length + right->length + 
			bottom->length + left->length;

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	memcpy(((uint8_t *) ret->bytecode) + required_size, 
			top->bytecode, top->length);
	required_size += top->length;

	memcpy(((uint8_t *) ret->bytecode) + required_size, 
			right->bytecode, right->length);
	required_size += right->length;

	memcpy(((uint8_t *) ret->bytecode) + required_size, 
			bottom->bytecode, bottom->length);
	required_size += bottom->length;

	memcpy(((uint8_t *) ret->bytecode) + required_size, 
			left->bytecode, left->length);
	required_size += left->length;

	assert(required_size == ret->length);

	*result = ret;
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (top)
		css_stylesheet_style_destroy(c->sheet, top, error == CSS_OK);
	if (right)
		css_stylesheet_style_destroy(c->sheet, right, error == CSS_OK);
	if (bottom)
		css_stylesheet_style_destroy(c->sheet, bottom, error == CSS_OK);
	if (left)
		css_stylesheet_style_destroy(c->sheet, left, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse border-bottom shorthand
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
css_error parse_border_bottom(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	return parse_border_side(c, vector, ctx, SIDE_BOTTOM, result);
}

/**
 * Parse border-bottom-color
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
css_error parse_border_bottom_color(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_color(c, vector, ctx, 
			CSS_PROP_BORDER_BOTTOM_COLOR, result);
}

/**
 * Parse border-bottom-style
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
css_error parse_border_bottom_style(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_style(c, vector, ctx, 
			CSS_PROP_BORDER_BOTTOM_STYLE, result);
}

/**
 * Parse border-bottom-width
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
css_error parse_border_bottom_width(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_width(c, vector, ctx, 
			CSS_PROP_BORDER_BOTTOM_WIDTH, result);
}

/**
 * Parse border-collapse
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
css_error parse_border_collapse(css_language *c, 
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

	/* IDENT (collapse, separate, inherit) */
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
			ident->idata, c->strings[COLLAPSE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_COLLAPSE_COLLAPSE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SEPARATE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_COLLAPSE_SEPARATE;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_BORDER_COLLAPSE, flags, value);

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
 * Parse border-color shorthand
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
css_error parse_border_color(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *top = NULL;
	css_style *right = NULL;
	css_style *bottom = NULL;
	css_style *left = NULL;
	css_style *ret = NULL;
	uint32_t num_sides = 0;
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
			4 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_COLOR,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_RIGHT_COLOR,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_BOTTOM_COLOR,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_LEFT_COLOR,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse up to 4 colours */
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

		if (top == NULL &&
				(error = parse_border_side_color(c, vector, 
				ctx, CSS_PROP_BORDER_TOP_COLOR, &top)) == 
				CSS_OK) {
			num_sides = 1;
		} else if (right == NULL &&
				(error = parse_border_side_color(c, vector, 
				ctx, CSS_PROP_BORDER_RIGHT_COLOR, &right)) == 
				CSS_OK) {
			num_sides = 2;
		} else if (bottom == NULL &&
				(error = parse_border_side_color(c, vector, 
				ctx, CSS_PROP_BORDER_BOTTOM_COLOR, &bottom)) == 
				CSS_OK) {
			num_sides = 3;
		} else if (left == NULL &&
				(error = parse_border_side_color(c, vector, 
				ctx, CSS_PROP_BORDER_LEFT_COLOR, &left)) == 
				CSS_OK) {
			num_sides = 4;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	if (num_sides == 0) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Calculate size of resultant style */
	if (num_sides == 1) {
		required_size = 4 * top->length;
	} else if (num_sides == 2) {
		required_size = 2 * top->length + 2 * right->length;
	} else if (num_sides == 3) {
		required_size = top->length + 2 * right->length + 
				bottom->length;
	} else {
		required_size = top->length + right->length +
				bottom->length + left->length;
	}

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (num_sides == 1) {
		uint32_t *opv = ((uint32_t *) top->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_RIGHT_COLOR, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_BOTTOM_COLOR, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_COLOR, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;
	} else if (num_sides == 2) {
		uint32_t *vopv = ((uint32_t *) top->bytecode);
		uint32_t *hopv = ((uint32_t *) right->bytecode);
		uint8_t vflags = getFlags(*vopv);
		uint8_t hflags = getFlags(*hopv);
		uint16_t vvalue = getValue(*vopv);
		uint16_t hvalue = getValue(*hopv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		*vopv = buildOPV(CSS_PROP_BORDER_BOTTOM_COLOR, vflags, vvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*hopv = buildOPV(CSS_PROP_BORDER_LEFT_COLOR, hflags, hvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else if (num_sides == 3) {
		uint32_t *opv = ((uint32_t *) right->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_COLOR, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				left->bytecode, left->length);
		required_size += left->length;
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (top)
		css_stylesheet_style_destroy(c->sheet, top, error == CSS_OK);
	if (right)
		css_stylesheet_style_destroy(c->sheet, right, error == CSS_OK);
	if (bottom)
		css_stylesheet_style_destroy(c->sheet, bottom, error == CSS_OK);
	if (left)
		css_stylesheet_style_destroy(c->sheet, left, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse border-left shorthand
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
css_error parse_border_left(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	return parse_border_side(c, vector, ctx, SIDE_LEFT, result);
}

/**
 * Parse border-left-color
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
css_error parse_border_left_color(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_color(c, vector, ctx, 
			CSS_PROP_BORDER_LEFT_COLOR, result);
}

/**
 * Parse border-left-style
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
css_error parse_border_left_style(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_style(c, vector, ctx, 
			CSS_PROP_BORDER_LEFT_STYLE, result);
}

/**
 * Parse border-left-width
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
css_error parse_border_left_width(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_width(c, vector, ctx, 
			CSS_PROP_BORDER_LEFT_WIDTH, result);
}

/**
 * Parse border-right shorthand
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
css_error parse_border_right(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	return parse_border_side(c, vector, ctx, SIDE_RIGHT, result);
}

/**
 * Parse border-right-color
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
css_error parse_border_right_color(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_color(c, vector, ctx, 
			CSS_PROP_BORDER_RIGHT_COLOR, result);
}

/**
 * Parse border-right-style
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
css_error parse_border_right_style(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_style(c, vector, ctx, 
			CSS_PROP_BORDER_RIGHT_STYLE, result);
}

/**
 * Parse border-right-width
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
css_error parse_border_right_width(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_width(c, vector, ctx, 
			CSS_PROP_BORDER_RIGHT_WIDTH, result);
}

/**
 * Parse border-spacing
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
css_error parse_border_spacing(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed length[2] = { 0 };
	uint32_t unit[2] = { 0 };
	uint32_t required_size;
	bool match;

	/* length length? | IDENT(inherit) */
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
	} else {
		int num_lengths = 0;

		error = parse_unit_specifier(c, vector, ctx, UNIT_PX,
				&length[0], &unit[0]);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if (unit[0] & UNIT_ANGLE || unit[0] & UNIT_TIME || 
				unit[0] & UNIT_FREQ || unit[0] & UNIT_PCT) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		num_lengths = 1;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL) {
			/* Attempt second length, ignoring errors.
			 * The core !important parser will ensure 
			 * any remaining junk is thrown out.
			 * Ctx will be preserved on error, as usual
			 */
			error = parse_unit_specifier(c, vector, ctx, UNIT_PX,
					&length[1], &unit[1]);
			if (error == CSS_OK) {
				if (unit[1] & UNIT_ANGLE || 
						unit[1] & UNIT_TIME ||
						unit[1] & UNIT_FREQ || 
						unit[1] & UNIT_PCT) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				num_lengths = 2;
			}
		}

		if (num_lengths == 1) {
			/* Only one length specified. Use for both axes. */
			length[1] = length[0];
			unit[1] = unit[0];
		}

		/* Lengths must not be negative */
		if (length[0] < 0 || length[1] < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = BORDER_SPACING_SET;
	}

	opv = buildOPV(CSS_PROP_BORDER_SPACING, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == BORDER_SPACING_SET)
		required_size += 2 * (sizeof(length[0]) + sizeof(unit[0]));

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == BORDER_SPACING_SET) {
		uint8_t *ptr = ((uint8_t *) (*result)->bytecode) + sizeof(opv);

		memcpy(ptr, &length[0], sizeof(length[0]));
		ptr += sizeof(length[0]);
		memcpy(ptr, &unit[0], sizeof(unit[0]));
		ptr += sizeof(unit[0]);
		memcpy(ptr, &length[1], sizeof(length[1]));
		ptr += sizeof(length[1]);
		memcpy(ptr, &unit[1], sizeof(unit[1]));
	}

	return CSS_OK;
}

/**
 * Parse border-style shorthand
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
css_error parse_border_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *top = NULL;
	css_style *right = NULL;
	css_style *bottom = NULL;
	css_style *left = NULL;
	css_style *ret = NULL;
	uint32_t num_sides = 0;
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
			4 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_STYLE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_RIGHT_STYLE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_BOTTOM_STYLE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_LEFT_STYLE,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse up to 4 styles */
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

		if (top == NULL &&
				(error = parse_border_side_style(c, vector, 
				ctx, CSS_PROP_BORDER_TOP_STYLE, &top)) == 
				CSS_OK) {
			num_sides = 1;
		} else if (right == NULL &&
				(error = parse_border_side_style(c, vector, 
				ctx, CSS_PROP_BORDER_RIGHT_STYLE, &right)) == 
				CSS_OK) {
			num_sides = 2;
		} else if (bottom == NULL &&
				(error = parse_border_side_style(c, vector, 
				ctx, CSS_PROP_BORDER_BOTTOM_STYLE, &bottom)) == 
				CSS_OK) {
			num_sides = 3;
		} else if (left == NULL &&
				(error = parse_border_side_style(c, vector, 
				ctx, CSS_PROP_BORDER_LEFT_STYLE, &left)) == 
				CSS_OK) {
			num_sides = 4;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	if (num_sides == 0) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Calculate size of resultant style */
	if (num_sides == 1) {
		required_size = 4 * top->length;
	} else if (num_sides == 2) {
		required_size = 2 * top->length + 2 * right->length;
	} else if (num_sides == 3) {
		required_size = top->length + 2 * right->length + 
				bottom->length;
	} else {
		required_size = top->length + right->length +
				bottom->length + left->length;
	}

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (num_sides == 1) {
		uint32_t *opv = ((uint32_t *) top->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_RIGHT_STYLE, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_BOTTOM_STYLE, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_STYLE, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;
	} else if (num_sides == 2) {
		uint32_t *vopv = ((uint32_t *) top->bytecode);
		uint32_t *hopv = ((uint32_t *) right->bytecode);
		uint8_t vflags = getFlags(*vopv);
		uint8_t hflags = getFlags(*hopv);
		uint16_t vvalue = getValue(*vopv);
		uint16_t hvalue = getValue(*hopv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		*vopv = buildOPV(CSS_PROP_BORDER_BOTTOM_STYLE, vflags, vvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*hopv = buildOPV(CSS_PROP_BORDER_LEFT_STYLE, hflags, hvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else if (num_sides == 3) {
		uint32_t *opv = ((uint32_t *) right->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_STYLE, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				left->bytecode, left->length);
		required_size += left->length;
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (top)
		css_stylesheet_style_destroy(c->sheet, top, error == CSS_OK);
	if (right)
		css_stylesheet_style_destroy(c->sheet, right, error == CSS_OK);
	if (bottom)
		css_stylesheet_style_destroy(c->sheet, bottom, error == CSS_OK);
	if (left)
		css_stylesheet_style_destroy(c->sheet, left, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse border-top shorthand
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
css_error parse_border_top(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	return parse_border_side(c, vector, ctx, SIDE_TOP, result);
}

/**
 * Parse border-top-color
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
css_error parse_border_top_color(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_color(c, vector, ctx, 
			CSS_PROP_BORDER_TOP_COLOR, result);
}

/**
 * Parse border-top-style
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
css_error parse_border_top_style(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_style(c, vector, ctx, 
			CSS_PROP_BORDER_TOP_STYLE, result);
}

/**
 * Parse border-top-width
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
css_error parse_border_top_width(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_border_side_width(c, vector, ctx, 
			CSS_PROP_BORDER_TOP_WIDTH, result);
}

/**
 * Parse border-width shorthand
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
css_error parse_border_width(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *top = NULL;
	css_style *right = NULL;
	css_style *bottom = NULL;
	css_style *left = NULL;
	css_style *ret = NULL;
	uint32_t num_sides = 0;
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
			4 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_WIDTH,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_RIGHT_WIDTH,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_BOTTOM_WIDTH,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_LEFT_WIDTH,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse up to 4 widths */
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

		if (top == NULL &&
				(error = parse_border_side_width(c, vector, 
				ctx, CSS_PROP_BORDER_TOP_WIDTH, &top)) == 
				CSS_OK) {
			num_sides = 1;
		} else if (right == NULL &&
				(error = parse_border_side_width(c, vector, 
				ctx, CSS_PROP_BORDER_RIGHT_WIDTH, &right)) == 
				CSS_OK) {
			num_sides = 2;
		} else if (bottom == NULL &&
				(error = parse_border_side_width(c, vector, 
				ctx, CSS_PROP_BORDER_BOTTOM_WIDTH, &bottom)) == 
				CSS_OK) {
			num_sides = 3;
		} else if (left == NULL &&
				(error = parse_border_side_width(c, vector, 
				ctx, CSS_PROP_BORDER_LEFT_WIDTH, &left)) == 
				CSS_OK) {
			num_sides = 4;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	if (num_sides == 0) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Calculate size of resultant style */
	if (num_sides == 1) {
		required_size = 4 * top->length;
	} else if (num_sides == 2) {
		required_size = 2 * top->length + 2 * right->length;
	} else if (num_sides == 3) {
		required_size = top->length + 2 * right->length + 
				bottom->length;
	} else {
		required_size = top->length + right->length +
				bottom->length + left->length;
	}

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (num_sides == 1) {
		uint32_t *opv = ((uint32_t *) top->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_RIGHT_WIDTH, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_BOTTOM_WIDTH, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_WIDTH, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;
	} else if (num_sides == 2) {
		uint32_t *vopv = ((uint32_t *) top->bytecode);
		uint32_t *hopv = ((uint32_t *) right->bytecode);
		uint8_t vflags = getFlags(*vopv);
		uint8_t hflags = getFlags(*hopv);
		uint16_t vvalue = getValue(*vopv);
		uint16_t hvalue = getValue(*hopv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		*vopv = buildOPV(CSS_PROP_BORDER_BOTTOM_WIDTH, vflags, vvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		*hopv = buildOPV(CSS_PROP_BORDER_LEFT_WIDTH, hflags, hvalue);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else if (num_sides == 3) {
		uint32_t *opv = ((uint32_t *) right->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		*opv = buildOPV(CSS_PROP_BORDER_LEFT_WIDTH, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;
	} else {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				top->bytecode, top->length);
		required_size += top->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				right->bytecode, right->length);
		required_size += right->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				bottom->bytecode, bottom->length);
		required_size += bottom->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				left->bytecode, left->length);
		required_size += left->length;
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (top)
		css_stylesheet_style_destroy(c->sheet, top, error == CSS_OK);
	if (right)
		css_stylesheet_style_destroy(c->sheet, right, error == CSS_OK);
	if (bottom)
		css_stylesheet_style_destroy(c->sheet, bottom, error == CSS_OK);
	if (left)
		css_stylesheet_style_destroy(c->sheet, left, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse outline shorthand
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
css_error parse_outline(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *color = NULL;
	css_style *style = NULL;
	css_style *width = NULL;
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

		*(bytecode++) = buildOPV(CSS_PROP_OUTLINE_COLOR,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_OUTLINE_STYLE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_OUTLINE_WIDTH,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse individual properties */
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

		if (color == NULL &&
				(error = parse_outline_color(c, vector, 
				ctx, &color)) == CSS_OK) {
		} else if (style == NULL &&
				(error = parse_outline_style(c, vector, 
				ctx, &style)) == CSS_OK) {
		} else if (width == NULL &&
				(error = parse_outline_width(c, vector, 
				ctx, &width)) == CSS_OK) {
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	/* Calculate size of resultant style */
	required_size = 0;
	if (color)
		required_size += color->length;
	else
		required_size += sizeof(uint32_t);

	if (style)
		required_size += style->length;
	else
		required_size += sizeof(uint32_t);

	if (width)
		required_size += width->length;
	else
		required_size += sizeof(uint32_t);

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (color) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				color->bytecode, color->length);
		required_size += color->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_OUTLINE_COLOR,
				0, OUTLINE_COLOR_INVERT);
		required_size += sizeof(uint32_t);
	}

	if (style) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				style->bytecode, style->length);
		required_size += style->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_OUTLINE_STYLE,
				0, OUTLINE_STYLE_NONE);
		required_size += sizeof(uint32_t);
	}

	if (width) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				width->bytecode, width->length);
		required_size += width->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_OUTLINE_WIDTH,
				0, OUTLINE_WIDTH_MEDIUM);
		required_size += sizeof(uint32_t);
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (color)
		css_stylesheet_style_destroy(c->sheet, color, error == CSS_OK);
	if (style)
		css_stylesheet_style_destroy(c->sheet, style, error == CSS_OK);
	if (width)
		css_stylesheet_style_destroy(c->sheet, width, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse outline-color
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
css_error parse_outline_color(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t colour = 0;
	uint32_t required_size;
	bool match;

	/* colour | IDENT (invert, inherit) */
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
		flags |= FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INVERT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = OUTLINE_COLOR_INVERT;
	} else {
		error = parse_colour_specifier(c, vector, ctx, &colour);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		value = OUTLINE_COLOR_SET;
	}

	opv = buildOPV(CSS_PROP_OUTLINE_COLOR, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == OUTLINE_COLOR_SET)
		required_size += sizeof(colour);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == OUTLINE_COLOR_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&colour, sizeof(colour));
	}

	return CSS_OK;
}

/**
 * Parse outline-style
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
css_error parse_outline_style(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	uint32_t opv;
	uint16_t value;

	/* Parse as a border style  */
	error = parse_border_side_style(c, vector, ctx, 
			CSS_PROP_OUTLINE_STYLE, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	opv = *((uint32_t *) (*result)->bytecode);

	value = getValue(opv);

	/* Hidden is invalid */
	if (value == BORDER_STYLE_HIDDEN) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	return CSS_OK;
}

/**
 * Parse outline-width
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
css_error parse_outline_width(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	/* Parse as border width */
	return parse_border_side_width(c, vector, ctx, 
			CSS_PROP_OUTLINE_WIDTH, result);
}

/**
 * Parse border-{top,right,bottom,left} shorthand
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param side	  The side we're parsing for
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_border_side(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint32_t side, css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *color = NULL;
	css_style *style = NULL;
	css_style *width = NULL;
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

		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_COLOR + side,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_STYLE + side,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BORDER_TOP_WIDTH + side,
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to parse individual properties */
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

		if (color == NULL &&
				(error = parse_border_side_color(c, vector, ctx,
				CSS_PROP_BORDER_TOP_COLOR + side, &color)) == 
				CSS_OK) {
		} else if (style == NULL &&
				(error = parse_border_side_style(c, vector, ctx,
				CSS_PROP_BORDER_TOP_STYLE + side, &style)) == 
				CSS_OK) {
		} else if (width == NULL &&
				(error = parse_border_side_width(c, vector, ctx,
				CSS_PROP_BORDER_TOP_WIDTH + side, &width)) == 
				CSS_OK) {
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	/* Calculate size of resultant style */
	required_size = 0;
	if (color)
		required_size += color->length;

	if (style)
		required_size += style->length;
	else
		required_size += sizeof(uint32_t);

	if (width)
		required_size += width->length;
	else
		required_size += sizeof(uint32_t);

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (color) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				color->bytecode, color->length);
		required_size += color->length;
	}

	if (style) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				style->bytecode, style->length);
		required_size += style->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BORDER_TOP_STYLE + side,
				0, BORDER_STYLE_NONE);
		required_size += sizeof(uint32_t);
	}

	if (width) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				width->bytecode, width->length);
		required_size += width->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BORDER_TOP_WIDTH + side,
				0, BORDER_WIDTH_MEDIUM);
		required_size += sizeof(uint32_t);
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (color)
		css_stylesheet_style_destroy(c->sheet, color, error == CSS_OK);
	if (style)
		css_stylesheet_style_destroy(c->sheet, style, error == CSS_OK);
	if (width)
		css_stylesheet_style_destroy(c->sheet, width, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse border-{top,right,bottom,left}-color
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param op	  Opcode to parse for (encodes side)
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_border_side_color(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint32_t opv;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t colour = 0;
	uint32_t required_size;
	bool match;

	/* colour | IDENT (transparent, inherit) */
	token= parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT && 
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		flags |= FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[TRANSPARENT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = BORDER_COLOR_TRANSPARENT;
	} else {
		error = parse_colour_specifier(c, vector, ctx, &colour);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		value = BORDER_COLOR_SET;
	}

	opv = buildOPV(op, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == BORDER_COLOR_SET)
		required_size += sizeof(colour);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == BORDER_COLOR_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&colour, sizeof(colour));
	}

	return CSS_OK;
}

/**
 * Parse border-{top,right,bottom,left}-style
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param op	  Opcode to parse for (encodes side)
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_border_side_style(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *ident;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	bool match;

	/* IDENT (none, hidden, dotted, dashed, solid, double, groove, 
	 * ridge, inset, outset, inherit) */
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
			ident->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_NONE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[HIDDEN],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_HIDDEN;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DOTTED],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_DOTTED;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DASHED],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_DASHED;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SOLID],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_SOLID;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[DOUBLE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_DOUBLE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[GROOVE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_GROOVE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[RIDGE],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_RIDGE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[INSET],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_INSET;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[OUTSET],
			&match) == lwc_error_ok && match)) {
		value = BORDER_STYLE_OUTSET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(op, flags, value);

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
 * Parse border-{top,right,bottom,left}-width
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param op	  Opcode to parse for (encodes side)
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_border_side_width(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint16_t op, css_style **result)
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

	/* length | IDENT(thin, medium, thick, inherit) */
	token= parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT && 
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		flags |= FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[THIN],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = BORDER_WIDTH_THIN;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[MEDIUM],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = BORDER_WIDTH_MEDIUM;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[THICK],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = BORDER_WIDTH_THICK;
	} else {
		error = parse_unit_specifier(c, vector, ctx, UNIT_PX,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if (unit == UNIT_PCT || unit & UNIT_ANGLE ||
				unit & UNIT_TIME || unit & UNIT_FREQ) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Length must be positive */
		if (length < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = BORDER_WIDTH_SET;
	}

	opv = buildOPV(op, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == BORDER_WIDTH_SET)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == BORDER_WIDTH_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

