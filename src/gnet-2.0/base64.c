/***********************************************************************
 *                   _     _                     __   _  _
 *   __ _ _ __   ___| |_  | |__   __ _ ___  ___ / /_ | || |
 *  / _` | '_ \ / _ \ __| | '_ \ / _` / __|/ _ \ '_ \| || |_
 * | (_| | | | |  __/ |_  | |_) | (_| \__ \  __/ (_) |__   _|
 *  \__, |_| |_|\___|\__| |_.__/ \__,_|___/\___|\___/   |_|
 *  |___/
 *
 *  created by Alfred Reibenschuh <alfredreibenschuh@gmx.net>,
 *  under the GNU Library General Public License (see below).
 *
 ***********************************************************************
 *
 * Copyright (C) 2003 Free Software Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ***********************************************************************/

/* FIXME: use new GLib functions once we depend on GLib >= 2.12 */

#include "base64.h"
#include <string.h>
#include <ctype.h>


static const gchar gnet_Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define gnet_Pad64	'='
static const guchar gnet_Base64_rank[256] = {
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0x00-0x0f	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0x10-0x1f	*/
	255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63, /*	0x20-0x2f	*/
	 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,255,255,255, /*	0x30-0x3f	*/
	255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /*	0x40-0x4f	*/
	 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255, /*	0x50-0x5f	*/
	255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /*	0x60-0x6f	*/
	 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255, /*	0x70-0x7f	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0x80-0x8f	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0x90-0x9f	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xa0-0xaf	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xb0-0xbf	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xc0-0xcf	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xd0-0xdf	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xe0-0xef	*/
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255, /*	0xf0-0xff	*/
};



/**
 *  gnet_base64_encode
 *  @src: source buffer
 *  @srclen: length of the source buffer
 *  @dstlenp: length of the buffer returned (including the terminating \0)
 *  @strict: insert new lines as required by RFC 2045
 *
 *  Convert a buffer from binary to base64 representation.  Set
 *  @strict to TRUE to insert a newline every 72th character.  This is
 *  required by RFC 2045, but some applications don't require this.
 *
 *  If @srclen is 0, an empty string will be returned (not NULL).
 *
 *  Returns: a newly-allocated and NUL-terminated string containing the
 *  input data in base64 coding. Free with g_free() when no longer needed.
 *
 **/
gchar*
gnet_base64_encode (const gchar* src, gint srclen, gint* dstlenp, gboolean strict) 
{
  gchar* dst;
  gint dstpos;
  guchar input[3];
  guchar output[4];
  gint ocnt;
  gint i;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (srclen >= 0, NULL);
  g_return_val_if_fail (dstlenp != NULL, NULL);

  if (srclen == 0) 
    return g_strdup ("");

  /* Calculate required length of dst.  4 bytes of dst are needed for
     every 3 bytes of src. */
  *dstlenp = (((srclen + 2) / 3) * 4)+5;
  if (strict)
    *dstlenp += (*dstlenp / 72);	/* Handle trailing \n */

  dst = g_new(gchar, *dstlenp );

  /* bulk encoding */
  dstpos = 0;
  ocnt = 0;
  while (srclen >= 3) 
    {
      /*
	Convert 3 bytes of src to 4 bytes of output

	output[0] = input[0] 7:2
	output[1] = input[0] 1:0 input[1] 7:4
	output[2] = input[1] 3:0 input[2] 7:6
	output[3] = input[1] 5:0

       */
      input[0] = *src++;
      input[1] = *src++;
      input[2] = *src++;
      srclen -= 3;

      output[0] = (input[0] >> 2);
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
      output[3] = (input[2] & 0x3f);

      g_assert ((dstpos + 4) < *dstlenp);

      /* Map output to the Base64 alphabet */
      dst[dstpos++] = gnet_Base64[(guint) output[0]];
      dst[dstpos++] = gnet_Base64[(guint) output[1]];
      dst[dstpos++] = gnet_Base64[(guint) output[2]];
      dst[dstpos++] = gnet_Base64[(guint) output[3]];

      /* Add a newline if strict and  */
      if (strict)
	if ((++ocnt % (72/4)) == 0) 
	  dst[dstpos++] = '\n';
    }

  /* Now worry about padding with remaining 1 or 2 bytes */
  if (srclen != 0) 
    {
      input[0] = input[1] = input[2] = '\0';
      for (i = 0; i < srclen; i++) 
	input[i] = *src++;

      output[0] = (input[0] >> 2);
      output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
      output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

      g_assert ((dstpos + 4) < *dstlenp);

      dst[dstpos++] = gnet_Base64[(guint) output[0]];
      dst[dstpos++] = gnet_Base64[(guint) output[1]];

      if (srclen == 1)
	dst[dstpos++] = gnet_Pad64;
      else
	dst[dstpos++] = gnet_Base64[(guint) output[2]];

      dst[dstpos++] = gnet_Pad64;
    }

  g_assert (dstpos <= *dstlenp);

  dst[dstpos] = '\0';

  *dstlenp = dstpos + 1;

  return dst;
}


/**
 *  gnet_base64_decode
 *  @src: the source buffer
 *  @srclen: the length of the source buffer, or -1 for strlen(@src).
 *  @dstlenp: where to return the length of the returned output buffer
 *
 *  Convert a buffer from base64 to binary representation.  This
 *  function is liberal in what it will accept.  It ignores non-base64
 *  symbols.
 *
 *  Returns: newly-allocated buffer. Free with g_free() when no longer
 *  needed. The integer pointed to by @dstlenp is set to the length of
 *  that buffer. 
 *
 **/
gchar* 
gnet_base64_decode (const gchar* src, gint srclen, gint* dstlenp)
{
  gchar* dst;
  gint   dstidx, state, ch = 0;
  gchar  res;
  guchar pos;

  g_return_val_if_fail (src != NULL, NULL);
  g_return_val_if_fail (dstlenp != NULL, NULL);

  if (srclen <= 0) 
    srclen = strlen(src);

  state = 0;
  dstidx = 0;
  res = 0;

  dst = g_new(gchar, srclen+1);
  *dstlenp = srclen+1;

  while (srclen-- > 0) 
    {
      ch = *src++;
      if (gnet_Base64_rank[ch]==255) /* Skip any non-base64 anywhere */
	continue;
      if (ch == gnet_Pad64) 
	break;

      pos = gnet_Base64_rank[ch];

      switch (state) 
	{
	case 0:
	  if (dst != NULL) 
	    {
	      dst[dstidx] = (pos << 2);
	    }
	  state = 1;
	  break;
	case 1:
	  if (dst != NULL) 
	    {
	      dst[dstidx] |= (pos >> 4);
	      res = ((pos & 0x0f) << 4);
	    }
	  dstidx++;
	  state = 2;
	  break;
	case 2:
	  if (dst != NULL) 
	    {
	      dst[dstidx] = res | (pos >> 2);
	      res = (pos & 0x03) << 6;
	    }
	  dstidx++;
	  state = 3;
	  break;
	case 3:
	  if (dst != NULL) 
	    {
	      dst[dstidx] = res | pos;
	    }
	  dstidx++;
	  state = 0;
	  break;
	default:
	  break;
	}
    }
  /*
   * We are done decoding Base-64 chars.  Let's see if we ended
   * on a byte boundary, and/or with erroneous trailing characters.
   */
  if (ch == gnet_Pad64)           /* We got a pad char. */
    {
      switch (state) 
	{
	case 0:             /* Invalid = in first position */
	case 1:             /* Invalid = in second position */
	  return NULL;
	case 2:             /* Valid, means one byte of info */
                                /* Skip any number of spaces. */
	  while (srclen-- > 0) 
	    {
	      ch = *src++;
	      if (gnet_Base64_rank[ch] != 255) break;
	    }
                                /* Make sure there is another trailing = sign. */
	  if (ch != gnet_Pad64) 
	    {
	      g_free(dst);
	      *dstlenp = 0;
	      return NULL;
	    }
                                /* FALLTHROUGH */
	case 3:             /* Valid, means two bytes of info */
                                /*
                                 * We know this char is an =.  Is there anything but
                                 * whitespace after it?
                                 */
	  while (srclen-- > 0) 
	    {
	      ch = *src++;
	      if (gnet_Base64_rank[ch] != 255) 
		{
		  g_free(dst);
		  *dstlenp = 0;
		  return NULL;
		}
	    }
                                /*
                                 * Now make sure for cases 2 and 3 that the "extra"
                                 * bits that slopped past the last full byte were
                                 * zeros.  If we don't check them, they become a
                                 * subliminal channel.
                                 */
	  if (dst != NULL && res != 0) 
	    {
	      g_free(dst);
	      *dstlenp = 0;
	      return NULL;
	    }
	default:
	  break;
	}
    } else 
      {
	/*
	 * We ended by seeing the end of the string.  Make sure we
	 * have no partial bytes lying around.
	 */
	if (state != 0) 
	  {
	    g_free(dst);
	    *dstlenp = 0;
	    return NULL;
	  }
      }
  dst[dstidx]=0;
  *dstlenp = dstidx;
  return dst;
}
