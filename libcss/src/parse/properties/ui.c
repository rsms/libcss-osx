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
 * Parse cursor
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
css_error parse_cursor(css_language *c, 
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

	/* [ (URI ',')* IDENT(auto, crosshair, default, pointer, move, e-resize,
	 *              ne-resize, nw-resize, n-resize, se-resize, sw-resize,
	 *              s-resize, w-resize, text, wait, help, progress) ] 
	 * | IDENT(inherit) 
	 */

	/* Pass 1: validate input and calculate bytecode size */
	token = parserutils_vector_iterate(vector, &temp_ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_URI)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		flags = FLAG_INHERIT;
	} else {
		bool first = true;

		/* URI* */
		while (token != NULL && token->type == CSS_TOKEN_URI) {
			lwc_string *uri = token->idata;

			if (first == false) {
				required_size += sizeof(opv);
			} else {
				value = CURSOR_URI;
			}
			required_size += sizeof(uri);

			consumeWhitespace(vector, &temp_ctx);

			/* Expect ',' */
			token = parserutils_vector_iterate(vector, &temp_ctx);
			if (token == NULL || tokenIsChar(token, ',') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			consumeWhitespace(vector, &temp_ctx);

			/* Expect either URI or IDENT */
			token = parserutils_vector_iterate(vector, &temp_ctx);
			if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_URI)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			first = false;
		}

		/* IDENT */
		if (token != NULL && token->type == CSS_TOKEN_IDENT) {
			if ((lwc_string_caseless_isequal(
					token->idata, c->strings[AUTO],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_AUTO;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[CROSSHAIR],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_CROSSHAIR;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[DEFAULT],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_DEFAULT;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[POINTER],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_POINTER;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[MOVE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_MOVE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[E_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_E_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[NE_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_NE_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[NW_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_NW_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[N_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_N_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[SE_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_SE_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[SW_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_SW_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[S_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_S_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[W_RESIZE],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_W_RESIZE;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[TEXT],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_TEXT;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[WAIT],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_WAIT;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[HELP],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_HELP;
				}
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[PROGRESS],
					&match) == lwc_error_ok && match)) {
				if (first) {
					value = CURSOR_PROGRESS;
				}
			} else {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (first == false) {
				required_size += sizeof(opv);
			}
		}
	}

	opv = buildOPV(CSS_PROP_CURSOR, flags, value);

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
			token->type != CSS_TOKEN_URI)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (token->type == CSS_TOKEN_IDENT &&
			(lwc_string_caseless_isequal(
			token->idata, c->strings[INHERIT],
			&match) == lwc_error_ok && match)) {
		/* Nothing to do */
	} else {
		bool first = true;

		/* URI* */
		while (token != NULL && token->type == CSS_TOKEN_URI) {
			lwc_string *uri;

			error = c->sheet->resolve(c->sheet->resolve_pw,
					c->sheet->url,
					token->idata, &uri);
			if (error != CSS_OK) {
				*ctx = orig_ctx;
				return error;
			}

			if (first == false) {
				opv = CURSOR_URI;
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}
                        
 			/* Don't ref URI -- we want to pass ownership to the 
			 * bytecode */
			memcpy(ptr, &uri, sizeof(uri));
			ptr += sizeof(uri);

			consumeWhitespace(vector, ctx);

			/* Expect ',' */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || tokenIsChar(token, ',') == false) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			consumeWhitespace(vector, ctx);

			/* Expect either URI or IDENT */
			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_URI)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			first = false;
		}

		/* IDENT */
		if (token != NULL && token->type == CSS_TOKEN_IDENT) {
			if ((lwc_string_caseless_isequal(
					token->idata, c->strings[AUTO],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_AUTO;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[CROSSHAIR],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_CROSSHAIR;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[DEFAULT],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_DEFAULT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[POINTER],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_POINTER;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[MOVE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_MOVE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[E_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_E_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[NE_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_NE_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[NW_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_NW_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[N_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_N_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[SE_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_SE_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[SW_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_SW_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[S_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_S_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[W_RESIZE],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_W_RESIZE;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[TEXT],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_TEXT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[WAIT],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_WAIT;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[HELP],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_HELP;
			} else if ((lwc_string_caseless_isequal(
					token->idata, c->strings[PROGRESS],
					&match) == lwc_error_ok && match)) {
				opv = CURSOR_PROGRESS;
			} else {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}

			if (first == false) {
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}
		}
	}

	return CSS_OK;
}

