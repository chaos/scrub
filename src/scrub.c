/*****************************************************************************\
 *  $Id: scrub.c 78 2006-02-15 01:05:03Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-006.
 *  
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see <http://www.llnl.gov/linux/scrub/>.
 *  
 *  Scrub is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  Scrub is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with Scrub; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

/* Scrub a raw disk or plain file.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#if defined(linux) || defined(sun) || defined(UNIXWARE) || defined(__hpux)
#define _LARGEFILE_SOURCE 
#define _FILE_OFFSET_BITS 64
#endif

#if defined(_AIX)
#define _LARGE_FILES
#include <sys/mode.h>
#endif

#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>
#include <sys/param.h> /* MAXPATHLEN */

#include "genrand.h"
#include "fillfile.h"
#include "filldentry.h"
#include "getsize.h"
#include "progress.h"
#include "util.h"
#include "sig.h"

/* NOTE: default blocksize was 8K in scrub 1.8, however on hpux
 * it was found that raising the default to 1M raised performance from
 * ~1.3 MB/s to 20MB/s. [Graham Smith]
 * Probably it won't hurt on the other OS's.
 */
#define BUFSIZE (1024*1024) /* default blocksize */

typedef enum { false, true } bool;
typedef enum { NOEXIST, REGULAR, SPECIAL, OTHER } filetype_t;

static char      *pat2str(int pat);
static void       usage(void);
static off_t      blkalign(off_t offset, int blocksize);
static filetype_t filetype(char *path);
static char      *pat2str(int pat);
static void       scrub(char *path, off_t size, const int pat[], int npat, 
                      int bufsize, int Sopt);
static void       scrub_free(char *path, const int pat[], int npat, 
                      int bufsize, int Sopt);
static void       scrub_dirent(char *path, char *newpath);
static void       scrub_file(char *path, const int pat[], int npat, 
                      int bufsize, int Sopt);
static void       scrub_resfork(char *path, const int pat[], int npat, 
                      int bufsize);
static void       scrub_disk(char *path, off_t size, const int pat[], int npat,
                      int bufsize, int Sopt);

#define RANDOM  0x0100
#define VERIFY  0x0200

static const int dirent_pattern[] = { 0x55, 0x22, 0x55, 0x22, 0x55, 0x22 };

static const int old_pattern[] = { 0, 0xff, 0xaa, RANDOM, 0x55, VERIFY };
static const int fastold_pattern[] = { 0, 0xff, 0xaa, 0x55, VERIFY };
static const int nnsa_pattern[] = { RANDOM, RANDOM, 0, VERIFY };
static const int dod_pattern[] = { 0, 0xff, RANDOM, 0, VERIFY };
static const int bsi_pattern[] = { 0xff, 0xfe, 0xfd, 0xfb, 0xf7,
                                   0xef, 0xdf, 0xbf, 0x7f };

#define OPTIONS "p:D:Xb:s:fSrv"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"pattern",          required_argument,  0, 'p'},
    {"dirent",           no_argument,        0, 'D'},
    {"freespace",        no_argument,        0, 'X'},
    {"blocksize",        required_argument,  0, 'b'},
    {"device-size",      required_argument,  0, 's'},
    {"force",            no_argument,        0, 'f'},
    {"no-signature",     no_argument,        0, 'S'},
    {"remove",           no_argument,        0, 'r'},
    {"version",          no_argument,        0, 'v'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif


char *prog;

int 
main(int argc, char *argv[])
{
    const int *pat = nnsa_pattern;
    int npat = sizeof(nnsa_pattern)/sizeof(nnsa_pattern[0]);
    bool Xopt = false;
    int bopt = BUFSIZE;
    off_t sopt = 0;
    char *Dopt = NULL;
    char *filename = NULL;
    int fopt = 0;
    int Sopt = 0;
    int ropt = 0;

    extern int optind;
    extern char *optarg;
    int c;

    /* Handle arguments.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'v':   /* --version */
            printf("scrub version %s\n", VERSION);
            exit(0);
        case 'p':   /* --pattern */
            if (!strcmp(optarg, "dod") || !strcmp(optarg, "DOD")) {
                pat = dod_pattern;
                npat = sizeof(dod_pattern)/sizeof(dod_pattern[0]);
            } else if (!strcmp(optarg, "nnsa") || !strcmp(optarg, "NNSA")) {
                pat = nnsa_pattern;
                npat = sizeof(nnsa_pattern)/sizeof(nnsa_pattern[0]);
            } else if (!strcmp(optarg, "old") || !strcmp(optarg, "OLD")) {
                pat = old_pattern;
                npat = sizeof(old_pattern)/sizeof(old_pattern[0]);
            } else if (!strcmp(optarg, "fastold") || !strcmp(optarg, "FASTOLD")) {
                pat = fastold_pattern;
                npat = sizeof(fastold_pattern)/sizeof(fastold_pattern[0]);
            } else if (!strcmp(optarg, "bsi") || !strcmp(optarg, "BSI")) {
                pat = bsi_pattern;
                npat = sizeof(bsi_pattern)/sizeof(bsi_pattern[0]);
            } else 
                usage();
            break;
        case 'X':   /* --freespace */
            Xopt = true;
            break;
        case 'D':   /* --dirent */
            Dopt = optarg;
            break;
        case 'r':   /* --remove */
            ropt = 1;
            break;
        case 'b':   /* --blocksize */
            bopt = str2int(optarg);
            if (bopt == 0) {
                fprintf(stderr, "%s: error parsing blocksize string\n", prog);
                exit(1);
            }
            break;
        case 's':   /* --device-size */
            sopt = str2size(optarg);
            if (sopt == 0) {
                fprintf(stderr, "%s: error parsing size string\n", prog);
                exit(1);
            }
            break;
        case 'f':   /* --force */
            fopt = 1;
            break;
        case 'S':   /* --no-signature */
            Sopt = 1;
            break;
        default:
            usage();
        }
    }
    if (argc - optind != 1)
        usage();
    filename = argv[optind++];

    /* Verify arguments.
     */
    if (filename == NULL)
        usage();
    if (Xopt) {
        if (filetype(filename) == SPECIAL) {
            fprintf(stderr, "%s: -X cannot be used on special files\n", prog);
            exit(1);
        }
        if (sopt > 0) {
            fprintf(stderr, "%s: -s and -X cannot be used together\n", prog);
            exit(1);
        }
        if (Dopt) {
            fprintf(stderr, "%s: -D and -X cannot be used together\n", prog);
            exit(1);
        }
    } else {
        switch (filetype(filename)) {
            case NOEXIST:
                fprintf(stderr, "%s: %s does not exist\n", prog, filename);
                exit(1);
                break;
            case OTHER:
                fprintf(stderr, "%s: %s is not a reg file or %sdisk device\n", 
                    prog, filename,
#if defined(linux)
                    ""
#else
                    "raw "
#endif
                    );
                exit(1);
                break;
            case SPECIAL:
                if (Dopt) {
                    fprintf(stderr, "%s: cannot use -D with special file\n", prog);
                    exit(1);
                }
                if (ropt) {
                    fprintf(stderr, "%s: cannot use -r with special file\n", prog);
                    exit(1);
                }
                break;
            case REGULAR:
                if (sopt > 0) {
                    fprintf(stderr, "%s: cannot use -s with regular file\n", prog);
                    exit(1);
                }
                if (Dopt && *Dopt != '/' && *filename == '/') {
                    fprintf(stderr, "%s: %s should be a full path like %s\n",
                            prog, Dopt, filename);
                    exit(1);
                }
                break;
        }
        if (access(filename, R_OK|W_OK) < 0) {
            fprintf(stderr, "%s: no permission to scrub %s\n", prog, filename);
            exit(1);
        }
        if (checksig(filename, bopt) && !fopt) {
            fprintf(stderr, "%s: %s has already been scrubbed? (use -f to force)\n",
                        prog, filename);
            exit(1);
        }
    }


    if (sizeof(off_t) < 8) {
        fprintf(stderr, "%s: warning: not using 64 bit file offsets\n", prog);
    }

    /* Scrub.
     */
    printf("%s: using %s patterns\n", prog, 
            (pat == dod_pattern)        ? "DoD 5220.22-M" 
            : (pat == nnsa_pattern)     ? "NNSA NAP-14.x" 
            : (pat == old_pattern)      ? "pre v1.7 scrub" 
            : (pat == fastold_pattern)  ? "pre v1.7 scrub (skip random)" 
            : (pat == bsi_pattern)      ? "BSI pattern" 
            : "unknown pattern");

    if (Xopt) {                                     /* scrub free */
        scrub_free(filename, pat, npat, bopt, Sopt);

    } else if (filetype(filename) == REGULAR) {     /* scrub file */
        scrub_file(filename, pat, npat, bopt, Sopt);
#if __APPLE__
        scrub_resfork(filename, pat, npat, bopt);
#endif
        if (Dopt) { /* XXX destroys 'filename' */
            scrub_dirent(filename, Dopt);
            filename = Dopt; /* -r needs this below */
        }

    } else if (filetype(filename) == SPECIAL) {     /* scrub disk */  
        scrub_disk(filename, sopt, pat, npat, bopt, Sopt);
    }

    /* unlink file at the end */
    if (ropt && filetype(filename) == REGULAR) {
        printf("%s: unlinking %s\n", prog, filename);
        if (unlink(filename) != 0) {
            perror(filename);
            exit(1);
        }
    }

    exit(0);
}

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s [-v] [-f] [-p pat] [-b blocksize] [-X] [-D newname] [-r] file\n", prog);
    fprintf(stderr, "\t-p select scrub patterns (see scrub(1))\n");
    fprintf(stderr, "\t-b overrides default I/O buffer size of %d bytes\n", BUFSIZE);
    fprintf(stderr, "\t-X create file and keep writing until write fails, then scrub\n");
    fprintf(stderr, "\t-D after scrubbing the file, scrub the directory entry and rename\n");
    fprintf(stderr, "\t-f scrub even if file has signature from previous scrub\n");
    fprintf(stderr, "\t-r remove file after scrub\n");
    fprintf(stderr, "\t-v display scrub version and exit\n");

    exit(1);
}

static char *
pat2str(int pat)
{
    static char str[255];

    if (pat == RANDOM)
        snprintf(str, sizeof(str), "random");
    else if (pat == VERIFY)
        snprintf(str, sizeof(str), "verify");
    else
        snprintf(str, sizeof(str), "0x%x", pat);
    return str;
}

/* Round 'offset' up to an even multiple of 'blocksize'.
 */
static off_t
blkalign(off_t offset, int blocksize)
{
    off_t r = offset % blocksize;

    if (r > 0) 
        offset += (blocksize - r);

    return offset;
}

/* Return the type of file represented by 'path'.
 */
static filetype_t
filetype(char *path)
{
    struct stat sb;
    filetype_t res = NOEXIST;

    if (stat(path, &sb) == 0) {
        if (S_ISREG(sb.st_mode))
            res = REGULAR;
#if defined(linux)
        /* on modern Linux, disks only have a block interface */
        else if (S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode))
#else
        else if (S_ISCHR(sb.st_mode))
#endif
            res = SPECIAL;
        else
            res = OTHER;
    }
    return res;
}

/* Scrub 'path', a file/device of size 'size'.
 * Fill with the bytes in 'pat', an array of 'npat' elements.
 * Use 'bufsize' length for I/O buffers.
 */
static void
scrub(char *path, off_t size, const int pat[], int npat, int bufsize, int Sopt)
{
    unsigned char *buf;
    int i;
    prog_t p;
    char sizestr[80];

    if (!(buf = malloc(bufsize))) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }

    size2str(sizestr, sizeof(sizestr), size);
    printf("%s: scrubbing %s %s\n", prog, path, sizestr);

    initrand();
    for (i = 0; i < npat; i++) {
        printf("%s: %-8s", prog, pat2str(pat[i]));
        progress_create(&p, 50);
        if (pat[i] == RANDOM) {
            churnrand();
            fillfile(path, size, buf, bufsize, 
                    (progress_t)progress_update, p, (refill_t)genrand);
        } else if (pat[i] == VERIFY) {
            assert(i > 0);
            assert(pat[i - 1] != RANDOM);
            assert(pat[i - 1] != VERIFY);
            /* depends on previous pass contents of buf being intact */
            checkfile(path, size, buf, bufsize, 
                    (progress_t)progress_update, p);
        } else {
            assert(pat[i] <= 0xff);
            memset(buf, pat[i], bufsize);
            fillfile(path, size, buf, bufsize, 
                    (progress_t)progress_update, p, NULL);
        }
        progress_destroy(p);
    }
    if (!Sopt)
        writesig(path, bufsize);

    free(buf);
}

/* Scrub free space (-X).
 */
static void
scrub_free(char *path, const int pat[], int npat, int bufsize, int Sopt)
{
    unsigned char *buf;
    off_t size; 

    assert(filetype(path) == NOEXIST || filetype(path) == REGULAR);

    /* special scrub for first pass */
    if (!(buf = malloc(bufsize))) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }
    assert(npat > 0);
    assert(pat[0] != VERIFY);
    printf("%s: filling file system by expanding %s\n", prog, path);
    printf("%s: %-8s...\n", prog, pat2str(pat[0]));
    fflush(stdout);
    /* XXX add some feedback */
    if (pat[0] == RANDOM) {
        churnrand();
        size = growfile(path, buf, bufsize, (refill_t)genrand);
    } else {
        memset(buf, pat[0], bufsize);
        size = growfile(path, buf, bufsize, NULL);
    }
    free(buf);

    /* remaining passes as usual */
    if (npat > 1)
        scrub(path, size, pat+1, npat-1, bufsize, Sopt);
}

/* Scrub name component of a directory entry through succesive renames.
 */
static void
scrub_dirent(char *path, char *newpath)
{
    const int *pat = dirent_pattern;
    int npat = sizeof(dirent_pattern)/sizeof(dirent_pattern[0]);
    prog_t p;
    int i;

    assert(filetype(path) == REGULAR);

    printf("%s: scrubbing directory entry\n", prog);

    for (i = 0; i < npat; i++) {
        assert(pat[i] != 0);
        assert(pat[i] != RANDOM);
        assert(pat[i] != VERIFY);
        printf("%s: %-8s", prog, pat2str(pat[i]));
        progress_create(&p, 50);
        filldentry(path, pat[i]); /* path: in/out */
        progress_update(p, 1.0);
        progress_destroy(p);
    }
    if (rename(path, newpath) < 0) {
        fprintf(stderr, "%s: error renaming %s to %s\n", prog, path, newpath);
        exit(1);
    }
}

/* Scrub a regular file.
 */
static void 
scrub_file(char *path, const int pat[], int npat, int bufsize, int Sopt)
{
    off_t size; 
    struct stat sb;

    assert(filetype(path) == REGULAR);

    if (stat(path, &sb) < 0) {
        fprintf(stderr, "%s: stat ", prog);
        perror(path);
        exit(1);
    }
    if (sb.st_size == 0) {
        fprintf(stderr, "%s: %s is zero length\n", prog, path);
        exit(1);
    }
    size = blkalign(sb.st_size, sb.st_blksize);
    if (size != sb.st_size) {
        printf("%s: padding %s with %d bytes to fill last fs block\n", 
                prog, path, (int)(size - sb.st_size)); 
    }
    scrub(path, size, pat, npat, bufsize, Sopt);
}

/* Scrub apple resource fork component of file.
 */
#if __APPLE__
static void
scrub_resfork(char *path, const int pat[], int npat, int bufsize)
{
    struct stat rsb;
    char rpath[MAXPATHLEN];
    off_t rsize; 

    assert(filetype(path) == REGULAR);
    (void)snprintf(rpath, sizeof(rpath), "%s/..namedfork/rsrc", path);
    if (stat(rpath, &rsb) < 0)
        return;
    if (rsb.st_size == 0) {
        printf("%s: skipping zero length resource fork: %s\n", prog, rpath);
        return;
    }
    printf("%s: scrubbing resource fork: %s\n", prog, rpath);
    rsize = blkalign(rsb.st_size, rsb.st_blksize);
    if (rsize != rsb.st_size) {
        printf("%s: padding %s with %d bytes to fill last fs block\n", 
                        prog, rpath, (int)(rsize - rsb.st_size)); 
    }
    scrub(rpath, rsize, pat, npat, bufsize, 0);
}
#endif

/* Scrub a special file corresponding to a disk.
 */
static void
scrub_disk(char *path, off_t size, const int pat[], int npat, int bufsize, int Sopt)
{
    assert(filetype(path) == SPECIAL);
    if (size == 0) {
        size = getsize(path);
        if (size == 0) {
            fprintf(stderr, "%s: could not determine size, use -s\n", prog);
            exit(1);
        }
        printf("%s: please verify that device size below is correct!\n", prog);
    }
    scrub(path, size, pat, npat, bufsize, Sopt);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
