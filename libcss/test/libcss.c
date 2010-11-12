#include <stdio.h>
#include <stdlib.h>

#include <libcss/libcss.h>

#include "testutils.h"

static void *myrealloc(void *ptr, size_t len, void *pw)
{
	UNUSED(pw);

	return realloc(ptr, len);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	assert(css_initialise(argv[1], myrealloc, NULL) == CSS_OK);

	assert (css_finalise(myrealloc, NULL) == CSS_OK);

	printf("PASS\n");

	return 0;
}
