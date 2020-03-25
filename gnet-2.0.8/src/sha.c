/*

  GNet API added by David Helder <dhelder@umich.edu> 2000-6-11.  All
  additions and changes placed in the public domain.

  Files originally from: http://www.gxsnmp.org/CVS/gxsnmp/

 */
/*
 *  sha.h : Implementation of the Secure Hash Algorithm
 *
 * Part of the Python Cryptography Toolkit, version 1.0.0
 *
 * Copyright (C) 1995, A.M. Kuchling
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */
  
/* SHA: NIST's Secure Hash Algorithm */

/* Based on SHA code originally posted to sci.crypt by Peter Gutmann
   in message <30ajo5$oe8@ccu2.auckland.ac.nz>.
   Modified to test for endianness on creation of SHA objects by AMK.
   Also, the original specification of SHA was found to have a weakness
   by NSA/NIST.  This code implements the fixed version of SHA.
*/

/* Here's the first paragraph of Peter Gutmann's posting:
   
The following is my SHA (FIPS 180) code updated to allow use of the "fixed"
SHA, thanks to Jim Gillogly and an anonymous contributor for the information on
what's changed in the new version.  The fix is a simple change which involves
adding a single rotate in the initial expansion function.  It is unknown
whether this is an optimal solution to the problem which was discovered in the
SHA or whether it's simply a bandaid which fixes the problem with a minimum of
effort (for example the reengineering of a great many Capstone chips).
*/

#include <glib.h>
#include "sha.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h" /* needed to define GNET_WIN32 */
#ifndef GNET_WIN32
#include <unistd.h>
#endif

/* The SHA block size and message digest sizes, in bytes */

#define SHA_DATASIZE    64
#define SHA_DIGESTSIZE  20

/* The structure for storing SHA info */

typedef struct {
               guint32  digest[ 5 ];         /* Message digest */
               guint32  countLo, countHi;    /* 64-bit bit count */
               guint32  data[ 16 ];          /* SHA data buffer */
	       int      Endianness;
               } SHA_CTX;

/* Message digest functions */

static void SHAInit(SHA_CTX* shaInfo);
static void SHAUpdate(SHA_CTX* shaInfo, guint8 const* buffer, guint count);
static void SHAFinal(char *key, SHA_CTX *shaInfo);
static void SHATransform(guint32 *digest, guint32 *data );

/*
 *  sha.c : Implementation of the Secure Hash Algorithm
 *
 * Part of the Python Cryptography Toolkit, version 1.0.0
 *
 * Copyright (C) 1995, A.M. Kuchling
 *
 * Distribute and use freely; there are no restrictions on further 
 * dissemination and usage except those imposed by the laws of your 
 * country of residence.
 *
 */
  
/* SHA: NIST's Secure Hash Algorithm */

/* Based on SHA code originally posted to sci.crypt by Peter Gutmann
   in message <30ajo5$oe8@ccu2.auckland.ac.nz>.
   Modified to test for endianness on creation of SHA objects by AMK.
   Also, the original specification of SHA was found to have a weakness
   by NSA/NIST.  This code implements the fixed version of SHA.
*/

/* Here's the first paragraph of Peter Gutmann's posting:
   
The following is my SHA (FIPS 180) code updated to allow use of the "fixed"
SHA, thanks to Jim Gillogly and an anonymous contributor for the information on
what's changed in the new version.  The fix is a simple change which involves
adding a single rotate in the initial expansion function.  It is unknown
whether this is an optimal solution to the problem which was discovered in the
SHA or whether it's simply a bandaid which fixes the problem with a minimum of
effort (for example the reengineering of a great many Capstone chips).
*/


#include <string.h>

/* The SHA f()-functions.  The f1 and f3 functions can be optimized to
   save one boolean operation each - thanks to Rich Schroeppel,
   rcs@cs.arizona.edu for discovering this */

/*#define f1(x,y,z) ( ( x & y ) | ( ~x & z ) )          // Rounds  0-19 */
#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
/*#define f3(x,y,z) ( ( x & y ) | ( x & z ) | ( y & z ) )   // Rounds 40-59 */
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */

/* The SHA Mysterious Constants */

#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

/* SHA initial values */

#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L

/* Note that it may be necessary to add parentheses to these macros if they
   are to be called with expressions as arguments */
/* 32-bit rotate left - kludged with shifts */

#define ROTL(n,X)  ( ( ( X ) << n ) | ( ( X ) >> ( 32 - n ) ) )

/* The initial expanding function.  The hash function is defined over an
   80-word expanded input array W, where the first 16 are copies of the input
   data, and the remaining 64 are defined by

        W[ i ] = W[ i - 16 ] ^ W[ i - 14 ] ^ W[ i - 8 ] ^ W[ i - 3 ]

   This implementation generates these values on the fly in a circular
   buffer - thanks to Colin Plumb, colin@nyx10.cs.du.edu for this
   optimization.

   The updated SHA changes the expanding function by adding a rotate of 1
   bit.  Thanks to Jim Gillogly, jim@rand.org, and an anonymous contributor
   for this information */

#define expand(W,i) ( W[ i & 15 ] = ROTL( 1, ( W[ i & 15 ] ^ W[ (i - 14) & 15 ] ^ \
                                                 W[ (i - 8) & 15 ] ^ W[ (i - 3) & 15 ] ) ) )


/* The prototype SHA sub-round.  The fundamental sub-round is:

        a' = e + ROTL( 5, a ) + f( b, c, d ) + k + data;
        b' = a;
        c' = ROTL( 30, b );
        d' = c;
        e' = d;

   but this is implemented by unrolling the loop 5 times and renaming the
   variables ( e, a, b, c, d ) = ( a', b', c', d', e' ) each iteration.
   This code is then replicated 20 times for each of the 4 functions, using
   the next 20 values from the W[] array each time */

#define subRound(a, b, c, d, e, f, k, data) \
    ( e += ROTL( 5, a ) + f( b, c, d ) + k + data, b = ROTL( 30, b ) )

/* Initialize the SHA values */

void
SHAInit(SHA_CTX * shaInfo )
{
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  shaInfo->Endianness = 1;
#else
  shaInfo->Endianness = 0;
#endif

  /* Set the h-vars to their initial values */
  shaInfo->digest[ 0 ] = h0init;
  shaInfo->digest[ 1 ] = h1init;
  shaInfo->digest[ 2 ] = h2init;
  shaInfo->digest[ 3 ] = h3init;
  shaInfo->digest[ 4 ] = h4init;

  /* Initialise bit count */
  shaInfo->countLo = shaInfo->countHi = 0;
}


/* Perform the SHA transformation.  Note that this code, like MD5, seems to
   break some optimizing compilers due to the complexity of the expressions
   and the size of the basic block.  It may be necessary to split it into
   sections, e.g. based on the four subrounds

   Note that this corrupts the shaInfo->data area */

void 
SHATransform(guint32 *digest, guint32 *data )
{
  guint32 A, B, C, D, E;     /* Local vars */
  guint32 eData[ 16 ];       /* Expanded data */

  /* Set up first buffer and local data buffer */
  A = digest[ 0 ];
  B = digest[ 1 ];
  C = digest[ 2 ];
  D = digest[ 3 ];
  E = digest[ 4 ];
  g_memmove( eData, data, SHA_DATASIZE );

  /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
  subRound( A, B, C, D, E, f1, K1, eData[  0 ] );
  subRound( E, A, B, C, D, f1, K1, eData[  1 ] );
  subRound( D, E, A, B, C, f1, K1, eData[  2 ] );
  subRound( C, D, E, A, B, f1, K1, eData[  3 ] );
  subRound( B, C, D, E, A, f1, K1, eData[  4 ] );
  subRound( A, B, C, D, E, f1, K1, eData[  5 ] );
  subRound( E, A, B, C, D, f1, K1, eData[  6 ] );
  subRound( D, E, A, B, C, f1, K1, eData[  7 ] );
  subRound( C, D, E, A, B, f1, K1, eData[  8 ] );
  subRound( B, C, D, E, A, f1, K1, eData[  9 ] );
  subRound( A, B, C, D, E, f1, K1, eData[ 10 ] );
  subRound( E, A, B, C, D, f1, K1, eData[ 11 ] );
  subRound( D, E, A, B, C, f1, K1, eData[ 12 ] );
  subRound( C, D, E, A, B, f1, K1, eData[ 13 ] );
  subRound( B, C, D, E, A, f1, K1, eData[ 14 ] );
  subRound( A, B, C, D, E, f1, K1, eData[ 15 ] );
  subRound( E, A, B, C, D, f1, K1, expand( eData, 16 ) );
  subRound( D, E, A, B, C, f1, K1, expand( eData, 17 ) );
  subRound( C, D, E, A, B, f1, K1, expand( eData, 18 ) );
  subRound( B, C, D, E, A, f1, K1, expand( eData, 19 ) );

  subRound( A, B, C, D, E, f2, K2, expand( eData, 20 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 21 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 22 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 23 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 24 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 25 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 26 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 27 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 28 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 29 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 30 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 31 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 32 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 33 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 34 ) );
  subRound( A, B, C, D, E, f2, K2, expand( eData, 35 ) );
  subRound( E, A, B, C, D, f2, K2, expand( eData, 36 ) );
  subRound( D, E, A, B, C, f2, K2, expand( eData, 37 ) );
  subRound( C, D, E, A, B, f2, K2, expand( eData, 38 ) );
  subRound( B, C, D, E, A, f2, K2, expand( eData, 39 ) );

  subRound( A, B, C, D, E, f3, K3, expand( eData, 40 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 41 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 42 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 43 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 44 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 45 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 46 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 47 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 48 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 49 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 50 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 51 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 52 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 53 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 54 ) );
  subRound( A, B, C, D, E, f3, K3, expand( eData, 55 ) );
  subRound( E, A, B, C, D, f3, K3, expand( eData, 56 ) );
  subRound( D, E, A, B, C, f3, K3, expand( eData, 57 ) );
  subRound( C, D, E, A, B, f3, K3, expand( eData, 58 ) );
  subRound( B, C, D, E, A, f3, K3, expand( eData, 59 ) );

  subRound( A, B, C, D, E, f4, K4, expand( eData, 60 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 61 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 62 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 63 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 64 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 65 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 66 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 67 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 68 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 69 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 70 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 71 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 72 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 73 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 74 ) );
  subRound( A, B, C, D, E, f4, K4, expand( eData, 75 ) );
  subRound( E, A, B, C, D, f4, K4, expand( eData, 76 ) );
  subRound( D, E, A, B, C, f4, K4, expand( eData, 77 ) );
  subRound( C, D, E, A, B, f4, K4, expand( eData, 78 ) );
  subRound( B, C, D, E, A, f4, K4, expand( eData, 79 ) );

  /* Build message digest */
  digest[ 0 ] += A;
  digest[ 1 ] += B;
  digest[ 2 ] += C;
  digest[ 3 ] += D;
  digest[ 4 ] += E;
}

/* When run on a little-endian CPU we need to perform byte reversal on an
   array of longwords. */

static void 
longReverse(guint32 *buffer, int byteCount, int Endianness )
{
  guint32 value;

  if (Endianness==1) return;
  byteCount /= sizeof( guint32 );
  while( byteCount-- )
    {
      value = *buffer;
      value = ( ( value & 0xFF00FF00L ) >> 8  ) | \
              ( ( value & 0x00FF00FFL ) << 8 );
      *buffer++ = ( value << 16 ) | ( value >> 16 );
    }
}

/* Update SHA for a block of data */

void
SHAUpdate( SHA_CTX *shaInfo, guint8 const *buffer, guint count )
{
  guint32 tmp;
  unsigned int dataCount;

  /* Update bitcount */
  tmp = shaInfo->countLo;
  if ( ( shaInfo->countLo = tmp + ( ( guint32 ) count << 3 ) ) < tmp )
    shaInfo->countHi++;             /* Carry from low to high */
  shaInfo->countHi += count >> 29;

  /* Get count of bytes already in data */
  dataCount = ( int ) ( tmp >> 3 ) & 0x3F;

  /* Handle any leading odd-sized chunks */
  if( dataCount )
    {
      guint8 *p = ( guint8 * ) shaInfo->data + dataCount;

      dataCount = SHA_DATASIZE - dataCount;
      if( count < dataCount )
        {
          g_memmove( p, buffer, count );
          return;
        }
      g_memmove( p, buffer, dataCount );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness);
      SHATransform( shaInfo->digest, shaInfo->data );
      buffer += dataCount;
      count -= dataCount;
    }

  /* Process data in SHA_DATASIZE chunks */
  while( count >= SHA_DATASIZE )
    {
      g_memmove( shaInfo->data, buffer, SHA_DATASIZE );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness );
      SHATransform( shaInfo->digest, shaInfo->data );
      buffer += SHA_DATASIZE;
      count -= SHA_DATASIZE;
    }

  /* Handle any remaining bytes of data. */
  g_memmove( shaInfo->data, buffer, count );
}

/* Final wrapup - pad to SHA_DATASIZE-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, MSB-first) */

void
SHAFinal( char *key, SHA_CTX *shaInfo )
{
  int count;
  guint8 *dataPtr;

  /* Compute number of bytes mod 64 */
  count = ( int ) shaInfo->countLo;
  count = ( count >> 3 ) & 0x3F;

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  dataPtr = ( guchar * ) shaInfo->data + count;
  *dataPtr++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = SHA_DATASIZE - 1 - count;

  /* Pad out to 56 mod 64 */
  if( count < 8 )
    {
      /* Two lots of padding:  Pad the first block to 64 bytes */
      memset( dataPtr, 0, count );
      longReverse( shaInfo->data, SHA_DATASIZE, shaInfo->Endianness );
      SHATransform( shaInfo->digest, shaInfo->data );

      /* Now fill the next block with 56 bytes */
      memset( shaInfo->data, 0, SHA_DATASIZE - 8 );
    }
  else
      /* Pad block to 56 bytes */
    memset( dataPtr, 0, count - 8 );

  /* Append length in bits and transform */
  shaInfo->data[ 14 ] = shaInfo->countHi;
  shaInfo->data[ 15 ] = shaInfo->countLo;

  longReverse( shaInfo->data, SHA_DATASIZE - 8, shaInfo->Endianness );
  SHATransform( shaInfo->digest, shaInfo->data );
  longReverse(shaInfo->digest, SHA_DIGESTSIZE, shaInfo->Endianness );
  g_memmove(key, shaInfo->digest, SHA_DIGESTSIZE);
}



/* ************************************************************ */
/* Code below is David Helder's API for GNet			*/

struct _GSHA
{
  SHA_CTX	ctx;
  guint8	digest[GNET_SHA_HASH_LENGTH];
};


/**
 *  gnet_sha_new:
 *  @buffer: buffer to hash
 *  @length: length of @buffer
 * 
 *  Creates a #GSHA from @buffer.
 *
 *  Returns: a new #GSHA.
 *
 **/
GSHA*           
gnet_sha_new (const gchar* buffer, guint length)
{
  GSHA* sha;

  sha = g_new0 (GSHA, 1);
  SHAInit (&sha->ctx);
  SHAUpdate (&sha->ctx, (const guchar*) buffer, length);
  SHAFinal ((gpointer) &sha->digest, &sha->ctx);

  return sha;
}



/**
 *  gnet_sha_new_string:
 *  @str: hexidecimal string
 * 
 *  Createss a #GSHA from @str.  @str is a hexidecimal string
 *  representing the digest.
 *
 *  Returns: a new #GSHA.
 *
 **/
GSHA*		
gnet_sha_new_string (const gchar* str)
{
  GSHA* sha;
  guint i;

  g_return_val_if_fail (str, NULL);
  g_return_val_if_fail (strlen(str) == (GNET_SHA_HASH_LENGTH * 2), NULL);

  sha = g_new0 (GSHA, 1);

  for (i = 0; i < (GNET_SHA_HASH_LENGTH * 2); ++i)
    {
      guint val = 0;

      switch (str[i])
	{
	case '0':	val = 0;	break;
	case '1':	val = 1;	break;
	case '2':	val = 2;	break;
	case '3':	val = 3;	break;
	case '4':	val = 4;	break;
	case '5':	val = 5;	break;
	case '6':	val = 6;	break;
	case '7':	val = 7;	break;
	case '8':	val = 8;	break;
	case '9':	val = 9;	break;
	case 'A':
	case 'a':	val = 10;	break;
	case 'B':
	case 'b':	val = 11;	break;
	case 'C':
	case 'c':	val = 12;	break;
	case 'D':
	case 'd':	val = 13;	break;
	case 'E':
	case 'e':	val = 14;	break;
	case 'F':
	case 'f':	val = 15;	break;
	default:
	  g_return_val_if_fail (FALSE, NULL);
	}

      if (i % 2)
	sha->digest[i / 2] |= val;
      else
	sha->digest[i / 2] = val << 4;
    }

  return sha;
}



/**
 *  gnet_sha_clone
 *  @sha: a #GSHA
 * 
 *  Copies a #GSHA.
 *
 *  Returns: a copy of @sha.
 *
 **/
GSHA*           
gnet_sha_clone (const GSHA* sha)
{
  GSHA* sha2;

  g_return_val_if_fail (sha, NULL);

  sha2      = g_new0 (GSHA, 1);
  sha2->ctx = sha->ctx;
  memcpy (sha2->digest, sha->digest, sizeof(sha->digest));

  return sha2;
}



/** 
 *  gnet_sha_delete
 *  @sha: a #GSHA
 *
 *  Deletes a #GSHA.
 *
 **/
void
gnet_sha_delete (GSHA* sha)
{
  g_free (sha);
}





/**
 *  gnet_sha_new_incremental
 *
 *  Creates a #GSHA incrementally.  After creating a #GSHA, call
 *  gnet_sha_update() one or more times to hash data.  Finally, call
 *  gnet_sha_final() to compute the final hash value.
 *
 *  Returns: a new #GSHA.
 *
 **/
GSHA*		
gnet_sha_new_incremental (void)
{
  GSHA* sha;

  sha = g_new0 (GSHA, 1);
  SHAInit (&sha->ctx);
  return sha;
}


/**
 *  gnet_sha_update
 *  @sha: a #GSHA
 *  @buffer: buffer to add
 *  @length: length of @buffer
 *
 *  Updates the hash with @buffer.  This may be called several times
 *  on a hash created by gnet_sha_new_incremental() before being
 *  finalized by calling gnet_sha_final().
 * 
 **/
void
gnet_sha_update (GSHA* sha, const gchar* buffer, guint length)
{
  g_return_if_fail (sha);

  SHAUpdate (&sha->ctx, (const guchar*) buffer, length);
}


/**
 *  gnet_sha_final
 *  @sha: a #GSHA
 *
 *  Calcuates the final hash value of a #GSHA.  This should only be
 *  called on a #GSHA created by gnet_sha_new_incremental().
 *
 **/
void
gnet_sha_final (GSHA* sha)
{
  g_return_if_fail (sha);

  SHAFinal ((gpointer) &sha->digest, &sha->ctx);
}



/* **************************************** */

/**
 *  gnet_sha_get_digest
 *  @sha: a #GSHA
 *
 *  Gets the raw SHA digest.
 *
 *  Returns: a callee-owned buffer containing the SHA hash digest.
 *  The buffer is %GNET_SHA_HASH_LENGTH bytes long.
 *
 **/
gchar*        	
gnet_sha_get_digest (const GSHA* sha)
{
  g_return_val_if_fail (sha, NULL);
  
  return (gchar*) sha->digest;
}


static const gchar bits2hex[] = "0123456789abcdef";

/**
 *  gnet_sha_get_string
 *  @sha: a #GSHA
 *
 *  Get the digest represented a human-readable string.
 *
 *  Returns: a hexadecimal string representing the digest.  The string
 *  is 2 * %GNET_SHA_HASH_LENGTH bytes long and NULL terminated.  The
 *  string is caller owned.
 *
 **/
gchar*          
gnet_sha_get_string (const GSHA* sha)
{
  gchar* str;

  g_return_val_if_fail (sha, NULL);

  str = g_new (gchar, GNET_SHA_HASH_LENGTH * 2 + 1);

  gnet_sha_copy_string (sha, str);
  str[GNET_SHA_HASH_LENGTH * 2] = '\0';

  return str;
}




/**
 * gnet_sha_copy_string
 * @sha: a #GSHA
 * @buffer: buffer at least 2 * %GNET_SHA_HASH_LENGTH bytes long
 *
 * Copies the digest, represented as a string, into @buffer.  The
 * string is not NULL terminated.
 * 
 **/
void
gnet_sha_copy_string (const GSHA* sha, gchar* buffer)
{
  guint i;

  g_return_if_fail (sha);
  g_return_if_fail (buffer);

  for (i = 0; i < GNET_SHA_HASH_LENGTH; ++i)
    {
      buffer[i * 2]       = bits2hex[(sha->digest[i] & 0xF0) >> 4];
      buffer[(i * 2) + 1] = bits2hex[(sha->digest[i] & 0x0F)     ];
    }
}


/**
 *  gnet_sha_equal
 *  @p1: a #GSHA.
 *  @p2: another #GSHA.
 *
 *  Compares two #GSHA's for equality.
 *
 *  Returns: TRUE if they are equal; FALSE otherwise.
 *
 **/
gboolean
gnet_sha_equal (gconstpointer p1, gconstpointer p2)
{
  GSHA* shaa = (GSHA*) p1;
  GSHA* shab = (GSHA*) p2;
  guint i;

  for (i = 0; i < GNET_SHA_HASH_LENGTH; ++i)
    if (shaa->digest[i] != shab->digest[i])
      return FALSE;

  return TRUE;
}


/**
 *  gnet_sha_hash
 *  @p: a #GSHA
 *
 *  Creates a hash code for a #GMD5 for use with GHashTable.  This
 *  hash value is not the same as the MD5 digest.
 *
 *  Returns: the hash code for @p.
 *
 **/
guint
gnet_sha_hash (gconstpointer p)
{
  const GSHA* sha = (const GSHA*) p;
  const guint* q;

  g_return_val_if_fail (sha, 0);

  q = (const guint*) sha->digest;

  return (q[0] ^ q[1] ^ q[2] ^ q[3] ^ q[4]);
}
