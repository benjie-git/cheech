/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2000  Andrew Lanoix
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
 * License along with this library; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */


#include "gnet-private.h"
#include "gnet.h"


/**
 * gnet_io_channel_writen
 * @channel: channel to write to
 * @buffer: buffer to read from
 * @length: length of @buffer
 * @bytes_writtenp: pointer to integer in which to store the
 *   number of bytes written
 * 
 * Writes all @length bytes in @buffer to @channel.  If
 * @bytes_writtenp is set, the number of bytes written is stored in
 * the integer it points to.  @bytes_writtenp will be less than
 * @length if the connection closed or an error occured.
 *
 * This function is essentially a wrapper around g_io_channel_write().
 * The problem with g_io_channel_write() is that it may not write all
 * the bytes and will return a short count even when there was not an
 * error.  This is rare, but possible, and often difficult to detect
 * when it does happen.
 *
 * Returns: %G_IO_ERROR_NONE if successful; something else otherwise.
 * The number of bytes written is stored in the integer pointed to by
 * @bytes_writtenp.
 *
 **/
GIOError
gnet_io_channel_writen (GIOChannel* channel, 
			gpointer    buffer, 
			gsize       length,
			gsize*      bytes_writtenp)
{
  gsize nleft;
  gsize nwritten;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  g_return_val_if_fail (channel, G_IO_ERROR_INVAL);
  g_return_val_if_fail (bytes_writtenp, G_IO_ERROR_INVAL);

  ptr = buffer;
  nleft = length;

  while (nleft > 0)
    {
      if ((error = g_io_channel_write(channel, ptr, nleft, &nwritten))
	  != G_IO_ERROR_NONE)
	{
	  if (error == G_IO_ERROR_AGAIN)
	    nwritten = 0;
	  else
	    break;
	}

      nleft -= nwritten;
      ptr += nwritten;
    }

  *bytes_writtenp = (length - nleft);

  return error;
}


/**
 * gnet_io_channel_readn:
 * @channel: channel to read from
 * @buffer: buffer to write to
 * @length: length of the buffer
 * @bytes_readp: pointer to integer for the function to store the 
 * number of of bytes read
 *
 * Read exactly @length bytes from @channel into @buffer.  If
 * @bytes_readp is set, the number of bytes read is stored in the
 * integer it points to.  @bytes_readp will be less than @length if the
 * end-of-file was reached or an error occured.
 *
 * This function is essentially a wrapper around g_io_channel_read().
 * The problem with g_io_channel_read() is that it may not read all
 * the bytes requested and will return a short count even when there
 * was not an error (this is rare, but it can happen and is often
 * difficult to detect when it does).
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  Also, returns the number of bytes read by modifying the
 * integer pointed to by @bytes_readp.
 *
 **/
GIOError
gnet_io_channel_readn (GIOChannel* channel, 
		       gpointer    buffer, 
		       gsize       length,
		       gsize*      bytes_readp)
{
  gsize nleft;
  gsize nread;
  gchar* ptr;
  GIOError error = G_IO_ERROR_NONE;

  g_return_val_if_fail (channel, G_IO_ERROR_INVAL);
  g_return_val_if_fail (bytes_readp, G_IO_ERROR_INVAL);

  ptr = buffer;
  nleft = length;

  while (nleft > 0)
    {
      if ((error = g_io_channel_read(channel, ptr, nleft, &nread))
	  !=  G_IO_ERROR_NONE)
	{
	  if (error == G_IO_ERROR_AGAIN)
	    nread = 0;
	  else
	    break;
	}
      else if (nread == 0)
	break;

      nleft -= nread;
      ptr += nread;
    }

  *bytes_readp = (length - nleft);

  return error;
}


/**
 * gnet_io_channel_readline
 * @channel: channel to read from
 * @buffer: buffer to write to
 * @length: length of the buffer
 * @bytes_readp: pointer to integer in which to store the 
 *   number of of bytes read
 *
 * Read a line from the channel.  The line will be NULL-terminated and
 * include the newline character.  If there is not enough room for the
 * line, the line is truncated to fit in the buffer.
 * 
 * Warnings:
 * 
 * 1. If the buffer is full and the last character is not a newline,
 * the line was truncated.  Do not assume the buffer ends with a
 * newline.
 *
 * 2. @bytes_readp is the number of bytes put in the buffer.  It
 * includes the terminating NULL character.
 * 
 * 3. NULL characters can appear in the line before the terminating
 * NULL (e.g., "Hello world\0\n").  If this matters,
 * check the string length of the buffer against the bytes read.
 *
 * I hope this isn't too confusing.  Usually the function works as you
 * expect it to if you have a big enough buffer.  If you have the
 * Stevens book, you should be familiar with the semantics.
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  The number of bytes read is stored in the integer
 * pointed to by @bytes_readp (this number includes the newline).  If
 * an error is returned, the contents of @buffer and @bytes_readp are
 * undefined.
 * 
 **/
GIOError
gnet_io_channel_readline (GIOChannel* channel, 
			  gchar*      buffer, 
			  gsize       length,
			  gsize*      bytes_readp)
{
  gsize n, rc;
  gchar c, *ptr;
  GIOError error = G_IO_ERROR_NONE;

  g_return_val_if_fail (channel, G_IO_ERROR_INVAL);
  g_return_val_if_fail (bytes_readp, G_IO_ERROR_INVAL);

  ptr = buffer;

  for (n = 1; n < length; ++n)
    {
    try_again:
      error = gnet_io_channel_readn(channel, &c, 1, &rc);

      if (error == G_IO_ERROR_NONE && rc == 1)		/* read 1 char */
	{
	  *ptr++ = c;
	  if (c == '\n')
	    break;
	}
      else if (error == G_IO_ERROR_NONE && rc == 0)	/* read EOF */
	{
	  if (n == 1)	/* no data read */
	    {
	      *bytes_readp = 0;
	      return G_IO_ERROR_NONE;
	    }
	  else		/* some data read */
	    break;
	}
      else
	{
	  if (error == G_IO_ERROR_AGAIN)
	    goto try_again;

	  return error;
	}
    }

  *ptr = 0;
  *bytes_readp = n;

  return error;
}



/**
 * gnet_io_channel_readline_strdup
 * @channel: channel to read from
 * @bufferp: pointer to gchar* in which to store the new buffer
 * @bytes_readp: pointer to integer in which to store the 
 *   number of of bytes read
 *
 * Read a line from the channel.  The line will be null-terminated and
 * include the newline character.  Similarly to g_strdup_printf, a
 * buffer large enough to hold the string will be allocated.
 * 
 * Warnings:
 * 
 * 1. If the last character of the buffer is not a newline, the line
 * was truncated by EOF.  Do not assume the buffer ends with a
 * newline.
 *
 * 2. @bytes_readp is actually the number of bytes put in the buffer.
 * It includes the terminating NULL character.
 * 
 * 3. NULL characters can appear in the line before the terminating
 * null (e.g., "Hello world\0\n").  If this matters, check the string
 * length of the buffer against the bytes read.
 *
 * Returns: %G_IO_ERROR_NONE if everything is ok; something else
 * otherwise.  The number of bytes read is stored in the integer
 * pointed to by @bytes_readp (this number includes the newline).  The
 * data pointer is stored in the pointer pointed to by @bufferp.  This
 * data is caller-owned.  If an error is returned, the contents of
 * @bufferp and @bytes_readp are undefined.
 *
 **/
GIOError
gnet_io_channel_readline_strdup (GIOChannel* channel, 
				 gchar**     bufferp, 
				 gsize*      bytes_readp)
{
  gsize rc, n, length;
  gchar c, *ptr, *buf;
  GIOError error = G_IO_ERROR_NONE;

  g_return_val_if_fail (channel, G_IO_ERROR_INVAL);
  g_return_val_if_fail (bytes_readp, G_IO_ERROR_INVAL);

  length = 100;
  buf = (gchar *)g_malloc(length);
  ptr = buf;
  n = 1;

  while (1)
    {
    try_again:
      error = gnet_io_channel_readn(channel, &c, 1, &rc);

      if (error == G_IO_ERROR_NONE && rc == 1)          /* read 1 char */
        {
          *ptr++ = c;
          if (c == '\n')
            break;
        }
      else if (error == G_IO_ERROR_NONE && rc == 0)     /* read EOF */
        {
          if (n == 1)   /* no data read */
            {
              *bytes_readp = 0;
	      *bufferp = NULL;
	      g_free(buf);

              return G_IO_ERROR_NONE;
            }
          else          /* some data read */
            break;
        }
      else
        {
          if (error == G_IO_ERROR_AGAIN)
            goto try_again;

          g_free(buf);

          return error;
        }

      ++n;

      if (n >= length)
        {
          length *= 2;
          buf = g_realloc(buf, length);
          ptr = buf + n - 1;
        }
    }

  *ptr = 0;
  *bufferp = buf;
  *bytes_readp = n;

  return error;
}
