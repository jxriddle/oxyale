#include "platform.h"
#include "crypt.h"

/* this PRNG stuff seems pretty janky, maybe we can do better later */

static void
SeedRandom( opalCrypt_t *crypt, const int32_t s )
{
    crypt->seed = s | !s;
}

static int32_t
Int32Random( opalCrypt_t *crypt )
{
    int32_t q = crypt->seed / 0x1F31D;
    int32_t r = crypt->seed % 0x1F31D;
    int32_t a = 16807 * r - 2836 * q;

    /* if 0 < a: a, else: a + 0x7FFFFFFF */
    crypt->seed = a + ( a <= 0 ) * 0x7FFFFFFF;
    return crypt->seed;
}

static double
DoubleRandom( opalCrypt_t *crypt )
{
    return (( double )Int32Random( crypt )) / 2147483647.0;
}

static int16_t
Int16Random( opalCrypt_t *crypt, const int16_t max )
{
    return ( int16_t )( DoubleRandom( crypt ) * ( double )max );
}

opalCrypt_t *
OPAL_MakeCrypt( )
{
    size_t i;
    opalCrypt_t *crypt = malloc( sizeof (*crypt) );
    SeedRandom( crypt, 0xA2C2A );
    for ( i = OPAL_CRYPT_LUT_LEN; i--; ) {
        crypt->lut[i] = Int16Random( crypt, OPAL_CRYPT_LUT_MAX );
    }

    return crypt;
}

byte *
OPAL_MakeEncryptedMessage( const opalCrypt_t *crypt, const char *message, const uint32_t byteLimit ) /* 254 */
{
    size_t c, i, z = strnlen( message, byteLimit );
    byte lastByte, *input, *output;

    output = calloc( z + 1, sizeof (*output) );
    input = calloc( z + 1, sizeof (*input) );
    memcpy( input, message, z );

    lastByte = '\0';
    c = 0;
    for ( i = z; i--; ) {
        output[i] = ( byte )( input[i] ^ crypt->lut[c++] ^ lastByte );
        lastByte = ( byte )( output[i] ^ crypt->lut[c++] );
    }

    free( input );
    return output;
 }

byte *
OPAL_MakeDecryptedMessage( const opalCrypt_t *crypt, const char *ciphertext )
{
    size_t c, i, z = strlen( ciphertext );
    byte lastByte, *input, *output;

    output = calloc( z + 1, sizeof (*output) );
    input = calloc( z + 1, sizeof (*input) );
    memcpy( input, ciphertext, z );

    lastByte = '\0';
    c = 0;
    for ( i = z; i--; ) {
        output[i] = ( byte )( input[i] ^ crypt->lut[c++] ^ lastByte );
        lastByte = ( byte )( input[i] ^ crypt->lut[c++] );
    }

    free( input );
    return output;
}

void
OPAL_FreeMessage( char *message )
{
    free( message );
}

void
OPAL_FreeCrypt( opalCrypt_t *crypt )
{
    free( crypt );
}