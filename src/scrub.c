/*****************************************************************************\
 *  $Id: scrub.c 78 2006-02-15 01:05:03Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov>.
 *  UCRL-CODE-2003-006.
 *  
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see https://code.google.com/p/diskscrub/
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
#if HAVE_GETOPT_H
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
#include <sys/resource.h>
#include <errno.h>

#include "util.h"
#include "genrand.h"
#include "fillfile.h"
#include "filldentry.h"
#include "getsize.h"
#include "progress.h"
#include "sig.h"
#include "pattern.h"

/* NOTE: default blocksize was 8K in scrub 1.8, however on hpux
 * it was found that raising the default to 1M raised performance from
 * ~1.3 MB/s to 20MB/s. [Graham Smith]
 */
#define BUFSIZE (1024*1024) /* default blocksize */

static void       usage(void);
static bool       scrub(char *path, off_t size, const sequence_t *seq,
                      int bufsize, bool Sopt, bool sparse, bool enospc);
static void       scrub_free(char *path, off_t size, const sequence_t *seq,
                      int bufsize, bool Sopt);
static void       scrub_dirent(char *path, char *newpath);
static void       scrub_file(char *path, off_t size, const sequence_t *seq,
                      int bufsize, bool Sopt, bool sparse);
#if __APPLE__
static void       scrub_resfork(char *path, const sequence_t *seq,
                      int bufsize);
#endif
static void       scrub_disk(char *path, off_t size, const sequence_t *seq,
                      int bufsize, bool Sopt, bool sparse);

#define OPTIONS "p:D:Xb:s:fSrvTLRh"
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
    {"test-sparse",      no_argument,        0, 'T'},
    {"no-link",          no_argument,        0, 'L'},
    {"no-hwrand",        no_argument,        0, 'R'},
    {"help",             no_argument,        0, 'h'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

char *prog;

int 
main(int argc, char *argv[])
{
    const sequence_t *seq = NULL;
    bool Xopt = false;
    int bopt = BUFSIZE;
    off_t sopt = 0;
    char *Dopt = NULL;
    char *filename = NULL;
    bool fopt = false;
    bool Sopt = false;
    bool ropt = false;
    bool Topt = false;
    bool Lopt = false;
    bool Ropt = false;
    extern int optind;
    extern char *optarg;
    int c;

    assert(sizeof(off_t) == 8);

    /* Handle arguments.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'v':   /* --version */
            printf("scrub version %s\n", VERSION);
            exit(0);
        case 'p':   /* --pattern */
            seq = seq_lookup(optarg);
            if (!seq) {
                fprintf(stderr, "%s: no such pattern sequence\n", prog);
                exit(1);
            }
            break;
        case 'X':   /* --freespace */
            Xopt = true;
            break;
        case 'D':   /* --dirent */
            Dopt = optarg;
            break;
        case 'r':   /* --remove */
            ropt = true;
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
            fopt = true;
            break;
        case 'S':   /* --no-signature */
            Sopt = true;
            break;
        case 'T':   /* --test-sparse */
            Topt = true;
            break;
        case 'L':   /* --no-link */
            Lopt = true;
            break;
        case 'R':   /* --no-hwrand */
            Ropt = true;
            break;
        case 'h':   /* --help */
        default:
            usage();
        }
    }
    if (argc - optind != 1)
        usage();
    filename = argv[optind];

    if (!seq)
        seq = seq_lookup("nnsa");
    assert(seq != NULL);
    printf("%s: using %s patterns\n", prog, seq->desc);

    if (Ropt)
        disable_hwrand();

    /* Handle -X specially.
     */
    if (Xopt) {
        if (filetype(filename) != FILE_NOEXIST) {
            fprintf(stderr, "%s: -X argument cannot exist\n", prog);
            exit(1);
        }
        if (Dopt) {
            fprintf(stderr, "%s: -D and -X cannot be used together\n", prog);
            exit(1);
        }
        scrub_free(filename, sopt, seq, bopt, Sopt);
        exit(0);
    } 

    switch (filetype(filename)) {
        case FILE_NOEXIST:
            fprintf(stderr, "%s: %s does not exist\n", prog, filename);
            exit(1);
        case FILE_OTHER:
            fprintf(stderr, "%s: %s is wrong type of file\n", prog, filename);
            exit(1);
        case FILE_BLOCK:
        case FILE_CHAR:
            if (Dopt) {
                fprintf(stderr, "%s: cannot use -D with special file\n", prog);
                exit(1);
            }
            if (ropt) {
                fprintf(stderr, "%s: cannot use -r with special file\n", prog);
                exit(1);
            }
            if (access(filename, R_OK|W_OK) < 0) {
                fprintf(stderr, "%s: no rw access to %s\n", prog, filename);
                exit(1);
            }
            if (checksig(filename) && !fopt) {
                fprintf(stderr, "%s: %s already scrubbed? (-f to force)\n",
                        prog, filename);
                exit(1);
            }
            scrub_disk(filename, sopt, seq, bopt, Sopt, Topt);
            break;
        case FILE_LINK:
            if (Lopt) {
                if (ropt) {
                    printf("%s: unlinking %s\n", prog, filename);
                    if (unlink(filename) != 0) {
                        fprintf(stderr, "%s: unlink %s: %s\n", prog, filename,
                            strerror(errno));
                        exit(1);
                    }
                }
                break;
            }
        case FILE_REGULAR:
            if (access(filename, R_OK|W_OK) < 0) {
                fprintf(stderr, "%s: no rw access to %s\n", prog, filename);
                exit(1);
            }
            if (checksig(filename) && !fopt) {
                fprintf(stderr, "%s: %s already scrubbed? (-f to force)\n",
                        prog, filename);
                exit(1);
            }
            if (Dopt && *Dopt != '/' && *filename == '/') {
                fprintf(stderr, "%s: %s should be a full path like %s\n",
                        prog, Dopt, filename);
                exit(1);
            }
            scrub_file(filename, sopt, seq, bopt, Sopt, Topt);
#if __APPLE__
            scrub_resfork(filename, seq, bopt);
#endif
            if (Dopt) {
                scrub_dirent(filename, Dopt);
                filename = Dopt; /* -r needs this below */
            }
            if (ropt) {
                printf("%s: unlinking %s\n", prog, filename);
                if (unlink(filename) != 0) {
                    fprintf(stderr, "%s: unlink %s: %s\n", prog, filename,
                            strerror(errno));
                    exit(1);
                }
            }
            break;
    }
    exit(0);
}

static void 
usage(void)
{
    fprintf(stderr,
"Usage: %s [OPTIONS] file\n"
"  -v, --version           display scrub version and exit\n"
"  -p, --pattern pat       select scrub pattern sequence\n"
"  -b, --blocksize size    set I/O buffer size (default 1m)\n"
"  -s, --device-size size  set device size manually\n"
"  -X, --freespace         create dir+files, fill until ENOSPC, then scrub\n"
"  -D, --dirent newname    after scrubbing file, scrub dir entry, rename\n"
"  -f, --force             scrub despite signature from previous scrub\n"
"  -S, --no-signature      do not write scrub signature after scrub\n"
"  -r, --remove            remove file after scrub\n"
"  -L, --no-link           do not scrub link target\n"
"  -R, --no-hwrand         do not use a hardware random number generator\n"
"  -h, --help              display this help message\n"
    , prog);

    fprintf(stderr, "Available patterns are:\n");
    seq_list ();
    exit(1);
}

/* Scrub 'path', a file/device of size 'size'.
 * Fill using the pattern sequence specified by 'seq'.
 * Use 'bufsize' length for I/O buffers.
 * If 'enospc', return true if first pass ended with ENOSPC error.
 */
static bool
scrub(char *path, off_t size, const sequence_t *seq, int bufsize, 
      bool Sopt, bool sparse, bool enospc)
{
    unsigned char *buf;
    int i;
    prog_t p;
    char sizestr[80];
    bool isfull = false;
    off_t written;

    if (!(buf = alloc_buffer(bufsize))) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }

    size2str(sizestr, sizeof(sizestr), size);
    printf("%s: scrubbing %s %s\n", prog, path, sizestr);

    initrand();
    for (i = 0; i < seq->len; i++) {
        if (i > 0)
            enospc = false;
        switch (seq->pat[i].ptype) {
            case PAT_RANDOM:
                printf("%s: %-8s", prog, "random");
                progress_create(&p, 50);
                churnrand();
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   (refill_t)genrand, sparse, enospc);
                progress_destroy(p);
                break;
            case PAT_NORMAL:
                printf("%s: %-8s", prog, pat2str(seq->pat[i]));
                progress_create(&p, 50);
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   NULL, sparse, enospc);
                progress_destroy(p);
                break;
            case PAT_VERIFY:
                printf("%s: %-8s", prog, pat2str(seq->pat[i]));
                progress_create(&p, 50);
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   NULL, sparse, enospc);
                progress_destroy(p);
                printf("%s: %-8s", prog, "verify");
                progress_create(&p, 50);
                checkfile(path, written, buf, bufsize, 
                          (progress_t)progress_update, p, sparse);
                progress_destroy(p);
                break;
        }
        if (written < size) {
            assert(i == 0);
            assert(enospc == true);
            isfull = true; 
            size = written;
        }
    }
    if (!Sopt)
        writesig(path);

    free(buf);
    return isfull;
}

static off_t
get_rlimit_fsize(void)
{
    struct rlimit r;

    if (getrlimit(RLIMIT_FSIZE, &r) < 0) {
        fprintf(stderr, "%s: getrlimit: %s\n", prog, strerror(errno));
        exit(1);
    }
    if (r.rlim_cur == RLIM_INFINITY)
        return 0;
    return r.rlim_cur;
}

static void
set_rlimit_fsize(off_t val)
{
    struct rlimit r;

    r.rlim_cur = r.rlim_max = val;
    if (setrlimit(RLIMIT_FSIZE, &r) < 0) {
        fprintf(stderr, "%s: setrlimit: %s\n", prog, strerror(errno));
        exit(1);
    }
}

/* Scrub free space (-X) by creating a directory, then filling it
 * with 'size' length files (use RLIMIT_FSIZE if no -s).
 */
static void
scrub_free(char *dirpath, off_t size, const sequence_t *seq, 
           int bufsize, bool Sopt)
{
    char path[MAXPATHLEN];
    int fileno = 0;
    struct stat sb;
    bool isfull;

    if (mkdir(dirpath, 0755) < 0) {
        fprintf(stderr, "%s: mkdir %s: %s\n", prog, path, strerror(errno));
        exit(1);
    } 
    if (stat(dirpath, &sb) < 0) {
        fprintf(stderr, "%s: stat %s: %s\n", prog, path, strerror(errno));
        exit(1);
    } 
    if (getuid() == 0)
        set_rlimit_fsize(RLIM_INFINITY);
    if (size == 0)
        size = get_rlimit_fsize();
    if (size == 0)
        size = 1024*1024*1024;
    size = blkalign(size, sb.st_blksize, DOWN);

    do {
        snprintf(path, sizeof(path), "%s/scrub.%.3d", dirpath, fileno++);
        isfull = scrub(path, size, seq, bufsize, Sopt, false, true);
    } while (!isfull);
}

/* Scrub name component of a directory entry through succesive renames.
 */
static void
scrub_dirent(char *path, char *newpath)
{
    const sequence_t *seq = seq_lookup("dirent");
    prog_t p;
    int i;
    filetype_t ftype = filetype(path);

    assert(seq != NULL);
    assert(ftype == FILE_REGULAR);

    printf("%s: scrubbing directory entry\n", prog);

    for (i = 0; i < seq->len; i++) {
        assert(seq->pat[i].ptype == PAT_NORMAL);
        assert(seq->pat[i].len == 1);
        printf("%s: %-8s", prog, pat2str(seq->pat[i]));
        progress_create(&p, 50);
        filldentry(path, seq->pat[i].pat[0]); /* path: in/out */
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
scrub_file(char *path, off_t size, const sequence_t *seq, 
           int bufsize, bool Sopt, bool sparse)
{
    struct stat sb;
    filetype_t ftype = filetype(path);

    assert(ftype == FILE_REGULAR || ftype == FILE_LINK);

    if (stat(path, &sb) < 0) {
        fprintf(stderr, "%s: stat %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (size > 0) {
        if (blkalign(sb.st_size, sb.st_blksize, UP) > size)
            fprintf(stderr, "%s: warning: -s size < file size\n", prog);
    } else  {
        if (sb.st_size == 0) {
            fprintf(stderr, "%s: warning: %s is zero length\n", prog, path);
            return;
        }
        size = blkalign(sb.st_size, sb.st_blksize, UP);
        if (size != sb.st_size) {
            printf("%s: padding %s with %d bytes to fill last fs block\n", 
                    prog, path, (int)(size - sb.st_size)); 
        }
    }
    scrub(path, size, seq, bufsize, Sopt, sparse, false);
}

/* Scrub apple resource fork component of file.
 */
#if __APPLE__
static void
scrub_resfork(char *path, const sequence_t *seq, int bufsize)
{
    struct stat rsb;
    char rpath[MAXPATHLEN];
    off_t rsize; 
    filetype_t ftype = filetype(path);

    assert(ftype == FILE_REGULAR);
    (void)snprintf(rpath, sizeof(rpath), "%s/..namedfork/rsrc", path);
    if (stat(rpath, &rsb) < 0)
        return;
    if (rsb.st_size == 0) {
        /*printf("%s: skipping zero length resource fork: %s\n", prog, rpath);*/
        return;
    }
    printf("%s: scrubbing resource fork: %s\n", prog, rpath);
    rsize = blkalign(rsb.st_size, rsb.st_blksize, UP);
    if (rsize != rsb.st_size) {
        printf("%s: padding %s with %d bytes to fill last fs block\n", 
                        prog, rpath, (int)(rsize - rsb.st_size)); 
    }
    scrub(rpath, rsize, seq, bufsize, false, false, false);
}
#endif

/* Scrub a special file corresponding to a disk.
 */
static void
scrub_disk(char *path, off_t size, const sequence_t *seq, int bufsize, 
           bool Sopt, bool sparse)
{
    filetype_t ftype = filetype(path);

    assert(ftype == FILE_BLOCK || ftype == FILE_CHAR);
    if (size == 0) {
        size = getsize(path);
        if (size == 0) {
            fprintf(stderr, "%s: could not determine size, use -s\n", prog);
            exit(1);
        }
        printf("%s: please verify that device size below is correct!\n", prog);
    }
    scrub(path, size, seq, bufsize, Sopt, sparse, false);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
