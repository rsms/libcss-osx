/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <string.h>

#include "stylesheet.h"
#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/utils.h"

/**
 * Parse a colour specifier
 *
 * \param c       Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx     Pointer to vector iteration context
 * \param result  Pointer to location to receive result (RRGGBBAA)
 * \return CSS_OK      on success,
 *         CSS_INVALID if the input is invalid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_colour_specifier(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint32_t *result)
{
	int orig_ctx = *ctx;
	const css_token *token;
	uint8_t r = 0, g = 0, b = 0, a = 255;
	bool match;
	css_error error;

	consumeWhitespace(vector, ctx);

	/* IDENT(<colour name>) | HASH(rgb | rrggbb) |
	 * FUNCTION(rgb) '(' [ [ NUMBER | PERCENTAGE ] ',' ] {3} ')' |
	 * FUNCTION(rgba) '(' [ [ NUMBER | PERCENTAGE ] ',' ] {3} ',' [ NUMBER | PERCENTAGE ]  ')'
	 *
	 * For quirks, NUMBER | DIMENSION | IDENT, too
	 * I.E. "123456" -> NUMBER, "1234f0" -> DIMENSION, "f00000" -> IDENT
	 */
	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
			token->type != CSS_TOKEN_HASH &&
			token->type != CSS_TOKEN_FUNCTION)) {
		if (c->sheet->quirks_allowed == false ||
				token == NULL ||
				(token->type != CSS_TOKEN_NUMBER &&
				token->type != CSS_TOKEN_DIMENSION))
			goto invalid;
	}

	if (token->type == CSS_TOKEN_IDENT) {
		error = parse_named_colour(c, token->idata, result);
		if (error != CSS_OK && c->sheet->quirks_allowed) {
			error = parse_hash_colour(token->idata, result);
			if (error == CSS_OK)
				c->sheet->quirks_used = true;
		}

		if (error != CSS_OK)
			*ctx = orig_ctx;

		return error;
	} else if (token->type == CSS_TOKEN_HASH) {
		error = parse_hash_colour(token->idata, result);
		if (error != CSS_OK)
			*ctx = orig_ctx;

		return error;
	} else if (c->sheet->quirks_allowed &&
			token->type == CSS_TOKEN_NUMBER) {
		error = parse_hash_colour(token->idata, result);
		if (error == CSS_OK)
			c->sheet->quirks_used = true;
		else
			*ctx = orig_ctx;

		return error;
	} else if (c->sheet->quirks_allowed &&
			token->type == CSS_TOKEN_DIMENSION) {
		error = parse_hash_colour(token->idata, result);
		if (error == CSS_OK)
			c->sheet->quirks_used = true;
		else
			*ctx = orig_ctx;

		return error;
	} else if (token->type == CSS_TOKEN_FUNCTION) {
    int ncomponents = 3;
		if ((lwc_string_caseless_isequal(token->idata, c->strings[RGB], &match) ==
		      lwc_error_ok && match) ||
		    (lwc_string_caseless_isequal(token->idata, c->strings[RGBA], &match) ==
		      lwc_error_ok && match && (ncomponents = 4))) {
			int i;
			css_token_type valid = CSS_TOKEN_NUMBER;

			for (i = 0; i < ncomponents; i++) {
				css_fixed num;
				size_t consumed = 0;
				uint8_t *component = i == 0 ? &r
							                      : i == 1 ? &g
							                      : i == 2 ? &b
							                      : &a;
				int32_t intval;

				consumeWhitespace(vector, ctx);

				token = parserutils_vector_peek(vector, *ctx);
				if (token == NULL || (token->type !=
						CSS_TOKEN_NUMBER &&
						token->type !=
						CSS_TOKEN_PERCENTAGE))
					goto invalid;

				if (i == 0)
					valid = token->type;
				else if (token->type != valid)
					goto invalid;

				num = number_from_lwc_string(token->idata,
						i != 3, // int only?
						&consumed);
				if (consumed != lwc_string_length(token->idata)) {
					goto invalid;
				}

				if (valid == CSS_TOKEN_NUMBER) {
				  if (i == 3) {
            intval = FIXTOINT(FMULI(num, 255));
			    } else {
					  intval = FIXTOINT(num);
				  }
				} else {
					intval = FIXTOINT(FDIVI(FMULI(num, 255), 100));
				}

				if (intval > 255)
					*component = 255;
				else if (intval < 0)
					*component = 0;
				else
					*component = intval;

				parserutils_vector_iterate(vector, ctx);

				consumeWhitespace(vector, ctx);

				token = parserutils_vector_peek(vector, *ctx);
				if (token == NULL)
					goto invalid;
        
        if (ncomponents == 4) {
  				if (i != 3 && tokenIsChar(token, ','))
  					parserutils_vector_iterate(vector, ctx);
  				else if (i == 3 && tokenIsChar(token, ')'))
  					parserutils_vector_iterate(vector, ctx);
  				else
  					goto invalid;
				} else {
  				if (i != 2 && tokenIsChar(token, ','))
  					parserutils_vector_iterate(vector, ctx);
  				else if (i == 2 && tokenIsChar(token, ')'))
  					parserutils_vector_iterate(vector, ctx);
  				else
  					goto invalid;
				}
			}
		} else
			goto invalid;
	}

	*result = (r << 24) | (g << 16) | (b << 8) | a;

	return CSS_OK;

invalid:
	*ctx = orig_ctx;
	return CSS_INVALID;
}

/**
 * Parse a named colour
 *
 * \param c       Parsing context
 * \param data    Colour name string
 * \param result  Pointer to location to receive result
 * \return CSS_OK      on success,
 *         CSS_INVALID if the colour name is unknown
 */
css_error parse_named_colour(css_language *c, lwc_string *data,
		uint32_t *result)
{
	static const uint32_t colourmap[LAST_COLOUR + 1 - FIRST_COLOUR] = {
		0xf0f8ffff, /* ALICEBLUE */
		0xfaebd7ff, /* ANTIQUEWHITE */
		0x00ffffff, /* AQUA */
		0x7fffd4ff, /* AQUAMARINE */
		0xf0ffffff, /* AZURE */
		0xf5f5dcff, /* BEIGE */
		0xffe4c4ff, /* BISQUE */
		0x000000ff, /* BLACK */
		0xffebcdff, /* BLANCHEDALMOND */
		0x0000ffff, /* BLUE */
		0x8a2be2ff, /* BLUEVIOLET */
		0xa52a2aff, /* BROWN */
		0xdeb887ff, /* BURLYWOOD */
		0x5f9ea0ff, /* CADETBLUE */
		0x7fff00ff, /* CHARTREUSE */
		0xd2691eff, /* CHOCOLATE */
		0xff7f50ff, /* CORAL */
		0x6495edff, /* CORNFLOWERBLUE */
		0xfff8dcff, /* CORNSILK */
		0xdc143cff, /* CRIMSON */
		0x00ffffff, /* CYAN */
		0x00008bff, /* DARKBLUE */
		0x008b8bff, /* DARKCYAN */
		0xb8860bff, /* DARKGOLDENROD */
		0xa9a9a9ff, /* DARKGRAY */
		0x006400ff, /* DARKGREEN */
		0xa9a9a9ff, /* DARKGREY */
		0xbdb76bff, /* DARKKHAKI */
		0x8b008bff, /* DARKMAGENTA */
		0x556b2fff, /* DARKOLIVEGREEN */
		0xff8c00ff, /* DARKORANGE */
		0x9932ccff, /* DARKORCHID */
		0x8b0000ff, /* DARKRED */
		0xe9967aff, /* DARKSALMON */
		0x8fbc8fff, /* DARKSEAGREEN */
		0x483d8bff, /* DARKSLATEBLUE */
		0x2f4f4fff, /* DARKSLATEGRAY */
		0x2f4f4fff, /* DARKSLATEGREY */
		0x00ced1ff, /* DARKTURQUOISE */
		0x9400d3ff, /* DARKVIOLET */
		0xff1493ff, /* DEEPPINK */
		0x00bfffff, /* DEEPSKYBLUE */
		0x696969ff, /* DIMGRAY */
		0x696969ff, /* DIMGREY */
		0x1e90ffff, /* DODGERBLUE */
		0xd19275ff, /* FELDSPAR */
		0xb22222ff, /* FIREBRICK */
		0xfffaf0ff, /* FLORALWHITE */
		0x228b22ff, /* FORESTGREEN */
		0xff00ffff, /* FUCHSIA */
		0xdcdcdcff, /* GAINSBORO */
		0xf8f8ffff, /* GHOSTWHITE */
		0xffd700ff, /* GOLD */
		0xdaa520ff, /* GOLDENROD */
		0x808080ff, /* GRAY */
		0x008000ff, /* GREEN */
		0xadff2fff, /* GREENYELLOW */
		0x808080ff, /* GREY */
		0xf0fff0ff, /* HONEYDEW */
		0xff69b4ff, /* HOTPINK */
		0xcd5c5cff, /* INDIANRED */
		0x4b0082ff, /* INDIGO */
		0xfffff0ff, /* IVORY */
		0xf0e68cff, /* KHAKI */
		0xe6e6faff, /* LAVENDER */
		0xfff0f5ff, /* LAVENDERBLUSH */
		0x7cfc00ff, /* LAWNGREEN */
		0xfffacdff, /* LEMONCHIFFON */
		0xadd8e6ff, /* LIGHTBLUE */
		0xf08080ff, /* LIGHTCORAL */
		0xe0ffffff, /* LIGHTCYAN */
		0xfafad2ff, /* LIGHTGOLDENRODYELLOW */
		0xd3d3d3ff, /* LIGHTGRAY */
		0x90ee90ff, /* LIGHTGREEN */
		0xd3d3d3ff, /* LIGHTGREY */
		0xffb6c1ff, /* LIGHTPINK */
		0xffa07aff, /* LIGHTSALMON */
		0x20b2aaff, /* LIGHTSEAGREEN */
		0x87cefaff, /* LIGHTSKYBLUE */
		0x8470ffff, /* LIGHTSLATEBLUE */
		0x778899ff, /* LIGHTSLATEGRAY */
		0x778899ff, /* LIGHTSLATEGREY */
		0xb0c4deff, /* LIGHTSTEELBLUE */
		0xffffe0ff, /* LIGHTYELLOW */
		0x00ff00ff, /* LIME */
		0x32cd32ff, /* LIMEGREEN */
		0xfaf0e6ff, /* LINEN */
		0xff00ffff, /* MAGENTA */
		0x800000ff, /* MAROON */
		0x66cdaaff, /* MEDIUMAQUAMARINE */
		0x0000cdff, /* MEDIUMBLUE */
		0xba55d3ff, /* MEDIUMORCHID */
		0x9370dbff, /* MEDIUMPURPLE */
		0x3cb371ff, /* MEDIUMSEAGREEN */
		0x7b68eeff, /* MEDIUMSLATEBLUE */
		0x00fa9aff, /* MEDIUMSPRINGGREEN */
		0x48d1ccff, /* MEDIUMTURQUOISE */
		0xc71585ff, /* MEDIUMVIOLETRED */
		0x191970ff, /* MIDNIGHTBLUE */
		0xf5fffaff, /* MINTCREAM */
		0xffe4e1ff, /* MISTYROSE */
		0xffe4b5ff, /* MOCCASIN */
		0xffdeadff, /* NAVAJOWHITE */
		0x000080ff, /* NAVY */
		0xfdf5e6ff, /* OLDLACE */
		0x808000ff, /* OLIVE */
		0x6b8e23ff, /* OLIVEDRAB */
		0xffa500ff, /* ORANGE */
		0xff4500ff, /* ORANGERED */
		0xda70d6ff, /* ORCHID */
		0xeee8aaff, /* PALEGOLDENROD */
		0x98fb98ff, /* PALEGREEN */
		0xafeeeeff, /* PALETURQUOISE */
		0xdb7093ff, /* PALEVIOLETRED */
		0xffefd5ff, /* PAPAYAWHIP */
		0xffdab9ff, /* PEACHPUFF */
		0xcd853fff, /* PERU */
		0xffc0cbff, /* PINK */
		0xdda0ddff, /* PLUM */
		0xb0e0e6ff, /* POWDERBLUE */
		0x800080ff, /* PURPLE */
		0xff0000ff, /* RED */
		0xbc8f8fff, /* ROSYBROWN */
		0x4169e1ff, /* ROYALBLUE */
		0x8b4513ff, /* SADDLEBROWN */
		0xfa8072ff, /* SALMON */
		0xf4a460ff, /* SANDYBROWN */
		0x2e8b57ff, /* SEAGREEN */
		0xfff5eeff, /* SEASHELL */
		0xa0522dff, /* SIENNA */
		0xc0c0c0ff, /* SILVER */
		0x87ceebff, /* SKYBLUE */
		0x6a5acdff, /* SLATEBLUE */
		0x708090ff, /* SLATEGRAY */
		0x708090ff, /* SLATEGREY */
		0xfffafaff, /* SNOW */
		0x00ff7fff, /* SPRINGGREEN */
		0x4682b4ff, /* STEELBLUE */
		0xd2b48cff, /* TAN */
		0x008080ff, /* TEAL */
		0xd8bfd8ff, /* THISTLE */
		0xff6347ff, /* TOMATO */
		0x40e0d0ff, /* TURQUOISE */
		0xee82eeff, /* VIOLET */
		0xd02090ff, /* VIOLETRED */
		0xf5deb3ff, /* WHEAT */
		0xffffffff, /* WHITE */
		0xf5f5f5ff, /* WHITESMOKE */
		0xffff00ff, /* YELLOW */
		0x9acd32ff  /* YELLOWGREEN */
	};
	int i;
	bool match;

	for (i = FIRST_COLOUR; i <= LAST_COLOUR; i++) {
          if (lwc_string_caseless_isequal(data, c->strings[i],
                                          &match) == lwc_error_ok &&
				match)
			break;
	}
	if (i == LAST_COLOUR + 1)
		return CSS_INVALID;

	*result = colourmap[i - FIRST_COLOUR];

	return CSS_OK;
}

/**
 * Parse a hash colour (#rgb or #rrggbb)
 *
 * \param data    Pointer to colour string
 * \param result  Pointer to location to receive result
 * \return CSS_OK      on success,
 *         CSS_INVALID if the input is invalid
 */
css_error parse_hash_colour(lwc_string *data, uint32_t *result)
{
	uint8_t r = 0, g = 0, b = 0;
	size_t len = lwc_string_length(data);
	const char *input = lwc_string_data(data);

	if (len == 3 &&	isHex(input[0]) && isHex(input[1]) &&
			isHex(input[2])) {
		r = charToHex(input[0]);
		g = charToHex(input[1]);
		b = charToHex(input[2]);

		r |= (r << 4);
		g |= (g << 4);
		b |= (b << 4);
	} else if (len == 6 && isHex(input[0]) && isHex(input[1]) &&
			isHex(input[2]) && isHex(input[3]) &&
			isHex(input[4]) && isHex(input[5])) {
		r = (charToHex(input[0]) << 4);
		r |= charToHex(input[1]);
		g = (charToHex(input[2]) << 4);
		g |= charToHex(input[3]);
		b = (charToHex(input[4]) << 4);
		b |= charToHex(input[5]);
	} else
		return CSS_INVALID;

	*result = (r << 24) | (g << 16) | (b << 8) | 0xff;

	return CSS_OK;
}

/**
 * Parse a unit specifier
 *
 * \param c             Parsing context
 * \param vector        Vector of tokens to process
 * \param ctx           Pointer to current vector iteration context
 * \param default_unit  The default unit to use if none specified
 * \param length        Pointer to location to receive length
 * \param unit          Pointer to location to receive unit
 * \return CSS_OK      on success,
 *         CSS_INVALID if the tokens do not form a valid unit
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error parse_unit_specifier(css_language *c,
		const parserutils_vector *vector, int *ctx,
		uint32_t default_unit,
		css_fixed *length, uint32_t *unit)
{
	int orig_ctx = *ctx;
	const css_token *token;
	css_fixed num;
	size_t consumed = 0;
	css_error error;

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || (token->type != CSS_TOKEN_DIMENSION &&
			token->type != CSS_TOKEN_NUMBER &&
			token->type != CSS_TOKEN_PERCENTAGE)) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	num = number_from_lwc_string(token->idata, false, &consumed);

	if (token->type == CSS_TOKEN_DIMENSION) {
		size_t len = lwc_string_length(token->idata);
		const char *data = lwc_string_data(token->idata);
		css_unit temp_unit = CSS_UNIT_PX;

		error = parse_unit_keyword(data + consumed, len - consumed,
				&temp_unit);
		if (error != CSS_OK) {
			*ctx = orig_ctx;
			return error;
		}

		*unit = (uint32_t) temp_unit;
	} else if (token->type == CSS_TOKEN_NUMBER) {
		/* Non-zero values are permitted in quirks mode */
		if (num != 0) {
			if (c->sheet->quirks_allowed) {
				c->sheet->quirks_used = true;
			} else {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		}

		*unit = default_unit;

		if (c->sheet->quirks_allowed) {
			/* Also, in quirks mode, we need to cater for
			 * dimensions separated from their units by whitespace
			 * (e.g. "0 px")
			 */
			int temp_ctx = *ctx;
			css_unit temp_unit;

			consumeWhitespace(vector, &temp_ctx);

			/* Try to parse the unit keyword, ignoring errors */
			token = parserutils_vector_iterate(vector, &temp_ctx);
			if (token != NULL && token->type == CSS_TOKEN_IDENT) {
				error = parse_unit_keyword(
						lwc_string_data(token->idata),
						lwc_string_length(token->idata),
						&temp_unit);
				if (error == CSS_OK) {
					c->sheet->quirks_used = true;
					*ctx = temp_ctx;
					*unit = (uint32_t) temp_unit;
				}
			}
		}
	} else {
		/* Percentage -- number must be entire token data */
		if (consumed != lwc_string_length(token->idata)) {
			*ctx = orig_ctx;
			return CSS_INVALID;
		}
		*unit = UNIT_PCT;
	}

	*length = num;

	return CSS_OK;
}

/**
 * Parse a unit keyword
 *
 * \param ptr   Pointer to keyword string
 * \param len   Length, in bytes, of string
 * \param unit  Pointer to location to receive computed unit
 * \return CSS_OK      on success,
 *         CSS_INVALID on encountering an unknown keyword
 */
css_error parse_unit_keyword(const char *ptr, size_t len, css_unit *unit)
{
	if (len == 4) {
		if (strncasecmp(ptr, "grad", 4) == 0)
			*unit = UNIT_GRAD;
		else
			return CSS_INVALID;
	} else if (len == 3) {
		if (strncasecmp(ptr, "kHz", 3) == 0)
			*unit = UNIT_KHZ;
		else if (strncasecmp(ptr, "deg", 3) == 0)
			*unit = UNIT_DEG;
		else if (strncasecmp(ptr, "rad", 3) == 0)
			*unit = UNIT_RAD;
		else
			return CSS_INVALID;
	} else if (len == 2) {
		if (strncasecmp(ptr, "Hz", 2) == 0)
			*unit = UNIT_HZ;
		else if (strncasecmp(ptr, "ms", 2) == 0)
			*unit = UNIT_MS;
		else if (strncasecmp(ptr, "px", 2) == 0)
			*unit = UNIT_PX;
		else if (strncasecmp(ptr, "ex", 2) == 0)
			*unit = UNIT_EX;
		else if (strncasecmp(ptr, "em", 2) == 0)
			*unit = UNIT_EM;
		else if (strncasecmp(ptr, "in", 2) == 0)
			*unit = UNIT_IN;
		else if (strncasecmp(ptr, "cm", 2) == 0)
			*unit = UNIT_CM;
		else if (strncasecmp(ptr, "mm", 2) == 0)
			*unit = UNIT_MM;
		else if (strncasecmp(ptr, "pt", 2) == 0)
			*unit = UNIT_PT;
		else if (strncasecmp(ptr, "pc", 2) == 0)
			*unit = UNIT_PC;
		else
			return CSS_INVALID;
	} else if (len == 1) {
		if (strncasecmp(ptr, "s", 1) == 0)
			*unit = UNIT_S;
		else
			return CSS_INVALID;
	} else
		return CSS_INVALID;

	return CSS_OK;
}

/**
 * Parse a comma separated list, calculating the storage space required
 *
 * \param c         Parsing context
 * \param vector    Vector of tokens to process
 * \param ctx       Pointer to vector iteration context
 * \param token     Pointer to current token
 * \param reserved  Callback to determine if an identifier is reserved
 * \param size      Pointer to location to receive required storage
 * \return CSS_OK      on success,
 *         CSS_INVALID if the input is invalid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error comma_list_length(css_language *c,
		const parserutils_vector *vector, int *ctx,
		const css_token *token,
		bool (*reserved)(css_language *c, const css_token *ident),
		uint32_t *size)
{
	int orig_ctx = *ctx;
	uint32_t opv;
	uint32_t required_size = 0;
	bool first = true;

	while (token != NULL) {
		if (token->type == CSS_TOKEN_IDENT) {
			/* IDENT+ */
			required_size += first ? 0 : sizeof(opv);

			if (reserved(c, token) == false) {
				required_size += sizeof(lwc_string *);

				/* Skip past [ IDENT* S* ]* */
				while (token != NULL) {
					token = parserutils_vector_peek(
							vector, *ctx);
					if (token != NULL &&
						token->type !=
							CSS_TOKEN_IDENT &&
							token->type !=
							CSS_TOKEN_S) {
						break;
					}

					/* Idents must not be reserved */
					if (token != NULL && token->type ==
							CSS_TOKEN_IDENT &&
							reserved(c, token)) {
						*ctx = orig_ctx;
						return CSS_INVALID;
					}

					token = parserutils_vector_iterate(
						vector, ctx);
				}
			}
		} else if (token->type == CSS_TOKEN_STRING) {
			/* STRING */
			required_size += first ? 0 : sizeof(opv);

			required_size += sizeof(lwc_string *);
		} else {
			/* Invalid token */
			*ctx = orig_ctx;
			return CSS_INVALID;
		}

		consumeWhitespace(vector, ctx);

		/* Look for a comma */
		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && tokenIsChar(token, ',')) {
			/* Got one. Move past it */
			parserutils_vector_iterate(vector, ctx);

			consumeWhitespace(vector, ctx);

			/* Ensure that a valid token follows */
			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_STRING)) {
				*ctx = orig_ctx;
				return CSS_INVALID;
			}
		} else {
			/* No comma, so we're done */
			break;
		}

		/* Flag that this is no longer the first pass */
		first = false;

		/* Iterate for next chunk */
		token = parserutils_vector_iterate(vector, ctx);
	}

	required_size += sizeof(opv);

	*size = required_size;

	return CSS_OK;
}

/**
 * Parse a comma separated list, converting to bytecode
 *
 * \param c          Parsing context
 * \param vector     Vector of tokens to process
 * \param ctx        Pointer to vector iteration context
 * \param token      Pointer to current token
 * \param reserved   Callback to determine if an identifier is reserved
 * \param get_value  Callback to retrieve bytecode value for a token
 * \param bytecode   Pointer to pointer to bytecode buffer. Updated on exit.
 * \return CSS_OK      on success,
 *         CSS_INVALID if the input is invalid
 *
 * \note The bytecode buffer must be at least comma_list_length() bytes long.
 *
 * Post condition: \a *ctx is updated with the next token to process
 *                 If the input is invalid, then \a *ctx remains unchanged.
 */
css_error comma_list_to_bytecode(css_language *c,
		const parserutils_vector *vector, int *ctx,
		const css_token *token,
		bool (*reserved)(css_language *c, const css_token *ident),
		uint16_t (*get_value)(css_language *c, const css_token *token),
		uint8_t **bytecode)
{
	int orig_ctx = *ctx;
	uint8_t *ptr = *bytecode;
	bool first = true;
	uint32_t opv;
	uint8_t *buf = NULL;
	uint32_t buflen = 0;
	css_error error = CSS_OK;

	while (token != NULL) {
		if (token->type == CSS_TOKEN_IDENT) {
			lwc_string *tok_idata = token->idata;
			lwc_string *name = tok_idata;
			lwc_string *newname;

			opv = get_value(c, token);

			if (first == false) {
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}

			if (reserved(c, token) == false) {
				uint32_t len = lwc_string_length(token->idata);
				const css_token *temp_token = token;
				int temp_ctx = *ctx;
				lwc_error lerror;
				uint8_t *p;

				/* Calculate required length of temp buffer */
				while (temp_token != NULL) {
					temp_token = parserutils_vector_peek(
							vector, temp_ctx);
					if (temp_token != NULL &&
						temp_token->type !=
						CSS_TOKEN_IDENT &&
							temp_token->type !=
							CSS_TOKEN_S) {
						break;
					}

					if (temp_token != NULL &&
						temp_token->type ==
							CSS_TOKEN_IDENT) {
						len += lwc_string_length(
							temp_token->idata);
					} else if (temp_token != NULL) {
						len += 1;
					}

					temp_token = parserutils_vector_iterate(
							vector, &temp_ctx);
				}

				if (len > buflen) {
					/* Allocate buffer */
					uint8_t *b = c->alloc(buf, len, c->pw);
					if (b == NULL) {
						error = CSS_NOMEM;
						goto cleanup;
					}

					buf = b;
					buflen = len;
				}

				p = buf;

				/* Populate buffer with string data */
				memcpy(p, lwc_string_data(token->idata),
					lwc_string_length(token->idata));
				p += lwc_string_length(token->idata);

				while (token != NULL) {
					token = parserutils_vector_peek(
							vector, *ctx);
					if (token != NULL && token->type !=
							CSS_TOKEN_IDENT &&
							token->type !=
							CSS_TOKEN_S) {
						break;
					}

					if (token != NULL && token->type ==
							CSS_TOKEN_IDENT) {
						memcpy(p, lwc_string_data(
								token->idata),
							lwc_string_length(
								token->idata));
						p += lwc_string_length(
								token->idata);
					} else if (token != NULL) {
						*p++ = ' ';
					}

					token = parserutils_vector_iterate(
							vector, ctx);
				}

				/* Strip trailing whitespace */
				while (p > buf && p[-1] == ' ')
					p--;

				/* Insert into hash, if it's different
				 * from the name we already have */
				lerror = lwc_intern_string(
						(char *) buf, p - buf,
						&newname);
				if (lerror != lwc_error_ok) {
					error = css_error_from_lwc_error(
							lerror);
					goto cleanup;
				}

				if (newname == name) {
					lwc_string_unref(newname);
				}

				name = newname;

				/* Only ref 'name' again if the token owns it,
				 * otherwise we already own the only ref to the
				 * new name generated above.
				 */
				if (name == tok_idata) {
					lwc_string_ref(name);
				}

				memcpy(ptr, &name, sizeof(name));
				ptr += sizeof(name);
			}
		} else if (token->type == CSS_TOKEN_STRING) {
			opv = get_value(c, token);

			if (first == false) {
				memcpy(ptr, &opv, sizeof(opv));
				ptr += sizeof(opv);
			}

			lwc_string_ref(token->idata);

			memcpy(ptr, &token->idata, sizeof(token->idata));
			ptr += sizeof(token->idata);
		} else {
			error = CSS_INVALID;
			goto cleanup;
		}

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && tokenIsChar(token, ',')) {
			parserutils_vector_iterate(vector, ctx);

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
			if (token == NULL || (token->type != CSS_TOKEN_IDENT &&
					token->type != CSS_TOKEN_STRING)) {
				error = CSS_INVALID;
				goto cleanup;
			}
		} else {
			break;
		}

		first = false;

		token = parserutils_vector_iterate(vector, ctx);
	}

	*bytecode = ptr;

cleanup:
	if (buf)
		c->alloc(buf, 0, c->pw);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

