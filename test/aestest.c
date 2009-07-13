/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2001-2004  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include "aes.h"

char *prog;

/*
 * Rijndael Monte Carlo Test: ECB mode
 * source: NIST - rijndael-vals.zip
 */

static unsigned char AES_enc_test[3][16] =
{
    { 0xA0, 0x43, 0x77, 0xAB, 0xE2, 0x59, 0xB0, 0xD0,
      0xB5, 0xBA, 0x2D, 0x40, 0xA5, 0x01, 0x97, 0x1B },
    { 0x4E, 0x46, 0xF8, 0xC5, 0x09, 0x2B, 0x29, 0xE2,
      0x9A, 0x97, 0x1A, 0x0C, 0xD1, 0xF6, 0x10, 0xFB },
    { 0x1F, 0x67, 0x63, 0xDF, 0x80, 0x7A, 0x7E, 0x70,
      0x96, 0x0D, 0x4C, 0xD3, 0x11, 0x8E, 0x60, 0x1A }
};
    
static unsigned char AES_dec_test[3][16] =
{
    { 0xF5, 0xBF, 0x8B, 0x37, 0x13, 0x6F, 0x2E, 0x1F,
      0x6B, 0xEC, 0x6F, 0x57, 0x20, 0x21, 0xE3, 0xBA },
    { 0xF1, 0xA8, 0x1B, 0x68, 0xF6, 0xE5, 0xA6, 0x27,
      0x1A, 0x8C, 0xB2, 0x4E, 0x7D, 0x94, 0x91, 0xEF },
    { 0x4D, 0xE0, 0xC6, 0xDF, 0x7C, 0xB1, 0x69, 0x72,
      0x84, 0x60, 0x4D, 0x60, 0x27, 0x1B, 0xC5, 0x9A }
};
    
int main(int argc, char *argv[])
{
    int m, n, i, j;
    aes_context ctx;
    unsigned char buf[16];
    unsigned char key[32];

    prog = basename(argv[0]);

    for( m = 0; m < 2; m++ )
    {
        printf( "\n Rijndael Monte Carlo Test (ECB mode) - " );

        if( m == 0 ) printf( "encryption\n\n" );
        if( m == 1 ) printf( "decryption\n\n" );

        for( n = 0; n < 3; n++ )
        {
            printf( " Test %d, key size = %3d bits: ",
                    n + 1, 128 + n * 64 );

            fflush( stdout );

            memset( buf, 0, 16 );
            memset( key, 0, 16 + n * 8 );

            for( i = 0; i < 400; i++ )
            {
                aes_set_key( &ctx, key, 128 + n * 64 );

                for( j = 0; j < 9999; j++ )
                {
                    if( m == 0 ) aes_encrypt( &ctx, buf, buf );
                    if( m == 1 ) aes_decrypt( &ctx, buf, buf );
                }

                if( n > 0 )
                {
                    for( j = 0; j < (n << 3); j++ )
                    {
                        key[j] ^= buf[j + 16 - (n << 3)];
                    }
                }

                if( m == 0 ) aes_encrypt( &ctx, buf, buf );
                if( m == 1 ) aes_decrypt( &ctx, buf, buf );

                for( j = 0; j < 16; j++ )
                {
                    key[j + (n << 3)] ^= buf[j];
                }
            }

            if( ( m == 0 && memcmp( buf, AES_enc_test[n], 16 ) != 0 ) ||
                ( m == 1 && memcmp( buf, AES_dec_test[n], 16 ) != 0 ) )
            {
                printf( "failed!\n" );
                return( 1 );
            }

            printf( "passed.\n" );
        }
    }

    printf( "\n" );

    return( 0 );
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
