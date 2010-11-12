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
 * Parse background
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
css_error parse_background(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style **result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_style *attachment = NULL;
	css_style *color = NULL;
	css_style *image = NULL;
	css_style *position = NULL;
	css_style *repeat = NULL;
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
				5 * sizeof(uint32_t), &ret);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		bytecode = (uint32_t *) ret->bytecode;

		*(bytecode++) = buildOPV(CSS_PROP_BACKGROUND_ATTACHMENT, 
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BACKGROUND_COLOR,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BACKGROUND_IMAGE,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BACKGROUND_POSITION,
				FLAG_INHERIT, 0);
		*(bytecode++) = buildOPV(CSS_PROP_BACKGROUND_REPEAT,
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

		/* Try each property parser in turn, but only if we
		 * haven't already got a value for this property.
		 * To achieve this, we end up with a bunch of empty
		 * if/else statements. Perhaps there's a clearer way 
		 * of expressing this. */
		if (attachment == NULL && 
				(error = parse_background_attachment(c, vector,
				ctx, &attachment)) == CSS_OK) {
		} else if (color == NULL && 
				(error = parse_background_color(c, vector, ctx,
				&color)) == CSS_OK) {
		} else if (image == NULL && 
				(error = parse_background_image(c, vector, ctx,
				&image)) == CSS_OK) {
		} else if (position == NULL &&
				(error = parse_background_position(c, vector, 
				ctx, &position)) == CSS_OK) {
		} else if (repeat == NULL &&
				(error = parse_background_repeat(c, vector, 
				ctx, &repeat)) == CSS_OK) {
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

	if (attachment)
		required_size += attachment->length;
	else
		required_size += sizeof(uint32_t);

	if (color)
		required_size += color->length;
	else
		required_size += sizeof(uint32_t);

	if (image)
		required_size += image->length;
	else
		required_size += sizeof(uint32_t);

	if (position)
		required_size += position->length;
	else
		required_size += sizeof(uint32_t); /* Use top left, not 0% 0% */

	if (repeat)
		required_size += repeat->length;
	else
		required_size += sizeof(uint32_t);

	/* Create and populate it */
	error = css_stylesheet_style_create(c->sheet, required_size, &ret);
	if (error != CSS_OK)
		goto cleanup;

	required_size = 0;

	if (attachment) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				attachment->bytecode, attachment->length);
		required_size += attachment->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BACKGROUND_ATTACHMENT,
				0, BACKGROUND_ATTACHMENT_SCROLL);
		required_size += sizeof(uint32_t);
	}

	if (color) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				color->bytecode, color->length);
		required_size += color->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BACKGROUND_COLOR,
				0, BACKGROUND_COLOR_TRANSPARENT);
		required_size += sizeof(uint32_t);
	}

	if (image) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				image->bytecode, image->length);
		required_size += image->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BACKGROUND_IMAGE,
				0, BACKGROUND_IMAGE_NONE);
		required_size += sizeof(uint32_t);
	}

	if (position) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				position->bytecode, position->length);
		required_size += position->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BACKGROUND_POSITION,
				0, BACKGROUND_POSITION_HORZ_LEFT |
				BACKGROUND_POSITION_VERT_TOP);
		required_size += sizeof(uint32_t);
	}

	if (repeat) {
		memcpy(((uint8_t *) ret->bytecode) + required_size,
				repeat->bytecode, repeat->length);
		required_size += repeat->length;
	} else {
		void *bc = ((uint8_t *) ret->bytecode) + required_size;

		*((uint32_t *) bc) = buildOPV(CSS_PROP_BACKGROUND_REPEAT,
				0, BACKGROUND_REPEAT_REPEAT);
		required_size += sizeof(uint32_t);
	}

	assert(required_size == ret->length);

	/* Write the result */
	*result = ret;
	/* Invalidate ret, so that cleanup doesn't destroy it */
	ret = NULL;

	/* Clean up after ourselves */
cleanup:
	if (attachment)
		css_stylesheet_style_destroy(c->sheet, attachment, error == CSS_OK);
	if (color)
		css_stylesheet_style_destroy(c->sheet, color, error == CSS_OK);
	if (image)
		css_stylesheet_style_destroy(c->sheet, image, error == CSS_OK);
	if (position)
		css_stylesheet_style_destroy(c->sheet, position, error == CSS_OK);
	if (repeat)
		css_stylesheet_style_destroy(c->sheet, repeat, error == CSS_OK);
	if (ret)
		css_stylesheet_style_destroy(c->sheet, ret, error == CSS_OK);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

/**
 * Parse background-attachment
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
css_error parse_background_attachment(css_language *c, 
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

	/* IDENT (fixed, scroll, inherit) */
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
			ident->idata, c->strings[FIXED],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_ATTACHMENT_FIXED;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[SCROLL],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_ATTACHMENT_SCROLL;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_BACKGROUND_ATTACHMENT, flags, value);

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
 * Parse background-color
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
css_error parse_background_color(css_language *c, 
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

	/* colour | IDENT (transparent, inherit) */
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
			token->idata, c->strings[TRANSPARENT],
			&match) == lwc_error_ok && match)) {
		parserutils_vector_iterate(vector, ctx);
		value = BACKGROUND_COLOR_TRANSPARENT;
	} else {
		error = parse_colour_specifier(c, vector, ctx, &colour);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		value = BACKGROUND_COLOR_SET;
	}

	opv = buildOPV(CSS_PROP_BACKGROUND_COLOR, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == BACKGROUND_COLOR_SET)
		required_size += sizeof(colour);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == BACKGROUND_COLOR_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv),
				&colour, sizeof(colour));
	}

	return CSS_OK;
}

/**
 * Parse background-image
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
css_error parse_background_image(css_language *c, 
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
		value = BACKGROUND_IMAGE_NONE;
	} else if (token->type == CSS_TOKEN_URI) {
		value = BACKGROUND_IMAGE_URI;

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

	opv = buildOPV(CSS_PROP_BACKGROUND_IMAGE, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == BACKGROUND_IMAGE_URI)
		required_size += sizeof(lwc_string *);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == BACKGROUND_IMAGE_URI) {
		/* Don't ref URI -- we want to pass ownership to the bytecode */
		memcpy((uint8_t *) (*result)->bytecode + sizeof(opv),
				&uri, sizeof(lwc_string *));
	}

	return CSS_OK;
}

/**
 * Parse background-position
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
css_error parse_background_position(css_language *c, 
		const parserutils_vector *vector, int *ctx, 
		css_style **result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
	uint8_t flags = 0;
	uint32_t opv;
	uint16_t value[2] = { 0 };
	css_fixed length[2] = { 0 };
	uint32_t unit[2] = { 0 };
	uint32_t required_size;
	bool match;

	/* [length | percentage | IDENT(left, right, top, bottom, center)]{1,2}
	 * | IDENT(inherit) */
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
		int i;

		for (i = 0; i < 2; i++) {
			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL)
				break;

			if (token->type == CSS_TOKEN_IDENT) {
				if ((lwc_string_caseless_isequal(
						token->idata, c->strings[LEFT],
						&match) == lwc_error_ok && 
						match)) {
					value[i] = 
						BACKGROUND_POSITION_HORZ_LEFT;
				} else if ((lwc_string_caseless_isequal(
						token->idata, c->strings[RIGHT],
						&match) == lwc_error_ok && 
						match)) {
					value[i] = 
						BACKGROUND_POSITION_HORZ_RIGHT;
				} else if ((lwc_string_caseless_isequal(
						token->idata, c->strings[TOP],
						&match) == lwc_error_ok && 
						match)) {
					value[i] = BACKGROUND_POSITION_VERT_TOP;
				} else if ((lwc_string_caseless_isequal(
						token->idata, 
						c->strings[BOTTOM],
						&match) == lwc_error_ok && 
						match)) {
					value[i] = 
						BACKGROUND_POSITION_VERT_BOTTOM;
				} else if ((lwc_string_caseless_isequal(
						token->idata, 
						c->strings[CENTER],
						&match) == lwc_error_ok && 
						match)) {
					/* We'll fix this up later */
					value[i] = 
						BACKGROUND_POSITION_VERT_CENTER;
				} else if (i == 1) {
					/* Second pass, so ignore this one */
					break;
				} else {
					/* First pass, so invalid */
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				parserutils_vector_iterate(vector, ctx);
			} else if (token->type == CSS_TOKEN_DIMENSION ||
					token->type == CSS_TOKEN_NUMBER ||
					token->type == CSS_TOKEN_PERCENTAGE) {
				error = parse_unit_specifier(c, vector, ctx, 
						UNIT_PX, &length[i], &unit[i]);
				if (error != CSS_OK) {
					*ctx = orig_ctx;
					return error;
				}

				if (unit[i] & UNIT_ANGLE || 
						unit[i] & UNIT_TIME || 
						unit[i] & UNIT_FREQ) {
					*ctx = orig_ctx;
					return CSS_INVALID;
				}

				/* We'll fix this up later, too */
				value[i] = BACKGROUND_POSITION_VERT_SET;
			} else {
				if (i == 1) {
					/* Second pass, so ignore */
					break;
				} else {
					/* First pass, so invalid */
					*ctx = orig_ctx;
					return CSS_INVALID;
				}
			}

			consumeWhitespace(vector, ctx);
		}

		assert(i != 0);

		/* Now, sort out the mess we've got */
		if (i == 1) {
			assert(BACKGROUND_POSITION_VERT_CENTER ==
					BACKGROUND_POSITION_HORZ_CENTER);

			/* Only one value, so the other is center */
			switch (value[0]) {
			case BACKGROUND_POSITION_HORZ_LEFT:
			case BACKGROUND_POSITION_HORZ_RIGHT:
			case BACKGROUND_POSITION_VERT_CENTER:
			case BACKGROUND_POSITION_VERT_TOP:
			case BACKGROUND_POSITION_VERT_BOTTOM:
				break;
			case BACKGROUND_POSITION_VERT_SET:
				value[0] = BACKGROUND_POSITION_HORZ_SET;
				break;
			}

			value[1] = BACKGROUND_POSITION_VERT_CENTER;
		} else if (value[0] != BACKGROUND_POSITION_VERT_SET &&
				value[1] != BACKGROUND_POSITION_VERT_SET) {
			/* Two keywords. Verify the axes differ */
			if (((value[0] & 0xf) != 0 && (value[1] & 0xf) != 0) ||
					((value[0] & 0xf0) != 0 && 
						(value[1] & 0xf0) != 0)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else {
			/* One or two non-keywords. First is horizontal */
			if (value[0] == BACKGROUND_POSITION_VERT_SET)
				value[0] = BACKGROUND_POSITION_HORZ_SET;

			/* Verify the axes differ */
			if (((value[0] & 0xf) != 0 && (value[1] & 0xf) != 0) ||
					((value[0] & 0xf0) != 0 && 
						(value[1] & 0xf0) != 0)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		}
	}

	opv = buildOPV(CSS_PROP_BACKGROUND_POSITION, flags, 
			value[0] | value[1]);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false) { 
		if (value[0] == BACKGROUND_POSITION_HORZ_SET)
			required_size += sizeof(length[0]) + sizeof(unit[0]);
		if (value[1] == BACKGROUND_POSITION_VERT_SET)
			required_size += sizeof(length[1]) + sizeof(unit[1]);
	}

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false) {
		uint8_t *ptr = ((uint8_t *) (*result)->bytecode) + sizeof(opv);
		if (value[0] == BACKGROUND_POSITION_HORZ_SET) {
			memcpy(ptr, &length[0], sizeof(length[0]));
			ptr += sizeof(length[0]);
			memcpy(ptr, &unit[0], sizeof(unit[0]));
			ptr += sizeof(unit[0]);
		}
		if (value[1] == BACKGROUND_POSITION_VERT_SET) {
			memcpy(ptr, &length[1], sizeof(length[1]));
			ptr += sizeof(length[1]);
			memcpy(ptr, &unit[1], sizeof(unit[1]));
		}
	}

	return CSS_OK;
}

/**
 * Parse background-repeat
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
css_error parse_background_repeat(css_language *c, 
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

	/* IDENT (no-repeat, repeat-x, repeat-y, repeat, inherit) */
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
			ident->idata, c->strings[NO_REPEAT],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_REPEAT_NO_REPEAT;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[REPEAT_X],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_REPEAT_REPEAT_X;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[REPEAT_Y],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_REPEAT_REPEAT_Y;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[REPEAT],
			&match) == lwc_error_ok && match)) {
		value = BACKGROUND_REPEAT_REPEAT;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_BACKGROUND_REPEAT, flags, value);

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

