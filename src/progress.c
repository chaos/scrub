/*****************************************************************************\
 *  $Id: progress.c 56 2005-11-24 16:32:06Z garlick $
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

/* ASCII progress bar thingie.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "progress.h"

#define PROGRESS_MAGIC  0xabcd1234

struct prog_struct {
    int magic;
    int bars;
    int maxbars;
    int batch;
    char bar;
};

void 
progress_create(prog_t *ctx, int width)
{
    if ((*ctx = (prog_t)malloc(sizeof(struct prog_struct)))) {
        (*ctx)->magic = PROGRESS_MAGIC;
        (*ctx)->maxbars = width - 2;
        (*ctx)->bars = 0;
        (*ctx)->bar = '.';
        (*ctx)->batch = !isatty(1);
        if ((*ctx)->batch)
            printf("|");
        else {
            printf("|%*s|", (*ctx)->maxbars, "");
            while (width-- > 1)
                printf("\b");
        }
        fflush(stdout);
    } 
}

void 
progress_destroy(prog_t ctx)
{
    if (ctx) {
        assert(ctx->magic == PROGRESS_MAGIC);
        ctx->bar = 'x';
        progress_update(ctx, 1.0);
        ctx->magic = 0;
        if (ctx->batch)
            printf("|\n");
        else
            printf("\n");
        free(ctx);
    }
}

void 
progress_update(prog_t ctx, double complete)
{
    assert(complete >= 0.0 && complete <= 1.0);
    if (ctx) {
        assert(ctx->magic == PROGRESS_MAGIC);
        while (ctx->bars < (double)ctx->maxbars * complete) {
            printf("%c", ctx->bar);
            fflush(stdout);
            ctx->bars++;
        }
    }
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
