/* GNet packing unpack unit test
 * Copyright (C) 2000, 2002  David Helder
 * Copyright (C) 2007 Tim-Philipp Müller  <tim centricular net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "gnetcheck.h"

#include <string.h>

#define PTR_CONSTANT(p)  ((void*)((gulong)(p)))

#define TEST1(NUM, ANS, FORMAT, ADDR, LEN, ARG1)   do {\
  ARG1 = 0;						\
  gnet_unpack (FORMAT, ADDR, LEN, &ARG1);		\
  fail_unless (ARG1 == ANS);	} while (0)

#define TEST2(NUM, ANS, ANS2, FORMAT, ADDR, LEN, ARG1, ARG2)   do {\
  ARG1 = 0;						\
  ARG2 = 0;						\
  gnet_unpack (FORMAT, ADDR, LEN, &ARG1, &ARG2);	\
  fail_unless (ARG1 == ANS);  			\
  fail_unless (ARG2 == ANS2);	} while (0)

#define TEST3(NUM, ANS, ANS2, ANS3, FORMAT, ADDR, LEN, ARG1, ARG2, ARG3) do {\
  ARG1 = 0;						\
  ARG2 = 0;						\
  ARG3 = 0;						\
  gnet_unpack (FORMAT, ADDR, LEN, &ARG1, &ARG2, &ARG3);	\
  fail_unless (ARG1 == ANS);  			\
  fail_unless (ARG2 == ANS2); 			\
  fail_unless (ARG3 == ANS3); 	} while (0)

const char buf[12] =  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 
                        0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};

const char bufh[12] = { 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 
                        0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb};

/*** STRINGS ***/

#define TEST1S(NUM, ANS, FORMAT, BUF, LEN, ARG1)   do { \
  ARG1 = NULL;						\
  gnet_unpack (FORMAT, BUF, LEN, &ARG1);		\
  fail_unless_equals_string (ARG1, ANS);		\
  g_free(ARG1);			} while (0)

#define TEST2S(NUM, ANS, ANS2, FORMAT, BUF, LEN, ARG1, ARG2)   do {\
  ARG1 = NULL;						\
  ARG2 = NULL;						\
  gnet_unpack (FORMAT, BUF, LEN, &ARG1, &ARG2);		\
  fail_unless_equals_string (ARG1, ANS);  g_free(ARG1); \
  fail_unless_equals_string (ARG2, ANS2); g_free(ARG2); \
  } while (0)

GNET_START_TEST (test_unpack_strings)
{
  char hello[20] = "hello\0there";
  char hellop[12] = {0x05, 0x68, 0x65, 0x6c, 0x6c, 0x6f,
		     0x05, 0x74, 0x68, 0x65, 0x72, 0x65 };

  gchar *s1, *s2;

  TEST2S (40000, "hello", "there", "ss", hello, 12, s1, s2);
  TEST2S (40010, "hello", "there", "2s", hello, 12, s1, s2);
  
  s1 = NULL;
  gnet_unpack ("8S", hello, 6, &s1);
  fail_unless_equals_string (s1, "hello");  
  fail_unless_equals_int (s1[6], 0);
  g_free (s1);

  TEST1S (40110, "hello", "6S", hello, 6, s1);
  TEST1S (40120, "hell",  "4S", hello, 6, s1);
  TEST2S (40200, "hello", "there", "6S6S", hello, 12, s1, s2);

  s1 = s2 = NULL;
  gnet_unpack ("r", hello, 6, &s1, 6);
  fail_unless_equals_string (s1, "hello");
  g_free (s1);

  s1 = s2 = NULL;
  gnet_unpack ("2r", hello, 12, &s1, 6, &s2, 6);
  fail_unless_equals_string (s1, "hello");
  fail_unless_equals_string (s2, "there");
  g_free (s1);
  g_free (s2);

  TEST1S (40400, "hello", "6R", hello, 6, s1);
  TEST2S (40410, "hello", "there", "6R6R", hello, 12, s1, s2);

  TEST1S (40500, "hello", "p", hellop, 6, s1);
  TEST2S (40510, "hello", "there", "pp", hellop, 12, s1, s2);
  TEST2S (40520, "hello", "there", "2p", hellop, 12, s1, s2);
}
GNET_END_TEST;

/*** NATIVE ***/

GNET_START_TEST (test_unpack_native_common)
{
  unsigned char  iguint8;
  double doubley;
  float floaty;
  char igint8;
  /* use variable here for answer to avoid 'comparison always false due to
   * limited range of variable' compiler warnings (gcc-4.1.3) when just using
   * a simple cast and passing that into the comparison directly */
  char agint8;

  gnet_unpack ("x", buf, 1);
  fail_unless_equals_int (0, 0);

  TEST1 (10100, 0x01, "b", &buf[1], sizeof(igint8), igint8);
  agint8 = (gint8) 0xf1;
  TEST1 (10110, agint8, "b", &bufh[1], sizeof(igint8), igint8);

  TEST1 (10200, 0x01, "B", &buf[1], sizeof(iguint8), iguint8);
  TEST1 (10210, 0xf1, "B", &bufh[1], sizeof(iguint8), iguint8);

  gnet_unpack ("f", &buf[1], sizeof(float), &floaty);
  gnet_unpack ("d", &bufh[1], sizeof(double), &doubley);
}
GNET_END_TEST;

/*** NATIVE LITTLE ENDIAN ***/

GNET_START_TEST (test_unpack_native_little_endian)
{
  char   igint8, igint8_2;
  short  igint16;
  unsigned short iguint16;
  int  igint32, igint32_2;
  unsigned int iguint32;
  long  lgint32;
  unsigned long lguint32;
  void* voidp;

  TEST1 (10300, 0x0201, "h", &buf[1], sizeof(igint16), igint16);
  TEST1 (10310, 0xfffff2f1, "h", &bufh[1], sizeof(igint16), igint16);

  TEST1 (10400, 0x0201, "H", &buf[1], sizeof(iguint16), iguint16);
  TEST1 (10410, 0xf2f1, "H", &bufh[1], sizeof(iguint16), iguint16);

  TEST1 (10500, 0x4030201, "i", &buf[1], sizeof(igint32), igint32);
  TEST1 (10510, 0xf4f3f2f1, "i", &bufh[1], sizeof(igint32), igint32);

  TEST1 (10600, 0x04030201, "I", &buf[1], sizeof(iguint32), iguint32);
  TEST1 (10610, 0xf4f3f2f1, "I", &bufh[1], sizeof(iguint32), iguint32);

#if (SIZEOF_LONG == 8)
   TEST1 (10700, 0x0807060504030201, "l", &buf[1], sizeof(lgint32), lgint32);
   TEST1 (10710, 0xf8f7f6f5f4f3f2f1, "l", &bufh[1], sizeof(lgint32), lgint32);                                                                                
   TEST1 (10800, 0x0807060504030201, "L", &buf[1], sizeof(lguint32), lguint32);
   TEST1 (10810, 0xf8f7f6f5f4f3f2f1, "L", &bufh[1], sizeof(lguint32), lguint32);
#else
   TEST1 (10700, 0x4030201, "l", &buf[1], sizeof(lgint32), lgint32);
   TEST1 (10710, 0xf4f3f2f1, "l", &bufh[1], sizeof(lgint32), lgint32);
                                                                                
   TEST1 (10800, 0x04030201, "L", &buf[1], sizeof(lguint32), lguint32);
   TEST1 (10810, 0xf4f3f2f1, "L", &bufh[1], sizeof(lguint32), lguint32);
#endif

#if (SIZEOF_VOIDP == 4)
  TEST1 (11000, PTR_CONSTANT (0x04030201), "v", &buf[1], sizeof(void*), voidp);
  TEST1 (11010, PTR_CONSTANT (0xf4f3f2f1), "v", &bufh[1], sizeof(void*), voidp);
#else
  TEST1 (11000, PTR_CONSTANT (0x0807060504030201), "v", &buf[1], sizeof(void*), voidp);
  TEST1 (11010, PTR_CONSTANT (0xf8f7f6f5f4f3f2f1), "v", &bufh[1], sizeof(void*), voidp);
#endif

  TEST3 (12000, 0x01, 0x0302, 0x04, "bhb", &buf[1], 4, igint8, igint16, igint8_2);
  TEST2 (12010, 0x04030201, 0x08070605, "ii", &buf[1], 8, igint32, igint32_2);
  TEST2 (12020, 0x04030201, 0x08070605, "2i", &buf[1], 8, igint32, igint32_2);
}
GNET_END_TEST;

GNET_START_TEST (test_unpack_native_big_endian)
{
  char   igint8, igint8_2;
  short  igint16;
  unsigned short iguint16;
  int  igint32, igint32_2;
  unsigned int iguint32;
  long  lgint32;
  unsigned long lguint32;
  void* voidp;

  /* NATIVE BIG ENDIAN */

  TEST1 (10300, 0x0102, "h", &buf[1], sizeof(igint16), igint16);
  TEST1 (10310, 0xfffff1f2, "h", &bufh[1], sizeof(igint16), igint16);

  TEST1 (10400, 0x0102, "H", &buf[1], sizeof(iguint16), iguint16);
  TEST1 (10410, 0xf1f2, "H", &bufh[1], sizeof(iguint16), iguint16);

  TEST1 (10500, 0x01020304, "i", &buf[1], sizeof(igint32), igint32);
  TEST1 (10510, 0xf1f2f3f4, "i", &bufh[1], sizeof(igint32), igint32);

  TEST1 (10600, 0x01020304, "I", &buf[1], sizeof(iguint32), iguint32);
  TEST1 (10610, 0xf1f2f3f4, "I", &bufh[1], sizeof(iguint32), iguint32);

#if (SIZEOF_LONG == 8)
  TEST1 (10700, 0x0102030405060708, "l", &buf[1], sizeof(lgint32), lgint32);
  TEST1 (10710, 0xf1f2f3f4f5f6f7f8, "l", &bufh[1], sizeof(lgint32), lgint32);

  TEST1 (10800, 0x0102030405060708, "L", &buf[1], sizeof(lguint32), lguint32);
  TEST1 (10810, 0xf1f2f3f4f5f6f7f8, "L", &bufh[1], sizeof(lguint32), lguint32);
#else
  TEST1 (10700, 0x01020304, "l", &buf[1], sizeof(lgint32), lgint32);
  TEST1 (10710, 0xf1f2f3f4, "l", &bufh[1], sizeof(lgint32), lgint32);

  TEST1 (10800, 0x01020304, "L", &buf[1], sizeof(lguint32), lguint32);
  TEST1 (10810, 0xf1f2f3f4, "L", &bufh[1], sizeof(lguint32), lguint32);
#endif

#if (SIZEOF_VOIDP == 4)
  TEST1 (11000, PTR_CONSTANT (0x01020304), "v", &buf[1], sizeof(void*), voidp);
  TEST1 (11010, PTR_CONSTANT (0xf1f2f3f4), "v", &bufh[1], sizeof(void*), voidp);
#else
  TEST1 (11000, PTR_CONSTANT (0x0102030405060708), "v", &buf[1], sizeof(void*), voidp);
  TEST1 (11010, PTR_CONSTANT (0xf1f2f3f4f5f6f7f8), "v", &bufh[1], sizeof(void*), voidp);
#endif

  TEST3 (12000, 0x01, 0x0203, 0x04, "bhb", &buf[1], 4, igint8, igint16, igint8_2);
  TEST2 (12010, 0x01020304, 0x05060708, "ii", &buf[1], 8, igint32, igint32_2);
  TEST2 (12020, 0x01020304, 0x05060708, "2i", &buf[1], 8, igint32, igint32_2);
}
GNET_END_TEST;

/*** LITTLE ENDIAN ***/

GNET_START_TEST (test_unpack_little_endian)
{
  unsigned char  iguint8;
  char   igint8, igint8_2;
  short  igint16;
  unsigned short iguint16;
  int  igint32, igint32_2;
  unsigned int iguint32;
  double doubley;
  float floaty;

  /* use variable here for answer to avoid 'comparison always false due to
   * limited range of variable' compiler warnings (gcc-4.1.3) when just using
   * a simple cast and passing that into the comparison directly */
  char agint8;

  gnet_unpack ("<x", buf, 1);

  TEST1 (20100, 0x01, "<b", &buf[1], 1, igint8);
  agint8 = (gint8) 0xf1;
  TEST1 (20110, agint8, "<b", &bufh[1], 1, igint8);

  TEST1 (20200, 0x01, "<B", &buf[1], 1, iguint8);
  TEST1 (20210, 0xf1, "<B", &bufh[1], 1, iguint8);

  TEST1 (20300, 0x0201, "<h", &buf[1], 2, igint16);
  TEST1 (20310, 0xfffff2f1, "<h", &bufh[1], 2, igint16);

  TEST1 (20400, 0x0201, "<H", &buf[1], 2, iguint16);
  TEST1 (20410, 0xf2f1, "<H", &bufh[1], 2, iguint16);

  TEST1 (20500, 0x4030201, "<i", &buf[1], 4, igint32);
  TEST1 (20510, 0xf4f3f2f1, "<i", &bufh[1], 4, igint32);

  TEST1 (20600, 0x04030201, "<I", &buf[1], 4, iguint32);
  TEST1 (20610, 0xf4f3f2f1, "<I", &bufh[1], 4, iguint32);

  TEST1 (20700, 0x4030201, "<l", &buf[1], 4, igint32);
  TEST1 (20710, 0xf4f3f2f1, "<l", &bufh[1], 4, igint32);

  TEST1 (20800, 0x04030201, "<L", &buf[1], 4, iguint32);
  TEST1 (20810, 0xf4f3f2f1, "<L", &bufh[1], 4, iguint32);

  gnet_unpack ("<f", &buf[1], sizeof(float), &floaty);
  gnet_unpack ("<d", &bufh[1], sizeof(double), &doubley);

  TEST3 (22000, 0x01, 0x0302, 0x04, "<bhb", &buf[1], 4, igint8, igint16, igint8_2);
  TEST2 (22010, 0x04030201, 0x08070605, "<ii", &buf[1], 8, igint32, igint32_2);
  TEST2 (22020, 0x04030201, 0x08070605, "<2i", &buf[1], 8, igint32, igint32_2);
}
GNET_END_TEST;

/*** BIG ENDIAN ***/

GNET_START_TEST (test_unpack_big_endian)
{
  unsigned char  iguint8;
  char   igint8, igint8_2;
  short  igint16;
  unsigned short iguint16;
  int  igint32, igint32_2;
  unsigned int iguint32;
  double doubley;
  float floaty;

  /* use variable here for answer to avoid 'comparison always false due to
   * limited range of variable' compiler warnings (gcc-4.1.3) when just using
   * a simple cast and passing that into the comparison directly */
  char agint8;

  gnet_unpack (">x", buf, 1);

  TEST1 (30100, 0x01, ">b", &buf[1], 1, igint8);
  agint8 = (gint8) 0xf1;
  TEST1 (30110, agint8, ">b", &bufh[1], 1, igint8);

  TEST1 (30200, 0x01, ">B", &buf[1], 1, iguint8);
  TEST1 (30210, 0xf1, ">B", &bufh[1], 1, iguint8);

  TEST1 (30300, 0x0102, ">h", &buf[1], 2, igint16);
  TEST1 (30310, 0xfffff1f2, ">h", &bufh[1], 2, igint16);

  TEST1 (30400, 0x0102, ">H", &buf[1], 2, iguint16);
  TEST1 (30410, 0xf1f2, ">H", &bufh[1], 2, iguint16);

  TEST1 (30500, 0x01020304, ">i", &buf[1], 4, igint32);
  TEST1 (30510, 0xf1f2f3f4, ">i", &bufh[1], 4, igint32);

  TEST1 (30600, 0x01020304, ">I", &buf[1], 4, iguint32);
  TEST1 (30610, 0xf1f2f3f4, ">I", &bufh[1], 4, iguint32);

  TEST1 (30700, 0x01020304, ">l", &buf[1], 4, igint32);
  TEST1 (30710, 0xf1f2f3f4, ">l", &bufh[1], 4, igint32);

  TEST1 (30800, 0x01020304, ">L", &buf[1], 4, iguint32);
  TEST1 (30810, 0xf1f2f3f4, ">L", &bufh[1], 4, iguint32);

  gnet_unpack (">f", &buf[1], sizeof(float), &floaty);
  gnet_unpack (">d", &bufh[1], sizeof(double), &doubley);

  TEST3 (32000, 0x01, 0x0203, 0x04, ">bhb", &buf[1], 4, igint8, igint16, igint8_2);
  TEST2 (32010, 0x01020304, 0x05060708, ">ii", &buf[1], 8, igint32, igint32_2);
  TEST2 (32020, 0x01020304, 0x05060708, ">2i", &buf[1], 8, igint32, igint32_2);
}
GNET_END_TEST;

static Suite *
gnetunpack_suite (void)
{
  Suite *s = suite_create ("GNetUnpacking");
  TCase *tc_chain = tcase_create ("unpacking");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_unpack_strings);
  tcase_add_test (tc_chain, test_unpack_native_common);
  if (G_BYTE_ORDER == G_LITTLE_ENDIAN) {
    tcase_add_test (tc_chain, test_unpack_native_little_endian);
  } else {
    tcase_add_test (tc_chain, test_unpack_native_big_endian);
  }
  tcase_add_test (tc_chain, test_unpack_big_endian);
  tcase_add_test (tc_chain, test_unpack_little_endian);

  return s;
}

GNET_CHECK_MAIN (gnetunpack);

