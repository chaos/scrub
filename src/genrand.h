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
