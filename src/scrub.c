/*****************************************************************************\
 *  Copyright (C) 2001-2007 The Regents of the University of California.
 *  Copyright (C) 2007-2014 Lawrence Livermore National Security, LLC.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Jim Garlick <garlick@llnl.gov> UCRL-CODE-2003-006
 *
 *  This file is part of Scrub, a program for erasing disks.
 *  For details, see http://code.google.com/p/diskscrub.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *  See also: http://www.gnu.org/licenses
 *****************************************************************************/

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

#define BUFSIZE (4*1024*1024) /* default blocksize */

struct opt_struct {
    const sequence_t *seq;
    int blocksize;
    off_t devsize;
    char *dirent;
    bool force;
    bool nosig;
    bool remove;
    bool sparse;
    bool nofollow;
    bool nohwrand;
    bool nothreads;
};

static bool       scrub(char *path, off_t size, const sequence_t *seq,
                      int bufsize, bool nosig, bool sparse, bool enospc);
static void       scrub_free(char *path, const struct opt_struct *opt);
static void       scrub_dirent(char *path, const struct opt_struct *opt);
static void       scrub_file(char *path, const struct opt_struct *opt);
#if __APPLE__
static void       scrub_resfork(char *path, const struct opt_struct *opt);
#endif
static void       scrub_disk(char *path, const struct opt_struct *opt);
static int        scrub_object(char *path, const struct opt_struct *opt,
                               bool noexec, bool dryrun);

#define OPTIONS "p:D:Xb:s:fSrvTLRthn"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"pattern",          required_argument,  0, 'p'},
    {"dirent",           required_argument,  0, 'D'},
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
    {"no-threads",       no_argument,        0, 't'},
    {"dry-run",          no_argument,        0, 'n'},
    {"help",             no_argument,        0, 'h'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

char *prog;

static void 
usage(void)
{
    fprintf(stderr,
"Usage: %s [OPTIONS] file [file...]\n"
"  -v, --version           display scrub version and exit\n"
"  -p, --pattern pat       select scrub pattern sequence\n"
"  -b, --blocksize size    set I/O buffer size (default 4m)\n"
"  -s, --device-size size  set device size manually\n"
"  -X, --freespace basedir create temporary dir under basedir, fill with files and scrub until ENOSPC\n"
"  -D, --dirent newname    after scrubbing file, scrub dir entry, rename\n"
"  -f, --force             scrub despite signature from previous scrub\n"
"  -S, --no-signature      do not write scrub signature after scrub\n"
"  -r, --remove            remove file after scrub\n"
"  -L, --no-link           do not scrub link target\n"
"  -R, --no-hwrand         do not use a hardware random number generator\n"
"  -t, --no-threads        do not compute random data in a parallel thread\n"
"  -n, --dry-run           verify file arguments, without writing\n"
"  -h, --help              display this help message\n"
    , prog);

    fprintf(stderr, "Available patterns are:\n");
    seq_list ();
    exit(1);
}

int 
main(int argc, char *argv[])
{
    struct opt_struct opt;
    sequence_t *custom_seq = NULL;
    bool Xopt = false;
    bool nopt = false;
    extern int optind;
    extern char *optarg;
    int c;

    assert(sizeof(off_t) == 8);

    memset (&opt, 0, sizeof (opt));
    opt.blocksize = BUFSIZE;

    /* Handle arguments.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'v':   /* --version */
            printf("scrub version %s\n", VERSION);
            exit(0);
        case 'p':   /* --pattern */
            if (opt.seq != NULL) {
                fprintf(stderr, "%s: only one pattern can be selected\n", prog);
                exit(1);
            }
            if (!strncmp (optarg, "custom=", 7) && strlen (optarg) > 7) {
                if (!(custom_seq = seq_create ("custom", "Custom single-pass",
                                               &optarg[7]))) {
                    fprintf(stderr, "%s: custom sequence: %s\n", prog,
                            strerror(errno));
                    exit(1);
                }
                opt.seq = custom_seq;
            } else {
                opt.seq = seq_lookup(optarg);
                if (!opt.seq) {
                    fprintf(stderr, "%s: no such pattern sequence\n", prog);
                    exit(1);
                }
            }
            break;
        case 'X':   /* --freespace */
            Xopt = true;
            break;
        case 'D':   /* --dirent */
            opt.dirent = optarg;
            break;
        case 'r':   /* --remove */
            opt.remove = true;
            break;
        case 'b':   /* --blocksize */
            opt.blocksize = str2int(optarg);
            if (opt.blocksize == 0) {
                fprintf(stderr, "%s: error parsing blocksize string\n", prog);
                exit(1);
            }
            break;
        case 's':   /* --device-size */
            opt.devsize = str2size(optarg);
            if (opt.devsize == 0) {
                fprintf(stderr, "%s: error parsing size string\n", prog);
                exit(1);
            }
            break;
        case 'f':   /* --force */
            opt.force = true;
            break;
        case 'S':   /* --no-signature */
            opt.nosig = true;
            break;
        case 'T':   /* --test-sparse */
            opt.sparse = true;
            break;
        case 'L':   /* --no-link */
            opt.nofollow = true;
            break;
        case 'R':   /* --no-hwrand */
            opt.nohwrand = true;
            break;
        case 't':   /* --no-threads */
            opt.nothreads = true;
            break;
        case 'n':   /* --dry-run */
            nopt = true;
            break;
        case 'h':   /* --help */
        default:
            usage();
        }
    }
    if (argc == optind)
        usage();
    if (Xopt && argc - optind > 1) {
        fprintf(stderr, "%s: -X only takes one directory name plus pattern\n", prog);
        exit(1);
    }
    if (opt.dirent && argc - optind > 1) {
        fprintf(stderr, "%s: -D can only be used with one file\n", prog);
        exit(1);
    }

    if (!opt.seq)
        opt.seq = seq_lookup("nnsa");
    assert(opt.seq != NULL);
    printf("%s: using %s patterns\n", prog, opt.seq->desc);

    if (opt.nohwrand)
        disable_hwrand();
    if (opt.nothreads)
        disable_threads();

    /* Scrub free space
     * Feb 2015: -X takes a base directory argument which must exist.
     */
    if (Xopt) {
        if (filetype(argv[optind]) == FILE_NOEXIST) {
            fprintf(stderr, "%s: -X base diirectory %s does not exists\n", prog, argv[optind]);
            exit(1);
        }
        if (opt.dirent) {
            fprintf(stderr, "%s: -D and -X cannot be used together\n", prog);
            exit(1);
        }
        if (nopt) {
            printf("%s: (dryrun) scrub free space in %s\n", prog, argv[optind]);
        } else {
            scrub_free(argv[optind], &opt);
        }
    /* Scrub multiple files/devices
     */
    } else if (argc - optind > 1) {
        int i, errcount = 0;
        for (i = optind; i < argc; i++)
            errcount += scrub_object(argv[i], &opt, true, false);
        if (errcount > 0) {
            fprintf (stderr, "%s: no files were scrubbed\n", prog);
            exit(1); 
        }
        for (i = optind; i < argc; i++) {
            if (scrub_object(argv[i], &opt, false, nopt) > 0)
                exit(1);
        }
    /* Scrub single file/device.
     */
    } else {
        if (scrub_object(argv[optind], &opt, false, nopt) > 0)
            exit(1);
    }

    if (custom_seq)
        seq_destroy (custom_seq);
    exit(0);
}

static int scrub_object(char *filename, const struct opt_struct *opt,
                         bool noexec, bool dryrun)
{
    bool havesig = false;
    int errcount = 0;

    switch (filetype(filename)) {
        case FILE_NOEXIST:
            fprintf(stderr, "%s: %s does not exist\n", prog, filename);
            errcount++;
            break;
        case FILE_OTHER:
            fprintf(stderr, "%s: %s is wrong type of file\n", prog, filename);
            errcount++;
            break;
        case FILE_BLOCK:
        case FILE_CHAR:
            if (opt->dirent) {
                fprintf(stderr, "%s: cannot use -D with special file\n", prog);
                errcount++;
            } else if (opt->remove) {
                fprintf(stderr, "%s: cannot use -r with special file\n", prog);
                errcount++;
            } else if (access(filename, R_OK|W_OK) < 0) {
                fprintf(stderr, "%s: no rw access to %s\n", prog, filename);
                errcount++;
            } else if (checksig(filename, &havesig) < 0) {
                fprintf(stderr, "%s: %s: %s\n", prog, filename,
                        strerror(errno));
                errcount++;
            } else if (havesig && !opt->force) {
                fprintf(stderr, "%s: %s already scrubbed? (-f to force)\n",
                        prog, filename);
                errcount++;
            } else if (!noexec) {
                if (dryrun) {
                    printf("%s: (dryrun) scrub special file %s\n",
                            prog, filename);
                } else {
                    scrub_disk(filename, opt);
                }
            }
            break;
        case FILE_LINK:
            if (opt->nofollow) {
                if (opt->remove && !noexec) {
                    if (dryrun) {
                        printf("%s: (dryrun) unlink %s\n", prog, filename);
                    } else {
                        printf("%s: unlinking %s\n", prog, filename);
                        if (unlink(filename) != 0) {
                            fprintf(stderr, "%s: unlink %s: %s\n", prog,
                                    filename, strerror(errno));
                            exit(1);
                        }
                    }
                }
                break;
            }
            /* FALL THRU */
        case FILE_REGULAR:
            if (access(filename, R_OK|W_OK) < 0) {
                fprintf(stderr, "%s: no rw access to %s\n", prog, filename);
                errcount++;
            } else if (checksig(filename, &havesig) < 0) {
                fprintf(stderr, "%s: %s: %s\n", prog, filename,
                        strerror(errno));
                errcount++;
            } else if (havesig && !opt->force) {
                fprintf(stderr, "%s: %s already scrubbed? (-f to force)\n",
                        prog, filename);
                errcount++;
            } else if (opt->dirent && opt->dirent[0] != '/' && filename[0] == '/') {
                fprintf(stderr, "%s: %s should be a full path like %s\n",
                        prog, opt->dirent, filename);
                errcount++;
            } else if (!noexec) {
                if (dryrun) {
                    printf("%s: (dryrun) scrub reg file %s\n", prog, filename);
                } else {
                    scrub_file(filename, opt);
                }
#if __APPLE__
                if (dryrun) {
                    printf("%s: (dryrun) scrub res fork of %s\n",
                           prog, filename);
                } else {
                    scrub_resfork(filename, opt);
                }
#endif
                if (opt->dirent) {
                    if (dryrun) {
                        printf("%s: (dryrun) scrub dirent %s\n",
                               prog, filename);
                    } else {
                        scrub_dirent(filename, opt);
                    }
                }
                if (opt->remove) {
                    char *rmfile = opt->dirent ? opt->dirent : filename;
                    if (dryrun) {
                        printf("%s: (dryrun) unlink %s\n", prog, rmfile);
                    } else {
                        printf("%s: unlinking %s\n", prog, rmfile);
                        if (unlink(rmfile) != 0) {
                            fprintf(stderr, "%s: unlink %s: %s\n", prog, rmfile,
                                    strerror(errno));
                            exit(1);
                        }
                    }
                }
            }
            break;
    }
    return errcount;
}

static int progress_col (const sequence_t *seq)
{
    int i, max = 0, col = 50;
    for (i = 0; i < seq->len; i++)
        if (seq->pat[i].len > max)
            max = seq->pat[i].len;
    if (max > 3)
        col -= (2 * (max - 3));
    return col;
}

/* Scrub 'path', a file/device of size 'size'.
 * Fill using the pattern sequence specified by 'seq'.
 * Use 'bufsize' length for I/O buffers.
 * If 'enospc', return true if first pass ended with ENOSPC error.
 */
static bool
scrub(char *path, off_t size, const sequence_t *seq, int bufsize, 
      bool nosig, bool sparse, bool enospc)
{
    unsigned char *buf;
    int i;
    prog_t p;
    char sizestr[80];
    bool isfull = false;
    off_t written, checked;
    int pcol = progress_col(seq);

    if (!(buf = alloc_buffer(bufsize))) {
        fprintf(stderr, "%s: out of memory\n", prog);
        exit(1);
    }

    size2str(sizestr, sizeof(sizestr), size);
    printf("%s: scrubbing %s %s\n", prog, path, sizestr);

    if (initrand() < 0) {
        fprintf (stderr, "%s: initrand: %s\n", prog, strerror(errno));
        exit(1);
    }
    for (i = 0; i < seq->len; i++) {
        if (i > 0)
            enospc = false;
        switch (seq->pat[i].ptype) {
            case PAT_RANDOM:
                printf("%s: %-8s", prog, "random");
                progress_create(&p, pcol);
                if (churnrand() < 0) {
                    fprintf(stderr, "%s: churnrand: %s\n", prog,
                             strerror(errno));
                    exit(1);
                }
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   (refill_t)genrand, sparse, enospc);
                if (written == (off_t)-1) {
                    fprintf(stderr, "%s: %s: %s\n", prog, path,
                             strerror(errno));
                    exit(1);
                }
                progress_destroy(p);
                break;
            case PAT_NORMAL:
                printf("%s: %-8s", prog, pat2str(seq->pat[i]));
                progress_create(&p, pcol);
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   NULL, sparse, enospc);
                if (written == (off_t)-1) {
                    fprintf(stderr, "%s: %s: %s\n", prog, path,
                             strerror(errno));
                    exit(1);
                }
                progress_destroy(p);
                break;
            case PAT_VERIFY:
                printf("%s: %-8s", prog, pat2str(seq->pat[i]));
                progress_create(&p, pcol);
                memset_pat(buf, seq->pat[i], bufsize);
                written = fillfile(path, size, buf, bufsize, 
                                   (progress_t)progress_update, p, 
                                   NULL, sparse, enospc);
                if (written == (off_t)-1) {
                    fprintf(stderr, "%s: %s: %s\n", prog, path,
                             strerror(errno));
                    exit(1);
                }
                progress_destroy(p);
                printf("%s: %-8s", prog, "verify");
                progress_create(&p, pcol);
                checked = checkfile(path, written, buf, bufsize, 
                                    (progress_t)progress_update, p, sparse);
                if (checked == (off_t)-1) {
                    fprintf(stderr, "%s: %s: %s\n", prog, path,
                             strerror(errno));
                    exit(1);
                }
                if (checked < written) {
                    fprintf(stderr, "%s: %s: verification error\n",
                             prog, path);
                    exit(1);
                }
                progress_destroy(p);
                break;
        }
        if (written < size) {
            assert(i == 0);
            assert(enospc == true);
            isfull = true; 
            size = written;
            if (size == 0) {
                printf("%s: file system is full (0 bytes written)\n", prog);
                break;
            }
        }
    }
    if (!nosig && written > 0) {
        if (writesig(path) < 0) {
            fprintf(stderr, "%s: writing signature to %s: %s\n", prog,
                    path, strerror (errno));
            exit (1);
        }
    }

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
 * with opt->devsize length files (use RLIMIT_FSIZE if no opt->devsize).
 * Feb 2015: scrub_free will now create a subdirectory under *dirpath.
 */
static void
scrub_free(char *dirpath, const struct opt_struct *opt)
{
    char path[MAXPATHLEN];      /* will hold path to free space dir */
    char freespacedir[13];      /* Template Pattern */
    int fileno = 0;
    struct stat sb;
    bool isfull;
    off_t size = opt->devsize;

    /* Chdir to dirpath. Remain here throughout. */
    if (chdir(dirpath) < 0) {
        fprintf(stderr, "%s: chdir %s: %s\n", prog, dirpath, strerror(errno));
        exit(1);
    }
    /* Create temp directory */
    strcpy(freespacedir,"scrub.XXXXXX");
    if (mkdtemp(freespacedir) == NULL) {
        fprintf(stderr, "%s: mkdtemp %s: %s\n", prog, freespacedir, strerror(errno));
        exit(1);
    } 
    fprintf (stderr, "%s: created directory %s/%s\n", prog, dirpath, freespacedir);
    if (stat(freespacedir, &sb) < 0) {
        fprintf(stderr, "%s: stat %s: %s\n", prog, freespacedir, strerror(errno));
        exit(1);
    } 

    if (getuid() == 0)
        set_rlimit_fsize(RLIM_INFINITY);
    if (size == 0)
        size = get_rlimit_fsize();
    if (size == 0)
        size = 1024*1024*1024;
    size = blkalign(size, sb.st_blksize, DOWN);

    /* construct relative path to temp directory in snprintf */
    do {
        snprintf(path, sizeof(path), "%s/scrub.%.3d", freespacedir, fileno++);
        isfull = scrub(path, size, opt->seq, opt->blocksize, opt->nosig,
                       false, true);
    } while (!isfull);
    while (--fileno >= 0) {
        snprintf(path, sizeof(path), "%s/scrub.%.3d", freespacedir, fileno);
        if (unlink(path) < 0)
            fprintf(stderr, "%s: unlink %s: %s\n", prog, path, strerror(errno));
        else
            printf("%s: unlinked %s\n", prog, path);
    }
    if (rmdir(freespacedir) < 0)
        fprintf(stderr, "%s: rmdir %s/%s: %s\n", prog, dirpath, freespacedir, strerror(errno));
    else
        printf("%s: removed %s/%s\n", prog, dirpath, freespacedir);
}

/* Scrub name component of a directory entry through succesive renames.
 */
static void
scrub_dirent(char *path, const struct opt_struct *opt)
{
    char *newpath = opt->dirent;
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
        progress_create(&p, progress_col(seq));
        if (filldentry(path, seq->pat[i].pat[0]) < 0) {/* path: in/out */
            fprintf(stderr, "%s: filldentry: %s\n", prog, strerror(errno));
            exit(1);
        } 
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
scrub_file(char *path, const struct opt_struct *opt)
{
    struct stat sb;
    filetype_t ftype = filetype(path);
    off_t size = opt->devsize;

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
    scrub(path, size, opt->seq, opt->blocksize, opt->nosig, opt->sparse, false);
}

/* Scrub apple resource fork component of file.
 */
#if __APPLE__
static void
scrub_resfork(char *path, const struct opt_struct *opt)
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
    scrub(rpath, rsize, opt->seq, opt->blocksize, false, false, false);
}
#endif

/* Scrub a special file corresponding to a disk.
 */
static void
scrub_disk(char *path, const struct opt_struct *opt)
{
    filetype_t ftype = filetype(path);
    off_t devsize = opt->devsize;

    assert(ftype == FILE_BLOCK || ftype == FILE_CHAR);
    if (devsize == 0) {
        if (getsize(path, &devsize) < 0) {
            fprintf(stderr, "%s: %s: %s\n", prog, path, strerror(errno));
            fprintf(stderr, "%s: could not determine size, use -s\n", prog);
            exit(1);
        }
        printf("%s: please verify that device size below is correct!\n", prog);
    }
    scrub(path, devsize, opt->seq, opt->blocksize, opt->nosig, opt->sparse,
          false);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
