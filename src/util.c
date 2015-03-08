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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

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
    } while (n > 0 && count > 0);

    return n;
}

/* Return the type of file represented by 'path'.
 */
filetype_t
filetype(char *path)
{
    struct stat sb;

    filetype_t res = FILE_NOEXIST;

    if (lstat(path, &sb) == 0 && S_ISLNK(sb.st_mode)) {
        return FILE_LINK;
    }

    if (stat(path, &sb) == 0) {
        if (S_ISREG(sb.st_mode))
            res = FILE_REGULAR;
        else if (S_ISCHR(sb.st_mode))
            res = FILE_CHAR;
        else if (S_ISBLK(sb.st_mode))
            res = FILE_BLOCK;
        else
            res = FILE_OTHER;
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

/* Allocate an aligned buffer
 */
#define ALIGNMENT	(16*1024*1024) /* Hopefully good enough */
void *
alloc_buffer(int bufsize)
{
    void *ptr;

#ifdef HAVE_POSIX_MEMALIGN
    int err = posix_memalign(&ptr, ALIGNMENT, bufsize);
    if (err) {
        errno = err;
        ptr = NULL;
    }
#elif defined(HAVE_MEMALIGN)
    ptr = memalign(ALIGNMENT, bufsize);
#else
    ptr = malloc(bufsize);	/* Hope for the best? */
#endif

    return ptr;
}

/* check for scrub.conf in home directory only
 * code inspiration from lrzip application
 * by Con Kolivas
 * Valid Parameters and Values are:
 * PATTERN = valid pattern name excl custom (-p pat)
 * FORCE = 0,1,FALSE,TRUE (-f)
 * NOSIGNATURE = 0,1,FALSE,TRUE (-S)
 * REMOVE = 0,1,FALSE,TRUE (-r)
 * NOLINK = 0,1,FALSE,TRUE (-L)
 * NOHWRAND = 0,1,FALSE,TRUE (-R)
 * NOTHREADS = 0,1,FALSE,TRUE (-t)
 *
*/
void
read_conf( struct opt_struct *opt )
{
    char *HOME;
    char *parameter;
    char *parametervalue;
    char *homeconf;
    char *line;
    FILE *fp;

    HOME=getenv("HOME");
    if ( HOME == NULL ) 
        return;

    /* alloc sizeof HOME + len /.scrub.conf + 1 */
    homeconf = malloc( strlen(HOME)+13 );
    line = malloc( 255 );
    if ( homeconf == NULL || line == NULL ) {
        fprintf( stderr, "%s: Very Serious Memory Allocation Error: read_conf", "scrub" );
        exit(1);
    }

    /* $HOME/.scrub.conf */
    snprintf( homeconf, strlen(HOME)+13, "%s/.scrub.conf", HOME );

    /* open $HOME/.scrub.conf if available */
    fp = fopen( homeconf, "r" );
    if ( fp ) {
        while ( fgets(line, 255 , fp) != NULL ) {
            if ( strlen(line) )
                line[strlen(line) - 1] = '\0';
            parameter = strtok(line, " =");
            /* skip if blank line, space, or # */
            if ( parameter == NULL || isspace(*parameter) || *parameter == '#' )
                continue;

            parametervalue = strtok(NULL, " =");
            if ( parametervalue == NULL )
                continue;

            /* we have a valid parameter line */
            if ( !strcmp(parameter, "PATTERN") ) {
                opt->seq = seq_lookup(parametervalue); 
                if ( opt->seq == NULL )
                    fprintf( stderr, "%s: Invalid Pattern: %s. Ignored", "scrub", parametervalue );
            }
            else if ( !strcmp(parameter, "FORCE") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->force = true;
            }
            else if ( !strcmp(parameter, "NOSIGNATURE") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->nosig = true;
            }
            else if ( !strcmp(parameter, "REMOVE") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->remove = true;
            }
            else if ( !strcmp(parameter, "NOLINK") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->nofollow = true;
            }
            else if ( !strcmp(parameter, "NOHWRAND") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->nohwrand = true;
            }
            else if ( !strcmp(parameter, "NOTHREADS") ) {
                if ( !strcmp(parametervalue, "1") || !strcasecmp(parametervalue,"TRUE") )
                    opt->nothreads = true;
            }
            else 
                fprintf( stderr, "%s: Invalid Parameter in scrub.conf. Ignored: %s", "scrub", parameter ); 
        }
        if ( fclose(fp) ) {
            fprintf( stderr, "%s: Error closing %s.", "scrub", homeconf );
            exit(1);
        }
    } /* scrub.conf read complete */
    free( homeconf );
    free( line );
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
