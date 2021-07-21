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

#include "config.h"

void disable_hwrand(void);
int initrand(void);
void genrand(unsigned char *buf, int buflen);

#ifndef HAVE_LIBGCRYPT
int churnrand(void);
#endif /* HAVE_LIBGCRYPT. */


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
