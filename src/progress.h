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

typedef struct prog_struct *prog_t;

void progress_create(prog_t *ctx, int width);
void progress_destroy(prog_t ctx);
void progress_update(prog_t ctx, double complete);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
