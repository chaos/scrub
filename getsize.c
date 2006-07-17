/*****************************************************************************\
 *  $Id: getsize.c 76 2006-02-15 00:49:19Z garlick $
 *****************************************************************************
 *  Copyright (C) 2005 The Regents of the University of California.
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

#if defined(linux) || defined(sun)
#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#if defined(_AIX)
#define _LARGE_FILES
#include <sys/mode.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if !defined(__FreeBSD__) && !defined(sun)
#include <stdint.h>
#endif

#if defined(linux)
/* tested linux 2.6.11-1.1369_FC4 */

#include <sys/ioctl.h>
#include <linux/fs.h>

off_t 
getsize(char *path)
{
    unsigned long numblocks;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        exit(1);
    }
    if (ioctl(fd, BLKGETSIZE, &numblocks) < 0) {
        perror(path);
        exit(1);
    }
    (void)close(fd);

    return (off_t)numblocks*512;
}

#elif defined(__FreeBSD__)
/* tested freebsd 5.3-RELEASE-p5 */

#include <sys/ioctl.h>
#include <sys/disk.h>

off_t 
getsize(char *path)
{
    off_t numbytes;
    int fd;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror(path);
        exit(1);
    }
    if (ioctl(fd, DIOCGMEDIASIZE, &numbytes) < 0) {
        perror(path);
        exit(1);
    }
    (void)close(fd);

    return numbytes;
}

#elif defined(sun)
/* tested solaris 5.9 */

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
        perror(path);
        exit(1);
    }
    if (ioctl(fd, DKIOCGMEDIAINFO, &dkmp) < 0) {
        perror(path);
        exit(1);
    }
    (void)close(fd);

    return (off_t)dkmp.dki_capacity * dkmp.dki_lbsize;
}

#elif defined(__APPLE__)
/* tested OS X 7.9.0 */

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
        perror(path);
        exit(1);
    }
    if (ioctl(fd, DKIOCGETBLOCKSIZE, &blocksize) < 0) {
        perror(path);
        exit(1);
    }
    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &blockcount) < 0) {
        perror(path);
        exit(1);
    }
    (void)close(fd);

    return (off_t)blockcount * blocksize;
}
#elif defined(_AIX)
/* contributed by Dave Fox */
/* tested AIX 5.1 and 5.3 */
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
        perror(path);
        exit(1);
    }
    if (ioctl(fd, IOCINFO, &devinfo) == -1) {
        perror("ioctl IOCINFO");
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
    return size;
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

    if (eb >= 1)
        snprintf(str, len, "%lld bytes (~%lldEB)", size, eb);
    else if (pb >= 1)
        snprintf(str, len, "%lld bytes (~%lldPB)", size, pb);
    else if (tb >= 1)
        snprintf(str, len, "%lld bytes (~%lldTB)", size, tb);
    else if (gb >= 1)
        snprintf(str, len, "%lld bytes (~%lldGB)", size, gb);
    else if (mb >= 1)
        snprintf(str, len, "%lld bytes (~%lldMB)", size, mb);
    else if (kb >= 1)
        snprintf(str, len, "%lld bytes (~%lldKB)", size, kb);
    else
        snprintf(str, len, "%lld bytes", size);
}

off_t
str2size(char *str)
{
    char *endptr;
    unsigned long long size;
    int shift = 0;

    size = strtoull(str, &endptr, 10);
    if (size > (~0LL) || size == 0)
        goto err;
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
    fprintf(stderr, "error parsing size string\n");
    return 0;
}

#ifdef STAND
int
main(int argc, char *argv[])
{
    off_t sz;
    struct stat sb;
    char buf[80];

    if (argc != 2) {
        fprintf(stderr, "Usage: getsize [file|string]\n");
        exit(1);
    }
    if (stat(argv[1], &sb) < 0) {
        sz = str2size(argv[1]);
    } else {
        if (!S_ISCHR(sb.st_mode) && !S_ISBLK(sb.st_mode)) {
            fprintf(stderr, "file must be block or char special\n");
            exit(1);
        }
        sz = getsize(argv[1]);
    }
    if (sz != 0) {
        size2str(buf, sizeof(buf), sz); 
        printf("%s\n", buf);
    }
    exit(0);
}
#endif

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
