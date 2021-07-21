/************************************************************\
 * Copyright 2001 The Regents of the University of California.
 * Copyright 2007 Lawrence Livermore National Security, LLC.
 * (c.f. DISCLAIMER, COPYING)
 *
 * This file is part of Scrub.
 * For details, see https://github.com/chaos/scrub.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
\************************************************************/

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
