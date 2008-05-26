/*****************************************************************************\
 *  $Id: util.c 68 2006-02-14 21:54:16Z garlick $
 *****************************************************************************
 *  Copyright (C) 2001-2008 The Regents of the University of California.
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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "util.h"

/* Handles short reads but otherwise just like read(2).
 */
int
read_all(int fd, unsigned char *buf, int count)
{
    int n;

    do {
        n = read(fd, buf, count);
        if (n > 0) {
            count -= n;
            buf += n;
        }
    } while (n > 0 && count > 0);

    return n;
}

/* Handles short writes but otherwise just like write(2).
 */
int
write_all(int fd, const unsigned char *buf, int count)
{
    int n;

    do {
        n = write(fd, buf, count);
        if (n > 0) {
            count -= n;
            buf += n;
        }
    } while (n >= 0 && count > 0);

    return n;
}

/* Return the type of file represented by 'path'.
 */
filetype_t
filetype(char *path)
{
    struct stat sb;
    filetype_t res = NOEXIST;

    if (stat(path, &sb) == 0) {
        if (S_ISREG(sb.st_mode))
            res = REGULAR;
        else if (S_ISCHR(sb.st_mode))
            res = CHAR;
        else if (S_ISBLK(sb.st_mode))
            res = BLOCK;
        else
            res = OTHER;
    }
    return res;
}

/* Round 'offset' up to an even multiple of 'blocksize'.
 */
off_t
blkalign(off_t offset, int blocksize, round_t rtype)
{
    off_t r = offset % blocksize;

    if (r > 0) {
        switch (rtype) {
            case UP:
                offset += (blocksize - r);
                break;
            case DOWN:
                offset -= r;
                break;
        }
    }

    return offset;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
