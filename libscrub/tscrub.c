#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "scrub.h"

void
scrub (scrub_ctx_t c, const char *path)
{
	int sig_present;

	if (scrub_path_set (c, path) < 0) {
		fprintf (stderr, "scrub_path_set: %s\n", scrub_strerror (c));
		exit (1);
	}
	if (scrub_check_sig (c, &sig_present) < 0) {
		fprintf (stderr, "%s: %s\n", path, scrub_strerror (c));
		exit (1);
	}
	if (sig_present) {
		printf  ("%s has scrub signature - skipping\n", path);
		return;
	}
	printf ("Scrubbing %s\n", path);
	if (scrub_write (c, NULL, NULL) < 0) {
		fprintf (stderr, "%s: %s\n", path, scrub_strerror (c));
		exit (1);
	}
}

void
list_methods (scrub_ctx_t c)
{
	char **methods;
	int methods_len, i, method;

	if (scrub_methods_get (c, &methods, &methods_len) < 0) {
		fprintf (stderr, "scrub_methods_get: %s\n", scrub_strerror (c));
		exit (1);
	}
	if (scrub_attr_get (c, SCRUB_ATTR_METHOD, &method) < 0) {
		fprintf (stderr, "scrub_attr_get: %s\n", scrub_strerror (c));
		exit (1);
	}
	printf ("Methods:\n");
	for (i = 0; i < methods_len; i++)
		printf ("%c%2d: %s\n", i == method ? '*' : ' ', i, methods[i]);
}

int
main (int argc, char *argv[])
{
	scrub_ctx_t c;
	int i;

	if (scrub_init (&c) < 0) {
		perror ("scrub_init");
		exit (1);
	}

	/* Pretend user selected method 4 from a menu of methods.
	 */
	if (scrub_attr_set (c, SCRUB_ATTR_METHOD, 4) < 0) {
		fprintf (stderr, "scrub_attr_set: %s\n", scrub_strerror (c));
		exit (1);
	}
	list_methods (c);

	for (i = 1; i < argc; i++) {
		scrub (c, argv[i]);
	}
	scrub_fini (c);
	return 0;
}
