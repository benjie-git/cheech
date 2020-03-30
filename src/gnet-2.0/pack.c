/* GNet - Networking library
 * Copyright (C) 2000, 2001  David Helder
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

#include "pack.h"
#include <string.h>


static inline gsize strlenn(const char* str, gsize n);
static inline void flipmemcpy(char* dst, const char* src, gsize n);


#define MEMCPY(D,S,N)				\
  do {						\
    if ((N) == 1)				\
      *((char*) (D)) = *((char*) (S));		\
    else if ((N) == 2)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[0];	\
        ((char*) (D))[1] = ((char*) (S))[1];	\
      }						\
    else if ((N) == 4)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[0];	\
        ((char*) (D))[1] = ((char*) (S))[1];	\
        ((char*) (D))[2] = ((char*) (S))[2];	\
        ((char*) (D))[3] = ((char*) (S))[3];	\
      }						\
    else					\
      {						\
	memcpy((D), (S), (N));			\
      }						\
  } while(0)


#define FLIPMEMCPY(D,S,N)			\
  do {						\
    if ((N) == 1)				\
      *((char*) (D)) = *((char*) (S));		\
    else if ((N) == 2)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[1];	\
        ((char*) (D))[1] = ((char*) (S))[0];	\
      }						\
    else if ((N) == 4)				\
      {						\
        ((char*) (D))[0] = ((char*) (S))[3];	\
        ((char*) (D))[1] = ((char*) (S))[2];	\
        ((char*) (D))[2] = ((char*) (S))[1];	\
        ((char*) (D))[3] = ((char*) (S))[0];	\
      }						\
    else					\
      {						\
	flipmemcpy((D), (S), (N));		\
      }						\
  } while(0)


/*

  Get the length of string @str.  @str is at most @n characters long.
  @str need not be NUL-terminated (which is why we pass @n).

*/
static gsize
strlenn(const char* str, gsize n)
{
  gsize len = 0;

  while (*str++ && len < n) ++len;

  return len;
}


static void 
flipmemcpy(char* dst, const char* src, gsize n)
{
  while (n > 0) {
    *dst++ = src[n-1];
    --n;
  }
}


# if (G_BYTE_ORDER ==  G_LITTLE_ENDIAN)
#   define LEMEMCPY(D,S,N) MEMCPY(D,S,N)
#   define BEMEMCPY(D,S,N) FLIPMEMCPY(D,S,N)
# else
#   define BEMEMCPY(D,S,N) MEMCPY(D,S,N)
#   define LEMEMCPY(D,S,N) FLIPMEMCPY(D,S,N)
# endif


/* **************************************** */

/* SIZE of type */
/* TYPE is actual data type, VTYPE is type according to vargs, TYPESTD
   is the standard type (in endian or standard mode) */
#define SIZE(TYPENATIVE, VTYPE, TYPESTD)				\
  do {									\
    if (mult == 0) mult = 1;						\
    n += mult * (!sizemode ? sizeof(TYPENATIVE) : sizeof(TYPESTD));	\
    for (; mult != 0; mult--) { TYPENATIVE t = (TYPENATIVE) va_arg (args, VTYPE); t=t;} 	\
    /* mult = 0; */							\
  } while(0)



/* PACK does MEMCPY regardless of endian */
/* TYPE is actual data type, VTYPE is type according to vargs */
#define PACK(TYPE, VTYPE)					\
  do {								\
    for (mult=(mult?mult:1); mult; --mult)			\
      {								\
        TYPE t;							\
        g_return_val_if_fail (n + sizeof(TYPE) <= length, -1);	\
        t = (TYPE) va_arg (args, VTYPE);                        \
        MEMCPY(buffer, (char*) &t, sizeof(TYPE));               \
        buffer += sizeof(TYPE);					\
        n += sizeof(TYPE); 	                 		\
      }								\
    mult = 0;	 						\
   } while(0)


/* PACK2 does memcpy based on endian */
#define PACK2(TYPENATIVE, VTYPE, TYPESTD)				\
  do {									\
    for (mult=(mult?mult:1); mult; --mult)				\
      {									\
        if (sizemode == 0)						\
          {								\
             TYPENATIVE t;						\
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= length, -1);\
             t = (TYPENATIVE) va_arg (args, VTYPE);                    	\
             MEMCPY(buffer, (char*) &t, sizeof(TYPENATIVE));            \
             buffer += sizeof(TYPENATIVE);				\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= length, -1);	\
             t = (TYPESTD) va_arg (args, VTYPE);               		\
             LEMEMCPY(buffer, (char*) &t, sizeof(TYPESTD));             \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD t;							\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= length, -1);	\
             t = (TYPESTD) va_arg (args, VTYPE);                       	\
             BEMEMCPY(buffer, (char*) &t, sizeof(TYPESTD));             \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */


/**
 *  gnet_pack
 *  @format: pack data format
 *  @buffer: buffer to pack to
 *  @length: length of @buffer
 *  @Varargs: variables to pack from
 *  
 *  Writes @Varargs to @buffer.  @format is a string that describes
 *  the @Varargs and how they are to be packed.  This string is a list
 *  of characters, each describing the type of an argument in
 *  @Varargs.
 *
 *  An example:
 *
 *  <informalexample>
 *  <programlisting>
 *  char buf[5];
 *  int myint = 42;
 *  int mybyte = 23;
 *  &space;
 *  gnet_pack("!ib", buf, sizeof(buf), myint, mybyte);
 *  &comstart; Now buf is { 42, 0, 0, 0, 23 }; &comend;
 *  </programlisting>
 *  </informalexample>
 *
 *  As a shortcut, most types can be prefixed by an integer to specify
 *  how many times the type is repeated.  For example, "4i2b" is
 *  equivalent to "iiiibb".  This repeat argument is refered below to
 *  as REPEAT.
 *
 *  Native byte order and sizes are used by default.  If the first
 *  character of @format is < then little endian order and standard
 *  sizes are used.  If the first character is > or !, then big endian
 *  (or network) order and standard size are used.  Standard sizes are
 *  1 byte for chars, 2 bytes for shorts, and 4 bytes for ints and
 *  longs.
 *
 *  The types:
 *
 *  x is a NULL character.  It can be used for padding.  It does not
 *  correspond to an argument in @Varargs.
 *
 *  b/B is a signed/unsigned char.
 *
 *  h/H is a signed/unsigned short.
 *
 *  i/I is a signed/unsigned int.
 *
 *  l/L is a signed/unsigned long.
 *
 *  f/D is a float/double (always native order/size).
 *
 *  v is a void pointer (always native size).
 *
 *  s is a zero-terminated string.
 *
 *  S is a zero-padded string of maximum length REPEAT.  We write
 *  up-to a NULL character or REPEAT characters, whichever comes
 *  first.  We then write NULL characters up to a total of REPEAT
 *  characters.  Special case: if REPEAT is not specified, we write
 *  the string as a non-NULL-terminated string (note that it can't be
 *  unpacked easily then).
 *
 *  r is a byte array of NEXT bytes.  NEXT is the next argument passed
 *  to gnet_pack() and is a gint.
 *
 *  R is a byte array of REPEAT bytes.  REPEAT must be specified.
 *
 *  p is a Pascal string.  The string passed is a NULL-termiated
 *  string of less than 256 character.  The string writen is a
 *  non-NULL-terminated string with a byte before the string storing
 *  the string length.  REPEAT is repeat.
 *
 *  Mnemonics: (B)yte, s(H)ort, (I)nteger, (F)loat, (D)ouble, (V)oid
 *  pointer, (S)tring, (R)aw
 *
 *  Pack was inspired by Python's and Perl's pack.  It is more like
 *  Python's than Perl's.  Note that in GNet, a repeat of 0 does not
 *  align the data (as in Python).
 *
 *  Returns: number of bytes packed; -1 on error.
 *
 **/
gint
gnet_pack (const gchar* format, gchar* buffer, const gint length, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, length);
  rv = gnet_vpack (format, buffer, length, args);
  va_end (args);

  return rv;
}


/**
 *  gnet_pack_strdup
 *  @format: pack data format
 *  @bufferp: pointer to a buffer (buffer is caller owned)
 *  @Varargs: variables to pack from
 *
 *  Writes @Varargs into a buffer pointed to by @bufferp.  The buffer
 *  is allocated by the function and is caller owned.  See gnet_pack()
 *  for more information.
 *
 *  Returns: bytes packed; -1 on error.
 *
 **/
gint
gnet_pack_strdup (const gchar* format, gchar** bufferp, ...)
{
  va_list args;
  gint size;
  gint rv;
  
  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (bufferp, -1);

  /* Get size */
  va_start (args, bufferp);
  size = gnet_vcalcsize (format, args);
  va_end (args);
  g_return_val_if_fail (size >= 0, -1);
  if (size == 0)
    {
      *bufferp = NULL;
      return 0;
    }

  *bufferp = g_new (gchar, size);

  /* Pack */
  va_start (args, bufferp);
  rv = gnet_vpack (format, *bufferp, size, args);
  va_end (args);

  return rv;
}


/* **************************************** */

/**
 *  gnet_calcsize
 *  @format: pack data format
 *  @Varargs: variables
 *
 *  Calculates the size of the buffer needed to pack @Varargs by the
 *  given format.  See gnet_pack() for more information.
 *
 *  Returns: number of bytes required to pack; -1 on error.
 *  
 **/
gint
gnet_calcsize (const gchar* format, ...)
{
  va_list args;
  gint size;

  va_start (args, format);
  size = gnet_vcalcsize (format, args);
  va_end (args);

  return size;
}


/**
 *  gnet_vcalcsize
 *  @format: pack data format
 *  @args: var args
 *
 *  Var arg interface to gnet_calcsize().  See gnet_calcsize() for
 *  additional information.
 *
 *  Returns: number of bytes required to pack; -1 on error.
 *
 **/
gint
gnet_vcalcsize (const gchar* format, va_list args)
{
  gint n = 0;
  gchar* p = (gchar*) format;
  guint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  if (!format)
    return 0;

  switch (*p)
    {
    case '@':					++p;	break;
    case '<':	sizemode = 1;			++p;	break;
    case '>':	
    case '!':	sizemode = 2;			++p;	break;
    }

  for (; *p; ++p)
    {
      switch (*p)
	{
	case 'x':  { n += mult?mult:1;   mult = 0;  		break;  }
	case 'b':  { SIZE(gint8, int, gint8); 			break;	}
	case 'B':  { SIZE(guint8, unsigned int, guint8);	break;	}

	case 'h':  { SIZE(short, int, gint16); 			break;  }
	case 'H':  { SIZE(unsigned short, unsigned int, guint16); break;  }

	case 'i':  { SIZE(int, int, gint32); 			break;  }
	case 'I':  { SIZE(unsigned int, unsigned int, guint32); break;  }

	case 'l':  { SIZE(long, long, gint32); 			break;  }
	case 'L':  { SIZE(unsigned long, unsigned long, guint32); break;  }

	case 'f':  { SIZE(float, double, float);		break;  }
	case 'd':  { SIZE(double, double, double);		break;  }

	case 'v':  { SIZE(void*, void*, void*); 		break;	}

	case 's':
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s; 

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		n += strlen(s) + 1;
	      }

	    mult = 0; 
	    break;
	  }

	case 'S':  
	  { 
	    if (mult != 0)
	      n += mult;
	    else
	      {
		char* s; 
		s = va_arg (args, char*);
		n += strlen(s);
	      }

	    mult = 0; 
	    break; 
	  }

	case 'r':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s; 
		int ln;

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		ln = va_arg (args, int);
		n += ln;
	      }

	    mult = 0; 
	    break;
	  }

	case 'R':
	  {
	    char* s; 
	    s = va_arg (args, char*);

	    g_return_val_if_fail (s, -1);
	    g_return_val_if_fail (mult, -1);

	    n += mult;
	    mult = 0;
	    break;
	  }

	case 'p': 
	  {
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		char* s;
		int slen;

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		slen = strlen(s);
		n += slen + 1;
	      }

	    mult = 0;
	    break;
	  }

	case '0':case '1':case '2':case '3':case '4':
	case '5':case '6':case '7':case '8':case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');

	    break;
	  }

	case ' ':case '\t':case '\n':  break;

	default:  
	  g_return_val_if_fail (FALSE, -1);
	}
    }

  return n;
}



/**
 *  gnet_vpack
 *  @format: pack data format
 *  @buffer: buffer to pack to
 *  @length: length of @buffer
 *  @args: var args
 *
 *  Var arg interface to gnet_pack().  See gnet_pack() for more
 *  information.
 *
 *  Returns: bytes packed; -1 on error.
 *
 **/
gint
gnet_vpack (const gchar* format, gchar* buffer, const gint length, va_list args)
{
  gint n = 0;
  gchar* p = (gchar*) format;
  guint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (buffer, -1);
  g_return_val_if_fail (length, -1);

  switch (*p)
    {
    case '@':			++p;	break;
    case '<':	sizemode = 1;	++p;	break;
    case '>':	
    case '!':	sizemode = 2;	++p;	break;
    }

  for (; *p; ++p)
    {
      switch (*p)
	{
	case 'x': 
	  {	
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		g_return_val_if_fail (n + 1 <= length, -1);
		*buffer++ = 0;	++n; 
	      }

	    mult = 0;
	    break; 
	  }


	case 'b':  { PACK(gint8, int); 				 break;	}
	case 'B':  { PACK(guint8, unsigned int);		break;	}

	case 'h':  { PACK2(short, int, gint16); 		break;  }
	case 'H':  { PACK2(unsigned short, unsigned int, guint16); break;  }

	case 'i':  { PACK2(int, int, gint32); 			break;  }
	case 'I':  { PACK2(unsigned int, unsigned int, guint32); break;  }

	case 'l':  { PACK2(long, long, gint32);			break;  }
	case 'L':  { PACK2(unsigned long, unsigned long, guint32); break;  }

	case 'f':  { PACK(float, double);			break;  }
	case 'd':  { PACK(double, double);			break;  }

	case 'v':  { PACK2(void*, void*, void*); 		break;	}

	case 's':
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar* s; 
		gsize slen;

		s = va_arg (args, gchar*);
		g_return_val_if_fail (s, -1);

		slen = strlen(s);
		g_return_val_if_fail (n + slen + 1 <= length, -1);

		memcpy (buffer, s, slen + 1);	/* include the 0 */
		buffer += slen + 1;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
	  }

	case 'S':
	  {
	    gchar* s;

	    s = va_arg (args, gchar*);
	    g_return_val_if_fail (s, -1);

	    if (!mult)
	      {
		gsize slen;

		slen = strlen(s);
		g_return_val_if_fail (n + slen <= length, -1);

		memcpy (buffer, s, slen);	/* don't include the 0 */
		buffer += slen;
		n += slen;
	      }
	    else
	      {
		guint i;

		g_return_val_if_fail (n + mult <= length, -1);

		for (i = 0; i < mult && s[i]; ++i)
		  *buffer++ = s[i];
		for (; i < mult; ++i)
		  *buffer++ = 0;

		n += mult;
	      }

	    mult = 0; 
	    break;
	  }

	case 'r':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar* s; 
		guint ln;

		s = va_arg (args, gchar*);
		ln = va_arg (args, guint);

		g_return_val_if_fail (s, -1);
		g_return_val_if_fail (n + ln <= length, -1);

		memcpy(buffer, s, ln);
		buffer += ln;
		n += ln;
	      }

	    mult = 0; 
	    break;
	  }

	case 'R':  
	  { 
	    gchar* s; 

	    s = va_arg (args, gchar*);
	    g_return_val_if_fail (s, -1);

	    g_return_val_if_fail (mult, -1);
	    g_return_val_if_fail (n + mult <= length, -1);

	    memcpy (buffer, s, mult);
	    buffer += mult;
	    n += mult;
	    mult = 0; 
	    break;
	  }

	case 'p':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar* s;
		gsize slen;

		s = va_arg (args, char*);
		g_return_val_if_fail (s, -1);

		slen = strlen(s);
		g_return_val_if_fail (n < 256, -1);
		g_return_val_if_fail (n + slen + 1 <= length, -1);

		*buffer++ = slen;
		memcpy (buffer, s, slen);   
		buffer += slen;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
	  }

	case '0':  case '1': case '2': case '3': case '4':
	case '5':  case '6': case '7': case '8': case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');
	    break;
	  }

	case ' ': case '\t': case '\n': break;

	default: g_return_val_if_fail (FALSE, -1);
	}
    }

  return n;
}


/* **************************************** */

#define UNPACK(TYPE)						\
  do {								\
    for (mult=(mult?mult:1); mult; --mult)			\
      {								\
        TYPE* t;						\
        g_return_val_if_fail (n + sizeof(TYPE) <= length, FALSE);	\
        t = va_arg (args, TYPE*);                               \
        MEMCPY((char*) t, buffer, sizeof(TYPE));                \
        buffer += sizeof(TYPE);					\
        n += sizeof(TYPE); 	                 		\
      }								\
    mult = 0;	 						\
   } while(0)


#define UNPACK2(TYPENATIVE, TYPESTD)					\
  do {									\
    for (mult=(mult?mult:1); mult; --mult)				\
      {									\
        if (sizemode == 0)						\
          {								\
             TYPENATIVE* t;						\
             g_return_val_if_fail (n + sizeof(TYPENATIVE) <= length, -1);	\
             t = va_arg (args, TYPENATIVE*);                           	\
             MEMCPY((char*) t, buffer, sizeof(TYPENATIVE));             \
             buffer += sizeof(TYPENATIVE);				\
             n += sizeof(TYPENATIVE); 	                 		\
          }								\
        else if (sizemode == 1)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= length, -1);	\
             t = va_arg (args, TYPESTD*);                           	\
             LEMEMCPY((char*) t, buffer, sizeof(TYPESTD));              \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
        else if (sizemode == 2)						\
          {								\
             TYPESTD* t;						\
             g_return_val_if_fail (n + sizeof(TYPESTD) <= length, -1);	\
             t = va_arg (args, TYPESTD*);                           	\
             BEMEMCPY((char*) t, buffer, sizeof(TYPESTD));              \
             buffer += sizeof(TYPESTD);					\
             n += sizeof(TYPESTD); 	                 		\
          }								\
      }									\
    mult = 0;								\
   } while(0)


/* **************************************** */

/**
 *  gnet_unpack
 *  @format: unpack data format
 *  @buffer: buffer to unpack from
 *  @length: length of @buffer
 *  @Varargs: addresses of variables to unpack to
 *
 *  Reads the data in @buffer into @Varargs.  @format is a string that
 *  describes the @Varargs and how they are to be packed.  This string
 *  is a list of characters, each describing the type of an argument
 *  in @Varargs.
 *
 *  An example:
 *
 *  <informalexample>
 *  <programlisting>
 *  char buf[5] = { 42, 0, 0, 0, 23 };
 *  int myint;
 *  int mybyte;
 *  &space;
 *  gnet_unpack("!ib", buf, sizeof(buf), &amp;myint, &amp;mybyte);
 *  &comstart; Now myint is 42 and mybyte is 23 &comend;
 *  </programlisting>
 *  </informalexample>
 *
 *  In unpack, the arguments must be pointers to the appropriate type.
 *  Strings and byte arrays are allocated dynamicly (by g_new()).  The
 *  caller is responsible for g_free()-ing it.
 *
 *  As a shortcut, most types can be prefixed by an integer to specify
 *  how many times the type is repeated.  For example, "4i2b" is
 *  equivalent to "iiiibb".  This repeat argument is refered below to
 *  as REPEAT.
 *
 *  The types:
 *
 *  x is a pad byte.  The byte is skipped and not stored.  We do not
 *  check its value.
 *
 *  b/B is a signed/unsigned char.
 *
 *  h/H is a signed/unsigned short.
 *
 *  i/I is a signed/unsigned int.
 *
 *  l/L is a signed/unsigned long.
 *
 *  f/D is a float/double (always native order/size).
 *  
 *  v is a void pointer (always native size).
 *
 *  s is a zero-terminated string.
 *
 *  S is a zero-padded string of length REPEAT.  We read REPEAT
 *  characters or until a NULL character.  Any remaining characters
 *  are filled in with 0's.  REPEAT must be specified.
 * 
 *  r is a byte array of NEXT bytes.  NEXT is the next argument and is
 *  a gint.  REPEAT is repeat.
 * 
 *  R is a byte array of REPEAT bytes.  REPEAT must be specified.
 * 
 *  p is a Pascal string.  The first byte read is the length of the
 *  string and the string follows.  The unpacked string will be a
 *  normal, NULL-terminated string.  REPEAT is repeat.
 *
 *  Returns: number of bytes unpacked; -1 on error.  The bytes are
 *  unpacked to the variables pointed to by the @Varargs.
 * 
 **/
gint 
gnet_unpack (const gchar * format, const gchar * buffer, gint length, ...)
{
  va_list args;
  gint rv;
  
  va_start (args, length);
  rv = gnet_vunpack (format, buffer, length, args);
  va_end (args);

  return rv;
}


/**
 *  gnet_vunpack
 *  @format: unpack data format
 *  @buffer: buffer to unpack from
 *  @length: length of @buffer
 *  @args: var args
 *
 *  Var arg interface to gnet_unpack().  See gnet_unpack() for more
 *  information.
 *
 *  Returns: number of bytes packed; -1 on error.
 *
 **/
gint 
gnet_vunpack (const gchar * format, const gchar * buffer, gint length,
    va_list args)
{
  gint n = 0;
  gchar* p = (gchar*) format;
  guint mult = 0;
  gint sizemode = 0;	/* 1 = little, 2 = big */

  g_return_val_if_fail (format, -1);
  g_return_val_if_fail (buffer, -1);

  switch (*p)
    {
    case '@':					++p;	break;
    case '<':	sizemode = 1;			++p;	break;
    case '>':	
    case '!':	sizemode = 2;			++p;	break;
    }

  for (; *p; ++p)
    {
      switch (*p)
	{
	case 'x': 
	  {	
	    mult = mult? mult:1;
	    g_return_val_if_fail (n + mult <= length, FALSE);

	    buffer += mult;
	    n += mult;

	    mult = 0;
	    break; 
	  }

	case 'b':  { UNPACK(gint8); 			break;	}
	case 'B':  { UNPACK(guint8);			break;	}

	case 'h':  { UNPACK2(short, gint16); 		break;  }
	case 'H':  { UNPACK2(unsigned short, guint16);	break;  }

	case 'i':  { UNPACK2(int, gint32); 		break;  }
	case 'I':  { UNPACK2(unsigned int, guint32); 	break;  }

	case 'l':  { UNPACK2(long, gint32); 		break;  }
	case 'L':  { UNPACK2(unsigned long, guint32);	break;  }

	case 'f':  { UNPACK(float);			break;  }
	case 'd':  { UNPACK(double);			break;  }

	case 'v':  { UNPACK2(void*, void*); 		break;	}

	case 's':
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar** sp; 
		gsize slen;

		sp = va_arg (args, gchar**);
		g_return_val_if_fail (sp, -1);

		slen = strlenn(buffer, length - n);
		g_return_val_if_fail (n + slen <= length, FALSE);

		*sp = g_new(gchar, slen + 1);
		memcpy (*sp, buffer, slen);
		(*sp)[slen] = 0;
		buffer += slen + 1;
		n += slen + 1;
	      }

	    mult = 0; 
	    break;
	  }

	case 'S':
	  { 
	    gchar** sp; 
	    gsize slen;

	    g_return_val_if_fail (mult, -1);

	    sp = va_arg (args, gchar**);
	    g_return_val_if_fail (sp, -1);

	    slen = MIN(mult, strlenn(buffer, length - n));
	    g_return_val_if_fail ((n + slen) <= length, -1);

	    *sp = g_new(gchar, mult + 1);

	    memcpy (*sp, buffer, slen);
	    while (slen < (mult + 1)) (*sp)[slen++] = '\0';
	    buffer += mult;
	    n += mult;

	    mult = 0; break;
	  }

	case 'r':  /* r is the same as s, in this case. */
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar** sp; 
		guint ln;

		sp = va_arg (args, gchar**);
		ln = va_arg (args, guint);

		g_return_val_if_fail (sp, -1);
		g_return_val_if_fail (n + ln <= length, FALSE);

		*sp = g_new(char, ln);
		memcpy(*sp, buffer, ln);
		buffer += ln;
		n += ln;
	      }

	    mult = 0; break;
	  }

	case 'R':  
	  { 
	    gchar** sp; 

	    sp = va_arg (args, gchar**);
	    g_return_val_if_fail (sp, -1);

	    g_return_val_if_fail (mult, -1);
	    g_return_val_if_fail (n + mult <= length, -1);

	    *sp = g_new(char, mult);
	    memcpy(*sp, buffer, mult);
	    buffer += mult;
	    n += mult;
	    mult = 0; 
	    break;
	  }

	case 'p':  
	  { 
	    for (mult=(mult?mult:1); mult; --mult)
	      {
		gchar** sp;
		guint slen;

		sp = va_arg (args, gchar**);
		g_return_val_if_fail (sp, -1);
		g_return_val_if_fail (n + 1 <= length, FALSE);

		slen = *buffer++; 
		++n;
		g_return_val_if_fail (n + slen <= length, FALSE);

		*sp = g_new(gchar, slen + 1);
		memcpy (*sp, buffer, slen); 
		(*sp)[slen] = 0;
		buffer += slen;
		n += slen;
	      }

	    mult = 0;
	    break;
	  }

	case '0':  case '1': case '2': case '3': case '4':
	case '5':  case '6': case '7': case '8': case '9':
	  {
	    mult *= 10;
	    mult += (*p - '0');
	    break;
	  }

	case ' ': case '\t': case '\n': break;

	default: g_return_val_if_fail (FALSE, -1);
	}
    }

  return n;
}

