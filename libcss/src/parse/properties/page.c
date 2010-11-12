/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

/**
 * Parse orphans
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_orphans(css_language *c, 
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

	/* <integer> | IDENT (inherit) */
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
		num = number_from_lwc_string(token->idata, true, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Negative values are nonsensical */
		if (num < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = ORPHANS_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_ORPHANS, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == ORPHANS_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == ORPHANS_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

/**
 * Parse page-break-after
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_page_break_after(css_language *c, 
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

	/* IDENT (auto, always, avoid, left, right, inherit) */
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
			ident->idata, c->strings[AUTO],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_AFTER_AUTO;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[ALWAYS],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_AFTER_ALWAYS;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[AVOID],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_AFTER_AVOID;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LEFT],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_AFTER_LEFT;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[RIGHT],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_AFTER_RIGHT;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_PAGE_BREAK_AFTER, flags, value);

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
 * Parse page-break-before
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_page_break_before(css_language *c, 
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

	/* IDENT (auto, always, avoid, left, right, inherit) */
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
			ident->idata, c->strings[AUTO],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_BEFORE_AUTO;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[ALWAYS],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_BEFORE_ALWAYS;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[AVOID],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_BEFORE_AVOID;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[LEFT],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_BEFORE_LEFT;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[RIGHT],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_BEFORE_RIGHT;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_PAGE_BREAK_BEFORE, flags, value);

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
 * Parse page-break-inside
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_page_break_inside(css_language *c, 
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

	/* IDENT (auto, avoid, inherit) */
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
			ident->idata, c->strings[AUTO],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_INSIDE_AUTO;
	} else if ((lwc_string_caseless_isequal(
			ident->idata, c->strings[AVOID],
			&match) == lwc_error_ok && match)) {
		value = PAGE_BREAK_INSIDE_AVOID;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_PAGE_BREAK_INSIDE, flags, value);

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
 * Parse widows
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion,
 *         CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_widows(css_language *c, 
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

	/* <integer> | IDENT (inherit) */
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
		num = number_from_lwc_string(token->idata, true, &consumed);
		/* Invalid if there are trailing characters */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		/* Negative values are nonsensical */
		if (num < 0) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		value = WIDOWS_SET;
	} else {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	opv = buildOPV(CSS_PROP_WIDOWS, flags, value);

	required_size = sizeof(opv);
	if ((flags & FLAG_INHERIT) == false && value == WIDOWS_SET)
		required_size += sizeof(num);

	/* Allocate result */
	error = css_stylesheet_style_create(c->sheet, required_size, result);
	if (error != CSS_OK) {
		*ctx = orig_ctx;
		return error;
	}

	/* Copy the bytecode to it */
	memcpy((*result)->bytecode, &opv, sizeof(opv));
	if ((flags & FLAG_INHERIT) == false && value == WIDOWS_SET) {
		memcpy(((uint8_t *) (*result)->bytecode) + sizeof(opv), 
				&num, sizeof(num));
	}

	return CSS_OK;
}

