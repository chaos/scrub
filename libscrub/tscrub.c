#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "scrub.h"

static void
usage(void)
{
    fprintf(stderr,
"Usage: tscrub [OPTIONS] [FILE...]\n"
"  -X dir     create dir+files, fill until ENOSPC, then scrub\n"
"  -p n       select pattern sequence number\n"
    );
    exit(1);
}

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
scrub_free (scrub_ctx_t c, const char *dirpath)
{
    if (scrub_path_set (c, dirpath) < 0) {
        fprintf (stderr, "scrub_path_set: %s\n", scrub_strerror (c));
        exit (1);
    }
    printf ("Scrubbing free space %s\n", dirpath);
    if (scrub_write_free (c, NULL, NULL) < 0) {
        fprintf (stderr, "%s: %s\n", dirpath, scrub_strerror (c));
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
    int f;
    char *dirpath = NULL; /* scrub_free */
    scrub_ctx_t c;
    int i, il;
    int method = 4; /* default method 4 = one random pass */

    while ((f = getopt (argc, argv, "X:p:")) != -1) {
        switch (f) {
            case 'X':
                dirpath = optarg;
                break;
            case 'p':
                method = strtoul (optarg, NULL, 10);
                break;
            default:
                usage();
                exit(1);
        }
    }
    if (dirpath && ((argc - optind) > 0))
        usage();

    if (scrub_init (&c) < 0) {
        perror ("scrub_init");
        exit (1);
    }

    /* select method */
    if (scrub_attr_set (c, SCRUB_ATTR_METHOD, method) < 0) {
        fprintf (stderr, "scrub_attr_set: %s\n", scrub_strerror (c));
        exit (1);
    }
    list_methods (c);

    for (i = optind; i < argc; i++) {
        scrub (c, argv[i]);
    }

    if (dirpath) {
        scrub_free (c, dirpath);
    }

    scrub_fini (c);
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
