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

int getsize(char *path, off_t *sizep);
off_t str2size(char *str);
int str2int(char *str);
void size2str(char *str, int len, off_t size);

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
