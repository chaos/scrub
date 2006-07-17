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

#if defined(linux) || defined(sun)
#define _LARGEFILE_SOURCE 
#define _FILE_OFFSET_BITS 64
#endif

#if defined(_AIX)
#define _LARGE_FILES
#include <sys/mode.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#if !defined(__FreeBSD__) && !defined(sun)
#include <stdint.h>
#endif
#include <libgen.h>
#include <assert.h>
#include <sys/param.h> /* MAXPATHLEN */

#include "genrand.h"
#include "fillfile.h"
#include "filldentry.h"
#include "getsize.h"
#include "progress.h"
#include "util.h"

#define RANDOM	'r' /* 0x72 */
#define VERIFY  'v' /* 0x76 */

static const uint8_t dirent_pattern[] = { 0x55, 0x22, 0x55, 0x22, 0x55, 0x22 };

static const uint8_t old_pattern[] = { 0, 0xff, 0xaa, RANDOM, 0x55, VERIFY };
static const uint8_t fastold_pattern[] = { 0, 0xff, 0xaa, 0x55, VERIFY };
static const uint8_t nnsa_pattern[] = { RANDOM, RANDOM, 0, VERIFY };
static const uint8_t dod_pattern[] = { 0, 0xff, RANDOM, 0, VERIFY };
static const uint8_t bsi_pattern[] = { 0xff, 0xfe, 0xfd, 0xfb, 0xf7, 
                                             0xef, 0xdf, 0xbf, 0x7f };

typedef enum { false, true } bool;
typedef enum { NOEXIST, REGULAR, SPECIAL, OTHER } filetype_t;

static char *pat2str(uint8_t pat);
static void usage(void);
static off_t blkalign(off_t offset, int blocksize);
static filetype_t filetype(char *path);

#define BUFSIZE 8192      /* default blocksize */

static char *prog;

static void 
usage(void)
{
    fprintf(stderr, "Usage: %s [-p dod|nnsa|old|fastold|bsi] [-b blocksize] [-X] [-D newname] file\n", prog);
    fprintf(stderr, "\t-p select scrub patterns (see scrub(1))\n");
    fprintf(stderr, "\t-b overrides default I/O buffer size of %d bytes\n", BUFSIZE);
    fprintf(stderr, "\t-X create file and keep writing until write fails, then scrub\n");
    fprintf(stderr, "\t-D after scrubbing the file, scrub the directory entry and rename\n");

    exit(1);
}

static char *
pat2str(uint8_t pat)
{
    static char str[255];

    if (pat == RANDOM)
        sprintf(str, "random");
    else if (pat == VERIFY)
        sprintf(str, "verify");
    else
        sprintf(str, "0x%x", pat);
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
        /* on Linux, char devices were an afterthought so allow block */
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
scrub(char *path, off_t size, const uint8_t pat[], int npat, int bufsize)
{
    uint8_t *buf;
    int i;
    prog_t p;
    char sizestr[80];

    if (!(buf = (uint8_t *)malloc(bufsize))) {
        fprintf(stderr, "out of memory\n");
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
            memset(buf, pat[i], bufsize);
            fillfile(path, size, buf, bufsize, 
                    (progress_t)progress_update, p, NULL);
        }
        progress_destroy(p);
    }

    free(buf);
}

/* Scrub free space (-X).
 */
static void
scrub_free(char *path, const uint8_t pat[], int npat, int bufsize)
{
    uint8_t *buf;
    off_t size; 

    assert(filetype(path) == NOEXIST || filetype(path) == REGULAR);

    /* special scrub for first pass */
    if (!(buf = (uint8_t *)malloc(bufsize))) {
        fprintf(stderr, "out of memory\n");
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
        scrub(path, size, pat+1, npat-1, bufsize);
}

/* Scrub name component of a directory entry through succesive renames.
 */
static void
scrub_dirent(char *path, char *newpath)
{
    const uint8_t *pat = dirent_pattern;
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
scrub_file(char *path, const uint8_t pat[], int npat, int bufsize)
{
    off_t size; 
    struct stat sb;

    assert(filetype(path) == REGULAR);

    if (stat(path, &sb) < 0) {
        perror(path);
        exit(1);
    }
    size = blkalign(sb.st_size, sb.st_blksize);
    if (size != sb.st_size) {
        printf("%s: padding %s with %d bytes to fill last fs block\n", 
                prog, path, (int)(size - sb.st_size)); 
    }
    scrub(path, size, pat, npat, bufsize);

}

/* Scrub apple resource fork component of file.
 */
#if __APPLE__
static void
scrub_resfork(char *path, const uint8_t pat[], int npat, int bufsize)
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
    scrub(rpath, rsize, pat, npat, bufsize);
}
#endif

/* Scrub a special file corresponding to a disk.
 */
static void
scrub_disk(char *path, off_t size, const uint8_t pat[], int npat, int bufsize)
{
    assert(filetype(path) == SPECIAL);
    if (size == 0) {
        size = getsize(path);
        if (size == 0) {
            fprintf(stderr, "could not determine size, use -s\n");
            exit(1);
        }
        printf("%s: please verify that device size below is correct!\n", prog);
    }
    scrub(path, size, pat, npat, bufsize);
}

int 
main(int argc, char *argv[])
{
    const uint8_t *pat = nnsa_pattern;
    int npat = sizeof(nnsa_pattern)/sizeof(nnsa_pattern[0]);
    bool Xopt = false;
    int bopt = BUFSIZE;
    off_t sopt = 0;
    char *Dopt = NULL;
    char *filename = NULL;

    extern int optind;
    extern char *optarg;
    int c;

    /* Handle arguments.
     */
    prog = basename(argv[0]);
    while ((c = getopt(argc, argv, "p:D:Xb:s:")) != EOF) {
        switch (c) {
        case 'p':   /* Override default pattern with dod|nnsa|old|fastold */
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
        case 'X':   /* fill filesystem */
            Xopt = true;
            break;
        case 'D':   /* scrub name in dirent through successive renames */
            Dopt = optarg;
            break;
        case 'b':   /* override blocksize */
            bopt = strtoul(optarg, NULL, 10);
            break;
        case 's':   /* override size of special file */
            sopt = str2size(optarg);
            if (sopt == 0)
                exit(1);
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
            fprintf(stderr, "-X cannot be used on special files\n");
            exit(1);
        }
        if (sopt > 0) {
            fprintf(stderr, "-s and -X cannot be used together\n");
            exit(1);
        }
        if (Dopt) {
            fprintf(stderr, "-D and -X cannot be used together\n");
            exit(1);
        }
    } else {
        switch (filetype(filename)) {
            case NOEXIST:
                fprintf(stderr, "file does not exist\n");
                exit(1);
                break;
            case OTHER:
                fprintf(stderr, "unsupported file type\n");
                exit(1);
                break;
            case SPECIAL:
                if (Dopt) {
                    fprintf(stderr, "cannot use -D with special file\n");
                    exit(1);
                }
                break;
            case REGULAR:
                if (sopt > 0) {
                    fprintf(stderr, "cannot use -s with regular file\n");
                    exit(1);
                }
                if (Dopt && *Dopt != '/' && *filename == '/') {
                    fprintf(stderr, "please make %s a full path like %s\n",
                            Dopt, filename);
                    exit(1);
                }
                break;
        }
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
        scrub_free(filename, pat, npat, bopt);

    } else if (filetype(filename) == REGULAR) {     /* scrub file */
        scrub_file(filename, pat, npat, bopt);
#if __APPLE__
        scrub_resfork(filename, pat, npat, bopt);
#endif
        if (Dopt) /* XXX destroys 'filename' */
            scrub_dirent(filename, Dopt);

    } else if (filetype(filename) == SPECIAL) {     /* scrub disk */  
        scrub_disk(filename, sopt, pat, npat, bopt);
    }

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
