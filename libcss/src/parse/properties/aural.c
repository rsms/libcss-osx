/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *		http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

static css_error parse_cue_common(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		uint16_t op, css_style **result);
static css_error parse_pause_common(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		uint16_t op, css_style **result);

/**
 * Parse azimuth
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
css_error parse_azimuth(css_language *c, 
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

	/* angle | [ IDENT(left-side, far-left, left, center-left, center, 
	 *		   center-right, right, far-right, right-side) || 
	 *	   IDENT(behind) 
	 *	 ] 
	 *	 | IDENT(leftwards, rightwards, inherit)
	 */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		flags = FLAG_INHERIT;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(token->idata, c->strings[LEFTWARDS],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = AZIMUTH_LEFTWARDS;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(token->idata, c->strings[RIGHTWARDS],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = AZIMUTH_RIGHTWARDS;
	} else if (token->type == CSS_TOKEN_IDENT) {
		parserutils_vector_iterate(vector, ctx);

		/* Now, we may have one of the other keywords or behind,
		 * potentially followed by behind or other keyword, 
		 * respectively */
		if ((lwc_string_caseless_isequal(
				token->idata, c->strings[LEFT_SIDE],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_LEFT_SIDE;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[FAR_LEFT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_FAR_LEFT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[LEFT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_LEFT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[CENTER_LEFT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_CENTER_LEFT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[CENTER],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_CENTER;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[CENTER_RIGHT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_CENTER_RIGHT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[RIGHT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_RIGHT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[FAR_RIGHT],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_FAR_RIGHT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[RIGHT_SIDE],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_RIGHT_SIDE;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[BEHIND],
				&match) == lwc_error_ok && match)) {
			value = AZIMUTH_BEHIND;
		} else {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		consumeWhitespace(vector, ctx);

		/* Get potential following token */
		token = parserutils_vector_peek(vector, *ctx);

		if (token != NULL && token->type == CSS_TOKEN_IDENT &&
				value == AZIMUTH_BEHIND) {
			parserutils_vector_iterate(vector, ctx);

			if ((lwc_string_caseless_isequal(
					token->idata, c->strings[LEFT_SIDE],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_LEFT_SIDE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[FAR_LEFT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_FAR_LEFT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[LEFT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_LEFT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[CENTER_LEFT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_CENTER_LEFT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[CENTER],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_CENTER;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[CENTER_RIGHT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_CENTER_RIGHT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[RIGHT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_RIGHT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[FAR_RIGHT],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_FAR_RIGHT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[RIGHT_SIDE],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_RIGHT_SIDE;
			} else {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (token != NULL && token->type == CSS_TOKEN_IDENT &&
				value != AZIMUTH_BEHIND) {
			parserutils_vector_iterate(vector, ctx);

			if ((lwc_string_caseless_isequal(
					token->idata, c->strings[BEHIND],
					&match) == lwc_error_ok && match)) {
				value |= AZIMUTH_BEHIND;
			} else {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if ((token == NULL || token->type != CSS_TOKEN_IDENT) &&
				value == AZIMUTH_BEHIND) {
			value |= AZIMUTH_CENTER;
		}
	} else {
		error = parse_unit_specifier(c, vector, ctx, UNIT_DEG, 
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if ((unit & UNIT_ANGLE) == false) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Valid angles lie between -360 and 360 degrees */
		if (unit == UNIT_DEG) {
			if (length < FMULI(F_360, -1) || length > F_360) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (unit == UNIT_GRAD) {
			if (length < FMULI(F_400, -1) || length > F_400) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (unit == UNIT_RAD) {
			if (length < FMULI(F_2PI, -1) || length > F_2PI) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		}

		value = AZIMUTH_ANGLE;
	}

	opv = buildOPV(CSS_PROP_AZIMUTH, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == AZIMUTH_ANGLE)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == AZIMUTH_ANGLE) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

/**
 * Parse cue shorthand
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
css_error parse_cue(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	const css_token *token;
	css_style *before = NULL;
	css_style *after = NULL;
	css_style *ret = NULL;
	int num_read = 0;
	int prev_ctx;
	uint32_t required_size;
	bool match;
	css_error error;

	/* Deal with inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		uint32_t *bytecode;

		error = css_stylesheet_style_create(c->sheet,
				2 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_CUE_BEFORE, FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_CUE_AFTER, FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to read 1 or 2 cues */
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

		if (before == NULL && (error = parse_cue_before(c, vector, ctx, 
				&before)) == CSS_OK) {
			num_read = 1;
		} else if (after == NULL &&
				(error = parse_cue_after(c, vector, ctx, 
				&after)) == CSS_OK) {
			num_read = 2;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	if (num_read == 0) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Calculate size of resultant style */
	if (num_read == 1)
		required_size = 2 * before->length;
	else
		required_size = before->length + after->length;

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (num_read == 1) {
		uint32_t *opv = ((uint32_t *) before->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;

		*opv = buildOPV(CSS_PROP_CUE_AFTER, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;
	} else {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				after->bytecode, after->length);
		required_size += after->length;
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (before)
		css_stylesheet_style_destroy(c->sheet, before, error == CSS_OK);
	if (after)
		css_stylesheet_style_destroy(c->sheet, after, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse cue-after
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
css_error parse_cue_after(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_cue_common(c, vector, ctx, CSS_PROP_CUE_AFTER, result);
}

/**
 * Parse cue-before
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
css_error parse_cue_before(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_cue_common(c, vector, ctx, CSS_PROP_CUE_BEFORE, result);
}

/**
 * Parse elevation
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
css_error parse_elevation(css_language *c, 
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

	/* angle | IDENT(below, level, above, higher, lower, inherit) */
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
			token->idata, c->strings[BELOW],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = ELEVATION_BELOW;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[LEVEL],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = ELEVATION_LEVEL;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[ABOVE],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = ELEVATION_ABOVE;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[HIGHER],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = ELEVATION_HIGHER;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[LOWER],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = ELEVATION_LOWER;
	} else {
		error = parse_unit_specifier(c, vector, ctx, UNIT_DEG,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if ((unit & UNIT_ANGLE) == false) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Valid angles lie between -90 and 90 degrees */
		if (unit == UNIT_DEG) {
			if (length < FMULI(F_90, -1) || length > F_90) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (unit == UNIT_GRAD) {
			if (length < FMULI(F_100, -1) || length > F_100) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else if (unit == UNIT_RAD) {
			if (length < FMULI(F_PI_2, -1) || length > F_PI_2) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		}

		value = ELEVATION_ANGLE;
	}

	opv = buildOPV(CSS_PROP_ELEVATION, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == ELEVATION_ANGLE)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == ELEVATION_ANGLE) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

/**
 * Parse pause shorthand
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
css_error parse_pause(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	const css_token *token;
	css_style *before = NULL;
	css_style *after = NULL;
	css_style *ret = NULL;
	int num_read = 0;
	int prev_ctx;
	uint32_t required_size;
	bool match;
	css_error error;

	/* Deal with inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token != NULL && token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		uint32_t *bytecode;

		error = css_stylesheet_style_create(c->sheet,
				2 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_PAUSE_BEFORE, 
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_PAUSE_AFTER, 
				FLAG_INHERIT, 0);

		parserutils_vector_iterate(vector, ctx);

		*result = ret;

		return CSS_OK;
	} else if (token == NULL) {
		/* No tokens -- clearly garbage */
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	/* Attempt to read 1 or 2 pauses */
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

		if (before == NULL && (error = parse_pause_before(c, vector, 
				ctx, &before)) == CSS_OK) {
			num_read = 1;
		} else if (after == NULL && 
				(error = parse_pause_after(c, vector, ctx, 
				&after)) == CSS_OK) {
			num_read = 2;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);

	if (num_read == 0) {
		error = CSS_INVALID;
		goto cleanup;
	}

	/* Calculate size of resultant style */
	if (num_read == 1)
		required_size = 2 * before->length;
	else
		required_size = before->length + after->length;

	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (num_read == 1) {
		uint32_t *opv = ((uint32_t *) before->bytecode);
		uint8_t flags = getFlags(*opv);
		uint16_t value = getValue(*opv);

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;

		*opv = buildOPV(CSS_PROP_PAUSE_AFTER, flags, value);
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;
	} else {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				before->bytecode, before->length);
		required_size += before->length;

		memcpy(((uint8_t *) ret->bytecode) + required_size,
				after->bytecode, after->length);
		required_size += after->length;
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (before)
		css_stylesheet_style_destroy(c->sheet, before, error == CSS_OK);
	if (after)
		css_stylesheet_style_destroy(c->sheet, after, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse pause-after
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
css_error parse_pause_after(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_pause_common(c, vector, ctx, CSS_PROP_PAUSE_AFTER, result);
}

/**
 * Parse pause-before
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
css_error parse_pause_before(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	return parse_pause_common(c, vector, ctx, 
			CSS_PROP_PAUSE_BEFORE, result);
}

/**
 * Parse pitch-range
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
css_error parse_pitch_range(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed num = 0;
	uint32_t required_size;
	bool match;

	/* number | IDENT (inherit) */
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
		num = number_from_lwc_string(token->idata, false, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Must be between 0 and 100 */
		if (num < 0 || num > F_100) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = PITCH_RANGE_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_PITCH_RANGE, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == PITCH_RANGE_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == PITCH_RANGE_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

/**
 * Parse pitch
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
css_error parse_pitch(css_language *c, 
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

	/* frequency | IDENT(x-low, low, medium, high, x-high, inherit) */
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
			token->idata, c->strings[X_LOW],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = PITCH_X_LOW;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[LOW],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = PITCH_LOW;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[MEDIUM],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = PITCH_MEDIUM;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[HIGH],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = PITCH_HIGH;
	} else if (token->type == CSS_TOKEN_IDENT &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[X_HIGH],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = PITCH_X_HIGH;
	} else {
		error = parse_unit_specifier(c, vector, ctx, UNIT_HZ,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if ((unit & UNIT_FREQ) == false) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Negative values are invalid */
		if (length < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = PITCH_FREQUENCY;
	}

	opv = buildOPV(CSS_PROP_PITCH, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == PITCH_FREQUENCY)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == PITCH_FREQUENCY) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

/**
 * Parse play-during
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
css_error parse_play_during(css_language *c, 
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
	lwc_string *uri;
	bool match;

	/* URI [ IDENT(mix) || IDENT(repeat) ]? | IDENT(auto,none,inherit) */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_URI)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT) {
		if ((lwc_string_caseless_isequal(
				token->idata, c->strings[INHERIT],
				&match) == lwc_error_ok && match)) {
			flags |= FLAG_INHERIT;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[NONE],
				&match) == lwc_error_ok && match)) {
			value = PLAY_DURING_NONE;
		} else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[AUTO],
				&match) == lwc_error_ok && match)) {
			value = PLAY_DURING_AUTO;
		} else {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}
	} else {
		int modifiers;

		value = PLAY_DURING_URI;

		error = c->sheet->resolve(c->sheet->resolve_pw,
				c->sheet->url,
				token->idata, &uri);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		for (modifiers = 0; modifiers < 2; modifiers++) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
			if (token != NULL && token->type == CSS_TOKEN_IDENT) {
				if ((lwc_string_caseless_isequal(
						token->idata, c->strings[MIX],
						&match) == lwc_error_ok && 
						match)) {
					if ((value & PLAY_DURING_MIX) == 0)
						value |= PLAY_DURING_MIX;
					else {
						*ctx = orig_ctx;
						return CSS_INVALID;
					}
				} else if (lwc_string_caseless_isequal(
						token->idata, 
						c->strings[REPEAT],
						&match) == lwc_error_ok &&
						match) {
					if ((value & PLAY_DURING_REPEAT) == 0)
						value |= PLAY_DURING_REPEAT;
					else {
						*ctx = orig_ctx;
						return CSS_INVALID;
					}
				} else {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				parserutils_vector_iterate(vector, ctx);
			}
		}
	}

	opv = buildOPV(CSS_PROP_PLAY_DURING, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && 
			(value & PLAY_DURING_TYPE_MASK) == PLAY_DURING_URI)
		required_size += sizeof(lwc_string *);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && 
			(value & PLAY_DURING_TYPE_MASK)	 == PLAY_DURING_URI) {
		/* Don't ref URI -- we want to pass ownership to the bytecode */
		memcpy((uint8_t *) (*result)->bytecode + sizeof(opv),
				&uri, sizeof(lwc_string *));
	}

	return CSS_OK;
}

/**
 * Parse richness
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
css_error parse_richness(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed num = 0;
	uint32_t required_size;
	bool match;

	/* number | IDENT (inherit) */
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
		num = number_from_lwc_string(token->idata, false, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Must be between 0 and 100 */
		if (num < 0 || num > F_100) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = RICHNESS_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_RICHNESS, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == RICHNESS_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == RICHNESS_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

/**
 * Parse speak-header
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
css_error parse_speak_header(css_language *c, 
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

	/* IDENT (once, always, inherit) */
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
			ident->idata, c->strings[ONCE],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_HEADER_ONCE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[ALWAYS],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_HEADER_ALWAYS;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_SPEAK_HEADER, flags, value);

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
 * Parse speak-numeral
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
css_error parse_speak_numeral(css_language *c, 
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

	/* IDENT (digits, continuous, inherit) */
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
			ident->idata, c->strings[DIGITS],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_NUMERAL_DIGITS;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[CONTINUOUS],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_NUMERAL_CONTINUOUS;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_SPEAK_NUMERAL, flags, value);

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
 * Parse speak-punctuation
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
css_error parse_speak_punctuation(css_language *c, 
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

	/* IDENT (code, none, inherit) */
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
			ident->idata, c->strings[CODE],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_PUNCTUATION_CODE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_PUNCTUATION_NONE;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_SPEAK_PUNCTUATION, flags, value);

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
 * Parse speak
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
css_error parse_speak(css_language *c, 
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

	/* IDENT (normal, none, spell-out, inherit) */
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
		value = SPEAK_NORMAL;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_NONE;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SPELL_OUT],
			&match) == lwc_error_ok && match)) {
		value = SPEAK_SPELL_OUT;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_SPEAK, flags, value);

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
 * Parse speech-rate
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
css_error parse_speech_rate(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed num = 0;
	uint32_t required_size;
	bool match;

	/* number | IDENT (x-slow, slow, medium, fast, x-fast, faster, slower, 
	 *		 inherit)
	 */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_NUMBER)) {
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
			token->idata, c->strings[X_SLOW],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_X_SLOW;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[SLOW],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_SLOW;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[MEDIUM],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_MEDIUM;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[FAST],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_FAST;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[X_FAST],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_X_FAST;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[FASTER],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_FASTER;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[SLOWER],
			&match) == lwc_error_ok && match)) {
		value = SPEECH_RATE_SLOWER;
	} else if (token->type == CSS_TOKEN_NUMBER) {
		size_t consumed = 0;
		num = number_from_lwc_string(token->idata, false, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Make negative values invalid */
		if (num < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = SPEECH_RATE_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_SPEECH_RATE, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == SPEECH_RATE_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == SPEECH_RATE_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

/**
 * Parse stress
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
css_error parse_stress(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	css_fixed num = 0;
	uint32_t required_size;
	bool match;

	/* number | IDENT (inherit) */
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
		num = number_from_lwc_string(token->idata, false, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		if (num < 0 || num > INTTOFIX(100)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = STRESS_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_STRESS, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == STRESS_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == STRESS_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

/**
 * Determine if a given voice-family ident is reserved
 *
 * \param c	 Parsing context
 * \param ident	 IDENT to consider
 * \return True if IDENT is reserved, false otherwise
 */
static bool voice_family_reserved(css_language *c, const css_token *ident)
{
	bool match;

	return (lwc_string_caseless_isequal(
			ident->idata, c->strings[MALE],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[FEMALE],
			&match) == lwc_error_ok && match) ||
		(lwc_string_caseless_isequal(
			ident->idata, c->strings[CHILD],
			&match) == lwc_error_ok && match);
}

/**
 * Convert a voice-family token into a bytecode value
 *
 * \param c	 Parsing context
 * \param token	 Token to consider
 * \return Bytecode value
 */
static uint16_t voice_family_value(css_language *c, const css_token *token)
{
	uint16_t value;
	bool match;

	if (token->type == CSS_TOKEN_IDENT) {
		if ((lwc_string_caseless_isequal(
				token->idata, c->strings[MALE],
				&match) == lwc_error_ok && match))
			value = VOICE_FAMILY_MALE;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[FEMALE],
				&match) == lwc_error_ok && match))
			value = VOICE_FAMILY_FEMALE;
		else if ((lwc_string_caseless_isequal(
				token->idata, c->strings[CHILD],
				&match) == lwc_error_ok && match))
			value = VOICE_FAMILY_CHILD;
		else
			value = VOICE_FAMILY_IDENT_LIST;
	} else {
		value = VOICE_FAMILY_STRING;
	}

	return value;
}

/**
 * Parse voice-family
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	 CSS_NOMEM on memory exhaustion,
 *	 CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_voice_family(css_language *c, 
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

		value = voice_family_value(c, token);

		error = comma_list_length(c, vector, &temp_ctx, 
				token, voice_family_reserved, &list_size);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		required_size += list_size;
	}

	opv = buildOPV(CSS_PROP_VOICE_FAMILY, flags, value);

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
		css_stylesheet_style_destroy(c->sheet, *result, true);
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
				voice_family_reserved, voice_family_value,
				&ptr);
		if (error != CSS_OK) {
			css_stylesheet_style_destroy(c->sheet, *result, true);
			*result = NULL;
			*ctx = orig_ctx;
			return error;
		}

		/* Write terminator */
		opv = VOICE_FAMILY_END;
		memcpy(ptr, &opv, sizeof(opv));
		ptr += sizeof(opv);
	}

	return CSS_OK;
}

/**
 * Parse volume
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	 CSS_NOMEM on memory exhaustion,
 *	 CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_volume(css_language *c, 
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

	/* number | percentage | IDENT(silent, x-soft, soft, medium, loud, 
	 *			       x-loud, inherit)
	 */
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
			token->idata, c->strings[SILENT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_SILENT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[X_SOFT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_X_SOFT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[SOFT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_SOFT;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[MEDIUM],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_MEDIUM;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[LOUD],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_LOUD;
	} else if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[X_LOUD],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_X_LOUD;
	} else if (token->type == CSS_TOKEN_NUMBER) {
		size_t consumed = 0;
		length = number_from_lwc_string(token->idata, false, &consumed);
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Must be between 0 and 100 */
		if (length < 0 || length > F_100) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		parserutils_vector_iterate(vector, ctx);
		value = VOLUME_NUMBER;
	} else {
		/* Yes, really UNIT_PX -- percentages MUST have a % sign */
		error = parse_unit_specifier(c, vector, ctx, UNIT_PX,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if ((unit & UNIT_PCT) == false) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Must be positive */
		if (length < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = VOLUME_DIMENSION;
	}

	opv = buildOPV(CSS_PROP_VOLUME, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == VOLUME_NUMBER)
		required_size += sizeof(length);
	else if ((flags & FLAG_INHERIT) == false && value == VOLUME_DIMENSION)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && (value == VOLUME_NUMBER || 
			value == VOLUME_DIMENSION))
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
	if ((flags & FLAG_INHERIT) == false && value == VOLUME_DIMENSION)
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));

	return CSS_OK;
}

/**
 * Common parser for cue-after and cue-before
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
css_error parse_cue_common(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		uint16_t op, css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint16_t value = 0;
	uint32_t opv;
	uint32_t required_size;
	lwc_string *uri = NULL;
	bool match;

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
		value = CUE_AFTER_NONE;
	} else if (token->type == CSS_TOKEN_URI) {
		value = CUE_AFTER_URI;

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

	opv = buildOPV(op, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == CUE_AFTER_URI)
		required_size += sizeof(lwc_string *);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == CUE_AFTER_URI) {
		/* Don't ref URI -- we want to pass ownership to the bytecode */
		memcpy((uint8_t *) (*result)->bytecode + sizeof(opv),
				&uri, sizeof(lwc_string *));
	}
	
	return CSS_OK;
}

/**
 * Common parser for pause-after and pause-before
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
css_error parse_pause_common(css_language *c, 
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

	/* time | percentage | IDENT(inherit) */
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
		error = parse_unit_specifier(c, vector, ctx, UNIT_S,
				&length, &unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		if ((unit & UNIT_TIME) == false && (unit & UNIT_PCT) == false) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Negative values are illegal */
		if (length < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = PAUSE_AFTER_SET;
	}

	opv = buildOPV(op, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == PAUSE_AFTER_SET)
		required_size += sizeof(length) + sizeof(unit);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == PAUSE_AFTER_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&length, sizeof(length));
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv) +
				sizeof(length), &unit, sizeof(unit));
	}

	return CSS_OK;
}

