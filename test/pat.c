/* pat - generate pattern on stdout */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

#include "getsize.h"

void usage(void);

#define OPTIONS "Sp:s:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"pattern",          required_argument,  0, 'p'},
    {"device-size",      required_argument,  0, 's'},
    {"no-signature",     no_argument,        0, 'S'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

#define SCRUB_MAGIC "\001\002\003SCRUBBED!"

char *prog;

int main(int argc, char *argv[])
{
    extern int optind;
    extern char *optarg;
    int c;
    off_t size = 0;
    int Sopt = 0;
    unsigned char pat = 0;

    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'S':   /* --no-signature */
                Sopt = 1;
                break;
            case 'p':   /* --pattern */
                pat = strtoul(optarg, NULL, 0);
                break;
            case 's':   /* --device-size */
                size = str2size(optarg);
                if (size == 0) {
                    fprintf(stderr, "%s: error parsing size string\n", prog);
                    exit(1);
                }
                break;
            default:
                usage();
        }
    }
    if (size == 0)
        usage();

    if (size < strlen(SCRUB_MAGIC) + 1) {
        fprintf(stderr, "%s: size is too small\n", prog);
        exit(1);
    }
    if (!Sopt) {
        printf("%s", SCRUB_MAGIC);
        size -= strlen(SCRUB_MAGIC);
        putchar(0);
        size--;
    }
    while (size-- > 0)
        putchar(pat);
    exit(0);
}

void usage(void)
{
    fprintf(stderr, "Usage: pat [-S] -s size -p byte\n");
    exit(1);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
