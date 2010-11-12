#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libcss/libcss.h>

#include "stylesheet.h"
#include "utils/utils.h"

#include "testutils.h"

/** \todo at some point, we need to extend this to handle nested blocks */
typedef struct exp_entry {
	int type;
#define MAX_RULE_NAME_LEN (128)
	char name[MAX_RULE_NAME_LEN];
	size_t bclen;
	size_t bcused;
	uint8_t *bytecode;

	size_t stlen;
	size_t stused;
	struct stentry {
		size_t off;
		char *string;
	} *stringtab;
} exp_entry;

typedef struct line_ctx {
	size_t buflen;
	size_t bufused;
	uint8_t *buf;

	size_t explen;
	size_t expused;
	exp_entry *exp;

	bool indata;
	bool inerrors;
	bool inexp;

	bool inrule;
} line_ctx;

static bool handle_line(const char *data, size_t datalen, void *pw);
static void parse_expected(line_ctx *ctx, const char *data, size_t len);
static void run_test(const uint8_t *data, size_t len, 
		exp_entry *exp, size_t explen);
static void validate_rule_selector(css_rule_selector *s, exp_entry *e, 
		int testnum);
static void validate_rule_charset(css_rule_charset *s, exp_entry *e, 
		int testnum);
static void validate_rule_import(css_rule_import *s, exp_entry *e, 
		int testnum);

static void dump_selector_list(css_selector *list, char **ptr);
static void dump_selector(css_selector *selector, char **ptr);
static void dump_selector_detail(css_selector_detail *detail, char **ptr);
static void dump_string(lwc_string *string, char **ptr);

static void *myrealloc(void *data, size_t len, void *pw)
{
	UNUSED(pw);
	
	return realloc(data, len);
}

static void *counting_realloc(void *data, size_t len, void *pw)
{
	size_t *counter = (size_t *)pw;
	
	if (data == NULL)
		*counter += 1;
	else if (len == 0)
		*counter -= 1;
	
	return realloc(data, len);
}

static css_error resolve_url(void *pw,
		const char *base, lwc_string *rel, lwc_string **abs)
{
	UNUSED(pw);
	UNUSED(base);

	/* About as useless as possible */
	*abs = lwc_string_ref(rel);

	return CSS_OK;
}

static void
printing_lwc_iterator(lwc_string *str, void *pw)
{
	UNUSED(pw);
	
	printf(" DICT: %*s\n", (int)(lwc_string_length(str)), lwc_string_data(str));
}

int main(int argc, char **argv)
{
	line_ctx ctx;
	size_t counter = 0;

	if (argc != 3) {
		printf("Usage: %s <aliases_file> <filename>\n", argv[0]);
		return 1;
	}

	assert(css_initialise(argv[1], myrealloc, NULL) == CSS_OK);

	ctx.buflen = parse_filesize(argv[2]);
	if (ctx.buflen == 0)
		return 1;

	ctx.buf = malloc(ctx.buflen);
	if (ctx.buf == NULL) {
		printf("Failed allocating %u bytes\n",
				(unsigned int) ctx.buflen);
		return 1;
	}

	ctx.buf[0] = '\0';
	ctx.bufused = 0;
	ctx.explen = 0;
	ctx.expused = 0;
	ctx.exp = NULL;
	ctx.indata = false;
	ctx.inerrors = false;
	ctx.inexp = false;

	assert(lwc_initialise(counting_realloc, &counter, 0) == lwc_error_ok);
	
	assert(parse_testfile(argv[2], handle_line, &ctx) == true);

	/* and run final test */
	if (ctx.bufused > 0)
		run_test(ctx.buf, ctx.bufused, ctx.exp, ctx.expused);

	free(ctx.buf);

	assert(css_finalise(myrealloc, NULL) == CSS_OK);
	
	printf("INFO: Counter is %zu\n", counter);
	lwc_iterate_strings(printing_lwc_iterator, NULL);
	
	assert(counter == 2);
	
	printf("PASS\n");

	return 0;
}

bool handle_line(const char *data, size_t datalen, void *pw)
{
	line_ctx *ctx = (line_ctx *) pw;

	if (data[0] == '#') {
		if (ctx->inexp) {
			/* This marks end of testcase, so run it */

			run_test(ctx->buf, ctx->bufused, 
					ctx->exp, ctx->expused);

			ctx->buf[0] = '\0';
			ctx->bufused = 0;

			ctx->expused = 0;
		}

		if (ctx->indata && strncasecmp(data+1, "errors", 6) == 0) {
			ctx->indata = false;
			ctx->inerrors = true;
			ctx->inexp = false;
		} else if (ctx->inerrors && 
				strncasecmp(data+1, "expected", 8) == 0) {
			ctx->indata = false;
			ctx->inerrors = false;
			ctx->inexp = true;
			ctx->inrule = false;
		} else if (ctx->inexp && strncasecmp(data+1, "data", 4) == 0) {
			ctx->indata = true;
			ctx->inerrors = false;
			ctx->inexp = false;
		} else if (ctx->indata) {
			memcpy(ctx->buf + ctx->bufused, data, datalen);
			ctx->bufused += datalen;
		} else {
			ctx->indata = (strncasecmp(data+1, "data", 4) == 0);
			ctx->inerrors = (strncasecmp(data+1, "errors", 6) == 0);
			ctx->inexp = (strncasecmp(data+1, "expected", 8) == 0);
		}
	} else {
		if (ctx->indata) {
			memcpy(ctx->buf + ctx->bufused, data, datalen);
			ctx->bufused += datalen;
		}
		if (ctx->inexp) {
			if (data[datalen - 1] == '\n')
				datalen -= 1;

			parse_expected(ctx, data, datalen);
		}
	}

	return true;
}

void parse_expected(line_ctx *ctx, const char *data, size_t len)
{
	/* Ignore blanks or lines that don't start with | */
	if (len == 0 || data[0] != '|')
		return;

	if (ctx->inrule == false) {
		char *name;
		int type;
		
start_rule:
		type = strtol(data + 1, &name, 10);

		while (isspace(*name))
			name++;

		/* Append to list of expected rules */
		if (ctx->expused == ctx->explen) {
			size_t num = ctx->explen == 0 ? 4 : ctx->explen;

			exp_entry *temp = realloc(ctx->exp, 
					num * 2 * sizeof(exp_entry));
			if (temp == NULL) {
				assert(0 && "No memory for expected rules");
			}

			ctx->exp = temp;
			ctx->explen = num * 2;
		}

		ctx->exp[ctx->expused].type = type;
		memcpy(ctx->exp[ctx->expused].name, name, 
				min(len - (name - data), MAX_RULE_NAME_LEN));
		ctx->exp[ctx->expused].name[min(len - (name - data), 
				MAX_RULE_NAME_LEN - 1)] = '\0';
		ctx->exp[ctx->expused].bclen = 0;
		ctx->exp[ctx->expused].bcused = 0;
		ctx->exp[ctx->expused].bytecode = NULL;
		ctx->exp[ctx->expused].stlen = 0;
		ctx->exp[ctx->expused].stused = 0;
		ctx->exp[ctx->expused].stringtab =  NULL;

		ctx->expused++;
		
		ctx->inrule = true;
	} else {
		char *next = (char *) data + 1;
		exp_entry *rule = &ctx->exp[ctx->expused - 1];

		if (data[2] != ' ') {
			ctx->inrule = false;
			goto start_rule;
		}

		/** \todo how to deal with pointers? */
		while (next < data + len) {
			/* Skip whitespace */
			while (next < data + len && isspace(*next))
				next++;

			if (next == data + len)
				break;

			if (rule->bcused >= rule->bclen) {
				size_t num = rule->bcused == 0 ? 4 : 
						rule->bcused;

				uint8_t *temp = realloc(rule->bytecode,
						num * 2);
				if (temp == NULL) {
					assert(0 && "No memory for bytecode");
				}

				rule->bytecode = temp;
				rule->bclen = num * 2;
			}

			if (*next == 'P') {
				/* Pointer */
				const char *str;

				while (next < data + len && *next != '(')
					next++;
				str = next + 1;
				while (next < data + len && *next != ')')
					next++;
				next++;

				if (rule->stused >= rule->stlen) {
					size_t num = rule->stused == 0 ? 4 :
							rule->stused;

					struct stentry *temp = realloc(
						rule->stringtab, 
						num * 2 * sizeof(struct stentry));
					if (temp == NULL) {
						assert(0 && 
						"No memory for string table");
					}

					rule->stringtab = temp;
					rule->stlen = num * 2;
				}

				rule->stringtab[rule->stused].off = 
						rule->bcused;
				rule->stringtab[rule->stused].string =
						malloc(next - str);
				assert(rule->stringtab[rule->stused].string != 
						NULL);
				memcpy(rule->stringtab[rule->stused].string,
						str, next - str - 1);
				rule->stringtab[rule->stused].string[
						next - str - 1]	 = '\0';

				rule->bcused += sizeof(void *);
				rule->stused++;
			} else {
				/* Assume hexnum */
				uint32_t val = strtoul(next, &next, 16);

				/* Append to bytecode */
				memcpy(rule->bytecode + rule->bcused, 
						&val, sizeof(val));
				rule->bcused += sizeof(val);
			}
		}
	}
}

void run_test(const uint8_t *data, size_t len, exp_entry *exp, size_t explen)
{
	css_stylesheet *sheet;
	css_rule *rule;
	css_error error;
	size_t e;
	static int testnum;
	
	assert(css_stylesheet_create(CSS_LEVEL_21, "UTF-8", "foo", NULL,
			false, false, myrealloc, NULL, resolve_url, NULL, 
			&sheet) == CSS_OK);

	error = css_stylesheet_append_data(sheet, data, len);
	if (error != CSS_OK && error != CSS_NEEDDATA) {
		printf("Failed appending data: %d\n", error);
		assert(0);
	}

	error = css_stylesheet_data_done(sheet);
	assert(error == CSS_OK || error == CSS_IMPORTS_PENDING);

	while (error == CSS_IMPORTS_PENDING) {
		lwc_string *url;
		uint64_t media;

		error = css_stylesheet_next_pending_import(sheet,
				&url, &media);
		assert(error == CSS_OK || error == CSS_INVALID);

		if (error == CSS_OK) {
			css_stylesheet *import;
			char *buf = alloca(lwc_string_length(url) + 1);

			memcpy(buf, lwc_string_data(url), 
					lwc_string_length(url));
			buf[lwc_string_length(url)] = '\0';

			assert(css_stylesheet_create(CSS_LEVEL_21,
				"UTF-8", buf, NULL, false, false,
				myrealloc, NULL, resolve_url, NULL, 
				&import) == CSS_OK);

			assert(css_stylesheet_register_import(sheet,
				import) == CSS_OK);

			error = CSS_IMPORTS_PENDING;
			lwc_string_unref(url);
		}
	}

	e = 0;
	testnum++;

	if (sheet->rule_count != explen) {
		printf("%d: Got %d rules. Expected %u\n",
				testnum, sheet->rule_count, (int) explen);
		assert(0 && "Unexpected number of rules");
	}

	for (rule = sheet->rule_list; rule != NULL; rule = rule->next, e++) {
		if (rule->type != exp[e].type) {
			printf("%d: Got type %d. Expected %d\n", 
				testnum, rule->type, exp[e].type);
			assert(0 && "Types differ");
		}

		switch (rule->type) {
		case CSS_RULE_SELECTOR:
			validate_rule_selector((css_rule_selector *) rule,
					&exp[e], testnum);
			break;
		case CSS_RULE_CHARSET:
			validate_rule_charset((css_rule_charset *) rule,
					&exp[e], testnum);
			break;
		case CSS_RULE_IMPORT:
			validate_rule_import((css_rule_import *) rule,
					&exp[e], testnum);
			break;
		default:
			printf("%d: Unhandled rule type %d\n",
				testnum, rule->type);
			break;
		}
	}

	assert(e == explen);

	css_stylesheet_destroy(sheet);

	printf("Test %d: PASS\n", testnum);
}

void validate_rule_selector(css_rule_selector *s, exp_entry *e, int testnum)
{
	char name[MAX_RULE_NAME_LEN];
	char *ptr = name;
	uint32_t i;

	/* Build selector string */
	for (i = 0; i < s->base.items; i++) {
		dump_selector_list(s->selectors[i], &ptr);
		if (i != (uint32_t) (s->base.items - 1)) {
			memcpy(ptr, ", ", 2);
			ptr += 2;
		}
	}
	*ptr = '\0';

	/* Compare with expected selector */
	if (strcmp(e->name, name) != 0) {
		printf("%d: Got name '%s'. Expected '%s'\n",
			testnum, name, e->name);
		assert(0 && "Mismatched names");
	}

	/* Now compare bytecode */
	if (e->bytecode != NULL && s->style == NULL) {
		printf("%d: Expected bytecode but none created\n",
			testnum);
		assert(0 && "No bytecode");
	} else if (e->bytecode == NULL && s->style != NULL) {
		printf("%d: No bytecode expected but some created\n",
			testnum);
		assert(0 && "Unexpected bytecode");
	} else if (e->bytecode != NULL && s->style != NULL) {
		size_t i;

		if (s->style->length != e->bcused) {
			printf("%d: Got length %d, Expected %u\n",
				testnum, s->style->length, 
				(int) e->bcused);
			assert(0 && "Bytecode lengths differ");
		}

		for (i = 0; i < e->bcused; i++) {
			size_t j;

			for (j = 0; j < e->stused; j++) {
				if (e->stringtab[j].off == i)
					break;
			}

			if (j != e->stused) {
				lwc_string **p = (void *) ((uint8_t *) 
						s->style->bytecode + i);

				if (lwc_string_length(*p) != 
					strlen(e->stringtab[j].string) ||
					memcmp(lwc_string_data(*p), 
						e->stringtab[j].string,
						lwc_string_length(*p)) != 0) {
					printf("%d: Got string '%.*s'. "
						"Expected '%s'\n",
						testnum, 
						(int) lwc_string_length(*p), 
						lwc_string_data(*p), 
						e->stringtab[j].string);
					assert(0 && "Strings differ");
				}

				i += sizeof (void *) - 1;
			} else if (((uint8_t *) s->style->bytecode)[i] != 
					e->bytecode[i]) {
				printf("%d: Bytecode differs at %u\n",
					testnum, (int) i);
				while (i < e->bcused) {
					printf("%.2x ", 
						((uint8_t *) s->style->bytecode)[i]);
					i++;
				}
				printf("\n");
				assert(0 && "Bytecode differs");
			}
		}
	}
}

void validate_rule_charset(css_rule_charset *s, exp_entry *e, int testnum)
{
	char name[MAX_RULE_NAME_LEN];
	char *ptr = name;

	dump_string(s->encoding, &ptr);
	*ptr = '\0';

	if (strcmp(name, e->name) != 0) {
		printf("%d: Got charset '%s'. Expected '%s'\n",
			testnum, name, e->name);
		assert(0 && "Mismatched charsets");
	}
}

void validate_rule_import(css_rule_import *s, exp_entry *e, int testnum)
{
	if (strncmp(lwc_string_data(s->url), e->name,
		    lwc_string_length(s->url)) != 0) {
		printf("%d: Got URL '%.*s'. Expected '%s'\n",
			testnum, (int) lwc_string_length(s->url), 
			lwc_string_data(s->url),
		e->name);
		assert(0 && "Mismatched URLs");
	}

	css_stylesheet_destroy(s->sheet);
}

void dump_selector_list(css_selector *list, char **ptr)
{
	if (list->combinator != NULL) {
		dump_selector_list(list->combinator, ptr);
	}

	switch (list->data.comb) {
	case CSS_COMBINATOR_NONE:
		break;
	case CSS_COMBINATOR_ANCESTOR:
		(*ptr)[0] = ' ';
		*ptr += 1;
		break;
	case CSS_COMBINATOR_PARENT:
		memcpy(*ptr, " > ", 3);
		*ptr += 3;
		break;
	case CSS_COMBINATOR_SIBLING:
		memcpy(*ptr, " + ", 3);
		*ptr += 3;
		break;
	}

	dump_selector(list, ptr);
}


void dump_selector(css_selector *selector, char **ptr)
{
	css_selector_detail *d = &selector->data;

	while (true) {
		dump_selector_detail(d, ptr);

		if (d->next == 0)
			break;

		d++;
	}
}

void dump_selector_detail(css_selector_detail *detail, char **ptr)
{
	switch (detail->type) {
	case CSS_SELECTOR_ELEMENT:
		if (lwc_string_length(detail->name) == 1 && 
				lwc_string_data(detail->name)[0] == '*' &&
				detail->next == 0) {
			dump_string(detail->name, ptr);
		} else if (lwc_string_length(detail->name) != 1 ||
			   lwc_string_data(detail->name)[0] != '*') {
			dump_string(detail->name, ptr);
		}
		break;
	case CSS_SELECTOR_CLASS:
		**ptr = '.';
		*ptr += 1;
		dump_string(detail->name, ptr);
		break;
	case CSS_SELECTOR_ID:
		**ptr = '#';
		*ptr += 1;
		dump_string(detail->name, ptr);
		break;
	case CSS_SELECTOR_PSEUDO_CLASS:
	case CSS_SELECTOR_PSEUDO_ELEMENT:
		**ptr = ':';
		*ptr += 1;
		dump_string(detail->name, ptr);
		if (detail->value != NULL) {
			**ptr = '(';
			*ptr += 1;
			dump_string(detail->value, ptr);
			**ptr = ')';
			*ptr += 1;
		}
		break;
	case CSS_SELECTOR_ATTRIBUTE:
		**ptr = '[';
		*ptr += 1;
		dump_string(detail->name, ptr);
		**ptr = ']';
		*ptr += 1;
		break;
	case CSS_SELECTOR_ATTRIBUTE_EQUAL:
		**ptr = '[';
		*ptr += 1;
		dump_string(detail->name, ptr);
		(*ptr)[0] = '=';
		(*ptr)[1] = '"';
		*ptr += 2;
		dump_string(detail->value, ptr);
		(*ptr)[0] = '"';
		(*ptr)[1] = ']';
		*ptr += 2;
		break;
	case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
		**ptr = '[';
		*ptr += 1;
		dump_string(detail->name, ptr);
		(*ptr)[0] = '|';
		(*ptr)[1] = '=';
		(*ptr)[2] = '"';
		*ptr += 3;
		dump_string(detail->value, ptr);
		(*ptr)[0] = '"';
		(*ptr)[1] = ']';
		*ptr += 2;
		break;
	case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
		**ptr = '[';
		*ptr += 1;
		dump_string(detail->name, ptr);
		(*ptr)[0] = '~';
		(*ptr)[1] = '=';
		(*ptr)[2] = '"';
		*ptr += 3;
		dump_string(detail->value, ptr);
		(*ptr)[0] = '"';
		(*ptr)[1] = ']';
		*ptr += 2;
		break;
	}
}

void dump_string(lwc_string *string, char **ptr)
{
	*ptr += sprintf(*ptr, "%.*s", (int) lwc_string_length(string), 
			lwc_string_data(string));
}

