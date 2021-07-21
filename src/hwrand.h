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

typedef bool (*hwrand_t)(unsigned char *, int);

hwrand_t init_hwrand(void);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
