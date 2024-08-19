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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include <libgen.h>

#include "util.h"
#include "genrand.h"
#include "hwrand.h"

#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
#include "aes.h"
#elif defined(HAVE_LIBGCRYPT)
#include <gcrypt.h>
#elif defined(HAVE_OPENSSL)
#include <openssl/rand.h>
#endif /* !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL) */

extern char *prog;

static bool no_hwrand = false;
static hwrand_t gen_hwrand;

#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
#define PATH_URANDOM    "/dev/urandom"

#define PAYLOAD_SZ  16
#define KEY_SZ      16

static aes_context  ctx;
static unsigned char ctr[PAYLOAD_SZ];

#if HAVE_RAND_R
static unsigned int seed;
#elif HAVE_RANDOM_R
static char rstate[128];
static struct random_data rdata;
#else
#error Neither rand_r nor random_r are available
#endif

/* Increment 128 bit counter.
 * NOTE: we are not concerned with endianness here since the counter is
 * just sixteen bytes of payload to AES and outside of this function isn't
 * operated upon numerically.
 */
static void
incr128(unsigned char *val)
{
    unsigned long long *t = (unsigned long long *)val;

    assert(sizeof(unsigned long long) == 8);
    assert(PAYLOAD_SZ == 16);
    if (++t[0] == 0)
        ++t[1];
}

/* Copy 'buflen' bytes of raw randomness into 'buf'.
 */
static int
genrandraw(unsigned char *buf, int buflen)
{
    static int fd = -1;
    int n;

    if (fd < 0) {
        fd = open(PATH_URANDOM, O_RDONLY);

        /* Still can't open /dev/urandom - this is weak */
        if (fd < 0) {
#if HAVE_RAND_R
            for (n = 0; n < buflen; n++)
                buf[n] = rand_r (&seed);
#elif HAVE_RANDOM_R
            int32_t result;

            for (n = 0; n < buflen; n++) {
                if (random_r(&rdata, &result) < 0)
                    goto error;
                buf[n] = result;
            }
#endif
            return 0;
        }
    }

    n = read_all(fd, buf, buflen);
    if (n < 0)
        goto error;
    if (n == 0) {
        errno = EIO; /* early EOF */
        goto error;
    }
    return 0;
error:
    return -1;
}

/* Pick new (random) key and counter values.
 */
int
churnrand(void)
{
    unsigned char key[KEY_SZ];

    if (genrandraw(ctr, PAYLOAD_SZ) < 0)
        goto error;
    if (genrandraw(key, KEY_SZ) < 0)
        goto error;
    if (aes_set_key(&ctx, key, KEY_SZ*8) != 0) {
        errno = EINVAL;
        goto error;
    }
    return 0;
error:
    return -1;
}
#endif /* !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL) */

/* Initialize the module.
 */
int
initrand(void)
{
#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
    struct timeval tv;
#elif defined(HAVE_LIBCRYPT)
    if (!gcry_check_version(GCRYPT_VERSION)) {
        goto error;
    }
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif /* !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL) */

    if (!no_hwrand)
        gen_hwrand = init_hwrand();

#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
    /* Always initialize the software random number generator as backup */

    if (gettimeofday(&tv, NULL) < 0)
        goto error;
#if HAVE_RAND_R
    seed = tv.tv_usec;
#elif HAVE_RANDOM_R
    if (initstate_r(tv.tv_usec, rstate, sizeof(rstate), &rdata) < 0)
        goto error;
#endif
    if (churnrand() < 0)
        goto error;
#endif /* !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL) */
    return 0;
error:
    return -1;
}

/* Fill buf with random data.
 */
void
genrand(unsigned char *buf, int buflen)
{
#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
    int i;
    unsigned char out[PAYLOAD_SZ];
    int cpylen = PAYLOAD_SZ;
#endif

    if (gen_hwrand) {
        bool hwok = gen_hwrand(buf, buflen);
        if (hwok)
            return;
    }

#if !defined(HAVE_LIBGCRYPT) && !defined(HAVE_OPENSSL)
    for (i = 0; i < buflen; i += cpylen) {
        aes_encrypt(&ctx, ctr, out);
        incr128(ctr);
        if (cpylen > buflen - i)
            cpylen = buflen - i;
        memcpy(&buf[i], out, cpylen);
    }
    assert(i == buflen);
#elif defined(HAVE_LIBGCRYPT)
    gcry_randomize(buf, buflen, GCRY_STRONG_RANDOM);
#elif defined(HAVE_OPENSSL)
    assert(RAND_bytes(buf, buflen) == 1);
#endif /* HAVE_LIBGCRYPT. */
}

/*
 * Disable hardware random number generation
 */
void
disable_hwrand(void)
{
    no_hwrand = true;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
