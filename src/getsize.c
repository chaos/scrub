/*****************************************************************************\
 *  $Id: getsize.c 76 2006-02-15 00:49:19Z garlick $
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include "getsize.h"

extern char *prog;

#if defined(linux)
/* scrub-1.7 tested linux 2.6.11-1.1369_FC4 */
/* scrub-1.8 tested Fedora Core 5 */
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <sys/utsname.h>
typedef unsigned long long u64; /* for BLKGETSIZE64 (slackware) */

off_t 
getsize(char *path)
{
    struct utsname ut;
    unsigned long long numbytes;
    int valid_blkgetsize64 = 1;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }

    /* Ref: e2fsprogs-1.39 - apparently BLKGETSIZE64 doesn't work pre 2.6 */
    if ((uname(&ut) == 0) &&
            ((ut.release[0] == '2') && (ut.release[1] == '.') &&
             (ut.release[2] < '6') && (ut.release[3] == '.')))
                valid_blkgetsize64 = 0;
    if (valid_blkgetsize64) {
        if (ioctl(fd, BLKGETSIZE64, &numbytes) < 0) {
            fprintf(stderr, "%s: ioctl BLKGETSIZE64 %s: %s\n", prog, path,
                    strerror(errno));
            exit(1);
        }
    } else {
        unsigned long numblocks;

        if (ioctl(fd, BLKGETSIZE, &numblocks) < 0) {
            fprintf(stderr, "%s: ioctl BLKGETSIZE %s: %s\n", prog, path,
                    strerror(errno));
            exit(1);
        }
        numbytes = (off_t)numblocks*512; /* 2TB limit here */
    }

    (void)close(fd);

    return (off_t)numbytes;
}

#elif defined(__FreeBSD__)
/* scrub-1.7 tested freebsd 5.3-RELEASE-p5 */
#include <sys/ioctl.h>
#include <sys/disk.h>

off_t 
getsize(char *path)
{
    off_t numbytes;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (ioctl(fd, DIOCGMEDIASIZE, &numbytes) < 0) {
        fprintf(stderr, "%s: ioctl DIOCGMEDIASIZE %s: %s\n", prog, path,
                strerror(errno));
        exit(1);
    }
    (void)close(fd);

    return numbytes;
}

#elif defined(sun)
/* scrub-1.7 tested solaris 5.9 */
#include <sys/ioctl.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>

off_t 
getsize(char *path)
{
    struct dk_minfo dkmp;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (ioctl(fd, DKIOCGMEDIAINFO, &dkmp) < 0) {
        fprintf(stderr, "%s: ioctl DKIOCGMEDIAINFO %s: %s\n", prog, path,
                strerror(errno));
        exit(1);
    }
    (void)close(fd);

    return (off_t)dkmp.dki_capacity * dkmp.dki_lbsize;
}

#elif defined(__APPLE__)
/* scrub-1.7 tested OS X 7.9.0 */
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/disk.h>

off_t 
getsize(char *path)
{
    uint32_t blocksize;
    uint64_t blockcount;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (ioctl(fd, DKIOCGETBLOCKSIZE, &blocksize) < 0) {
        fprintf(stderr, "%s: ioctl DKIOCGETBLOCKSIZE %s: %s\n", prog, path,
                strerror(errno));
        exit(1);
    }
    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &blockcount) < 0) {
        fprintf(stderr, "%s: ioctl DKIOGETBLOCKCOUNT %s: %s\n", prog, path,
                strerror(errno));
        exit(1);
    }
    (void)close(fd);

    return (off_t)blockcount * blocksize;
}
#elif defined(_AIX)
/* scrub-1.7 tested AIX 5.1 and 5.3 */
/* scrub-1.8 tested AIX 5.2 */
#include <sys/ioctl.h>
#include <sys/devinfo.h>

off_t 
getsize(char *path)
{
    int fd;
    struct devinfo devinfo;
    off_t size;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (ioctl(fd, IOCINFO, &devinfo) == -1) {
        fprintf(stderr, "%s: ioctl IOCINFO %s: %s\n", prog, path, 
                strerror(errno));
        exit(1);
    }
    switch (devinfo.devtype) {
        case DD_DISK:   /* disk */
            size = (off_t)devinfo.un.dk.segment_size * devinfo.un.dk.segment_count;
            break;
        case DD_SCDISK: /* scsi disk */
            size = (off_t)devinfo.un.scdk.blksize * devinfo.un.scdk.numblks;
            break;
        default:        /* unknown */
            size = 0;
            break;
    }

    (void)close(fd);
    return size;
}
#elif defined (__hpux)

#include <stropts.h>
#include <sys/scsi.h>

off_t
getsize(char *path)
{
    int fd;
    struct capacity cap;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "%s: open %s: %s\n", prog, path, strerror(errno));
        exit(1);
    }
    if (ioctl(fd, SIOC_CAPACITY, &cap) == -1) {
        fprintf(stderr, "%s: ioctl SIOC_CAPACITY %s: %s\n", prog, path,
                strerror(errno));
        exit(1);
    }

    (void)close(fd);
    return (off_t)cap.lba * cap.blksz;
}

#else
/* Unimplemented!  Scrub will tell user to use -s.
 */
off_t 
getsize(char *path)
{
    return 0;
}
#endif

void
size2str(char *str, int len, off_t size)
{
    off_t eb = size >> 60;
    off_t pb = size >> 50;
    off_t tb = size >> 40;
    off_t gb = size >> 30;
    off_t mb = size >> 20;
    off_t kb = size >> 10;
    off_t num = 0;
    char *unit = NULL;

    if (eb >= 1) {
        num = eb; unit = "EB";
    } else if (pb >= 1) {
        num = pb; unit = "PB";
    } else if (tb >= 1) {
        num = tb; unit = "TB";
    } else if (gb >= 1) {
        num = gb; unit = "GB";
    } else if (mb >= 1) {
        num = mb; unit = "MB";
    } else if (kb >= 1) {
        num = kb; unit = "KB";
    } 
   
    if (unit)
        snprintf(str, len, "%lld bytes (~%lld%s)", (long long int)size, 
                 (long long int)num, unit);
    else
        snprintf(str, len, "%lld bytes", (long long int)size);
}

off_t
str2size(char *str)
{
    char *endptr;
    unsigned long long size;
    int shift = 0;

    if (sscanf(str, "%llu", &size) != 1) /* XXX hpux has no strtoull() */
        goto err;
    for (endptr = str; *endptr; endptr++)
        if (*endptr < '0' || *endptr > '9')
            break;
    if (endptr) {
        switch (*endptr) {
            case 'K':
            case 'k':
                shift = 10;
                break;
            case 'M':
            case 'm':
                shift = 20;
                break;
            case 'G':
            case 'g':
                shift = 30;
                break;
            case 'T':
            case 't':
                shift = 40;
                break;
            case 'P':
            case 'p':
                shift = 50;
                break;
            case 'E':
            case 'e':
                shift = 60;
                break;
            case '\0':
                break;
            default:
                goto err;
        }
        if (shift > 0) {
            if (shift > sizeof(size)*8)
                goto err;
            if ((size >> (sizeof(size)*8 - shift - 1)) > 0)
                goto err;
            size <<= shift;
        }
    }
    return (off_t)size;
err:
    return 0;
}

int
str2int(char *str)
{
    off_t val = str2size(str);

    if (val > 0x7fffffffL || val < 0)
        val = 0;
    return (int)val;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
