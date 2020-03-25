/* GNet packing unit test
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

#include "config.h" /* for SIZEOF_VOIDP */

#include "gnetcheck.h"

#define PTR_CONSTANT(p)  ((void*)((gulong)(p)))

static void test_bytes (int line_num, char * s_binary, int len,
    char * answer_ascii);

static const gchar bits2hex[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static void
test_bytes (int line_num, char * s_binary, int len, char * answer_ascii)
{
  char *s = s_binary;
  char *answer = answer_ascii;
  int i;

  for (i = 0; i < len; ++i)
    {
      if (!answer[2 * i] || !answer[(2 * i) + 1] ||
	  (bits2hex[(s[i] & 0xf0) >> 4] != answer[2 * i]) ||
	  (bits2hex[(s[i] & 0xf)      ] != answer[(2 * i) + 1]))
      {
	int j;

	g_printerr ("FAILURE: test at line %u at byte %d\n", line_num, i);
	g_printerr ("\toutput   : ");
	for (j = 0; j < len; ++j)
	  g_printerr ("%c%c", bits2hex[(s[j] & 0xf0) >> 4], 
		   bits2hex[s[j] & 0xf]);
	g_printerr ("\n\tshould be: %s\n", answer);
	g_assert (0); /* abort */
      }
    }
}

/* macros to test format with N arguments (not all compilers support varargs
 * in macros); We don't use the test number NUM any longer, but keep it around
 * for now */
#define TEST0(NUM, ANSWER, FORMAT, SIZE)                \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      g_assert (SIZE == strlen(ANSWER)/2);              \
      calclen = gnet_calcsize (FORMAT);                 \
      fail_unless (calclen == strlen (ANSWER)/2,        \
          "Calculated length %u for format '%s', but "  \
          "answer is only %u bytes", calclen, FORMAT,   \
          strlen (ANSWER) / 2);                         \
      len = gnet_pack (FORMAT, buffer, SIZE);           \
      fail_unless_equals_int (len, calclen);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      len = gnet_pack_strdup (FORMAT, &str);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      g_free (str);                                     \
    } G_STMT_END;

#define TEST1(NUM, ANSWER, FORMAT, SIZE, ARG1)          \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      g_assert (SIZE == strlen(ANSWER)/2);              \
      calclen = gnet_calcsize (FORMAT, ARG1);           \
      fail_unless (calclen == strlen (ANSWER)/2,        \
          "Calculated length %u for format '%s', but "  \
          "answer is only %u bytes", calclen, FORMAT,   \
          strlen (ANSWER) / 2);                         \
      len = gnet_pack (FORMAT, buffer, SIZE, ARG1);     \
      fail_unless_equals_int (len, calclen);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      len = gnet_pack_strdup (FORMAT, &str, ARG1);      \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      g_free (str);                                     \
    } G_STMT_END;

#define TEST2(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2)    \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      g_assert (SIZE == strlen(ANSWER)/2);              \
      calclen = gnet_calcsize (FORMAT, ARG1, ARG2);     \
      fail_unless (calclen == strlen (ANSWER)/2,        \
          "Calculated length %u for format '%s', but "  \
          "answer is only %u bytes", calclen, FORMAT,   \
          strlen (ANSWER) / 2);                         \
      len = gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2);\
      fail_unless_equals_int (len, calclen);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2);\
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      g_free (str);                                     \
    } G_STMT_END;

#define TEST3(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2, ARG3) \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      g_assert (SIZE == strlen(ANSWER)/2);              \
      calclen = gnet_calcsize (FORMAT, ARG1, ARG2, ARG3);\
      fail_unless (calclen == strlen (ANSWER)/2,        \
          "Calculated length %u for format '%s', but "  \
          "answer is only %u bytes", calclen, FORMAT,   \
          strlen (ANSWER) / 2);                         \
      len = gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2, ARG3);\
      fail_unless_equals_int (len, calclen);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2, ARG3);\
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      g_free (str);                                     \
    } G_STMT_END;

#define TEST4(NUM, ANSWER, FORMAT, SIZE, ARG1, ARG2, ARG3, ARG4) \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      g_assert (SIZE == strlen(ANSWER)/2);              \
      calclen = gnet_calcsize (FORMAT, ARG1, ARG2, ARG3, ARG4);\
      fail_unless (calclen == strlen (ANSWER)/2,        \
          "Calculated length %u for format '%s', but "  \
          "answer is only %u bytes", calclen, FORMAT,   \
          strlen (ANSWER) / 2);                         \
      len = gnet_pack (FORMAT, buffer, SIZE, ARG1, ARG2, ARG3, ARG4);\
      fail_unless_equals_int (len, calclen);            \
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      len = gnet_pack_strdup (FORMAT, &str, ARG1, ARG2, ARG3, ARG4);\
      test_bytes (__LINE__, buffer, SIZE, ANSWER);      \
      g_free (str);                                     \
    } G_STMT_END;

#define MEMTEST1(NUM, FORMAT, SIZE, ARG1) \
    G_STMT_START {                                      \
      char buffer[SIZE];                                \
      char *str;                                        \
      int len, calclen;                                 \
      calclen = gnet_calcsize (FORMAT, ARG1);           \
      fail_unless_equals_int (SIZE, calclen);           \
      len = gnet_pack (FORMAT, buffer, SIZE, ARG1);     \
      fail_unless_equals_int (len, calclen);            \
      len = gnet_pack_strdup (FORMAT, &str, ARG1);      \
      fail_unless_equals_int (len, calclen);            \
      g_free (str);                                     \
    } G_STMT_END;

/*** FAILURES (these cause warnings) ***/

GNET_START_TEST (test_pack_failures)
{
  gchar buffer[256];

  ASSERT_CRITICAL (gnet_pack ("b", buffer, 0, 0));
  ASSERT_CRITICAL (gnet_pack ("B", buffer, 0, 0));
  ASSERT_CRITICAL (gnet_pack ("h", buffer, sizeof(short) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("H", buffer, sizeof(unsigned short) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("i", buffer, sizeof(int) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("I", buffer, sizeof(unsigned int) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("l", buffer, sizeof(long) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("L", buffer, sizeof (gulong) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("f", buffer, sizeof(float) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("d", buffer, sizeof(double) - 1, 0));
  ASSERT_CRITICAL (gnet_pack ("s", buffer, 5, "hello"));
  ASSERT_CRITICAL (gnet_pack ("ss", buffer, 11, "hello", "world"));
  ASSERT_CRITICAL (gnet_pack ("12S", buffer, 11, "booger"));
  ASSERT_CRITICAL (gnet_pack ("4S", buffer, 3, "david"));
  ASSERT_CRITICAL (gnet_pack ("6S", buffer, 5, "helder"));
  ASSERT_CRITICAL (gnet_pack ("2r", buffer, 7, "abcd", 4, "efgh", 4));
  ASSERT_CRITICAL (gnet_pack ("4R", buffer, 3, "dcba"));
  ASSERT_CRITICAL (gnet_pack ("4R4R", buffer, 7, "efgh", "abcd"));
  ASSERT_CRITICAL (gnet_pack ("p", buffer, 4, "abcd"));
  ASSERT_CRITICAL (gnet_pack ("2p", buffer, 9, "efgh", "abcd"));
}
GNET_END_TEST;

/*** STRINGS ***/

GNET_START_TEST (test_pack_strings)
{
  TEST2 (40000, "68656c6c6f00746865726500", "ss", 12, "hello", "there");
  TEST2 (40010, "74686572650068656c6c6f00", "2s", 12, "there", "hello");
  TEST1 (40020, "626f6f676572000000000000", "12S", 12, "booger");
  TEST1 (40030, "64617669", "4S", 4, "david");
  TEST1 (40040, "68656c646572", "6S", 6, "helder");
  TEST2 (40050, "64636261", "r", 4, "dcba", 4);
  TEST4 (40060, "6162636465666768", "2r", 8, "abcd", 4, "efgh", 4);
  TEST1 (40070, "64636261", "4R", 4, "dcba"); 
  TEST2 (40080, "6566676861626364", "4R4R", 8, "efgh", "abcd");
  TEST1 (40090, "0461626364", "p", 5, "abcd");
  TEST2 (40100, "04656667680461626364", "2p", 10, "efgh", "abcd");
}
GNET_END_TEST;

/*** LITTLE ENDIAN, STANDARD SIZES ***/

GNET_START_TEST (test_pack_little_endian)
{
  TEST0 (20100, "00", "<x", 1);

  TEST1 (20200, "17", "<b", 1, 0x17);
  TEST1 (20210, "f1", "<b", 1, 0xf1);

  TEST1 (20300, "17", "<B", 1, 0x17);
  TEST1 (20310, "f1", "<B", 1, 0xf1);

  TEST1 (20400, "0201", "<h", 2, 0x0102);
  TEST1 (20410, "01f0", "<h", 2, 0xf001);
  TEST1 (20420, "f001", "<h", 2, 0x01f0);

  TEST1 (20500, "0201", "<H", 2, 0x0102);
  TEST1 (20510, "01f0", "<H", 2, 0xf001);
  TEST1 (20520, "f001", "<H", 2, 0x01f0);

  TEST1 (20600, "04030201", "<i", 4, 0x01020304);
  TEST1 (20610, "040302f1", "<i", 4, 0xf1020304);
  TEST1 (20620, "f4030201", "<i", 4, 0x010203f4);

  TEST1 (20700, "04030201", "<I", 4, 0x01020304);
  TEST1 (20710, "040302f1", "<I", 4, 0xf1020304);
  TEST1 (20720, "f4030201", "<I", 4, 0x010203f4);

  TEST1 (20800, "04030201", "<l", 4, 0x01020304);
  TEST1 (20810, "040302f1", "<l", 4, 0xf1020304);
  TEST1 (20820, "f4030201", "<l", 4, 0x010203f4);

  TEST1 (20900, "04030201", "<L", 4, 0x01020304);
  TEST1 (20910, "040302f1", "<L", 4, 0xf1020304);
  TEST1 (20920, "f4030201", "<L", 4, 0x010203f4);

  /* these should always be native size and endianness */
  /* CHECKME: do floats not get promoted to doubles in vararg functions?! */
  MEMTEST1 (21000, "<f", sizeof(float),  23.43);
  MEMTEST1 (21010, "<d", sizeof(double), 43.21);
  
  TEST3 (21100, "00010002", "<bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (21110, "0403020108070605", "<ii", 8, 0x01020304, 0x05060708);
  TEST2 (21120, "0403020108070605", "<2i", 8, 0x01020304, 0x05060708);
}
GNET_END_TEST;

/*** BIG ENDIAN, STANDARD SIZES ***/

GNET_START_TEST (test_pack_big_endian)
{
  TEST0 (30100, "00", ">x", 1);

  TEST1 (30200, "17", ">b", 1, 0x17);
  TEST1 (30210, "f1", ">b", 1, 0xf1);

  TEST1 (30300, "17", ">B", 1, 0x17);
  TEST1 (30310, "f1", ">B", 1, 0xf1);

  TEST1 (30400, "0102", ">h", 2, 0x0102);
  TEST1 (30410, "f001", ">h", 2, 0xf001);
  TEST1 (30420, "01f0", ">h", 2, 0x01f0);

  TEST1 (30500, "0102", ">H", 2, 0x0102);
  TEST1 (30510, "f001", ">H", 2, 0xf001);
  TEST1 (30520, "01f0", ">H", 2, 0x01f0);

  TEST1 (30600, "01020304", ">i", 4, 0x01020304);
  TEST1 (30610, "f1020304", ">i", 4, 0xf1020304);
  TEST1 (30620, "010203f4", ">i", 4, 0x010203f4);

  TEST1 (30700, "01020304", ">I", 4, 0x01020304);
  TEST1 (30710, "f1020304", ">I", 4, 0xf1020304);
  TEST1 (30720, "010203f4", ">I", 4, 0x010203f4);

  TEST1 (30800, "01020304", ">l", 4, 0x01020304);
  TEST1 (30810, "f1020304", ">l", 4, 0xf1020304);
  TEST1 (30820, "010203f4", ">l", 4, 0x010203f4);

  TEST1 (30900, "01020304", ">L", 4, 0x01020304);
  TEST1 (30910, "f1020304", ">L", 4, 0xf1020304);
  TEST1 (30920, "010203f4", ">L", 4, 0x010203f4);

  /* these should always be native size and endianness */
  /* CHECKME: do floats not get promoted to doubles in vararg functions?! */
  MEMTEST1 (31000, ">f", sizeof(float),  23.43);
  MEMTEST1 (31010, ">d", sizeof(double), 43.21);
  
  TEST3 (31200, "00000102", ">bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (31210, "0102030405060708", ">ii", 8, 0x01020304, 0x05060708);
  TEST2 (31220, "0102030405060708", ">2i", 8, 0x01020304, 0x05060708);
}
GNET_END_TEST;

/*** NATIVE MODE, NATIVE SIZES ***/

GNET_START_TEST (test_pack_native_common)
{
  TEST0 (10100, "00", "x", 1);

  TEST1 (10200, "17", "b", 1, 0x17);
  TEST1 (10210, "f1", "b", 1, 0xf1);

  TEST1 (10300, "17", "B", 1, 0x17);
  TEST1 (10310, "f1", "B", 1, 0xf1);

  MEMTEST1 (11000, "f", sizeof(float),  23.43);
  MEMTEST1 (11010, "d", sizeof(double), 43.21);
}

GNET_END_TEST;


GNET_START_TEST (test_pack_native_little_endian)
{
  /* NATIVE LITTLE ENDIAN */

  TEST1 (10400, "0201", "h", sizeof(short), 0x0102);
  TEST1 (10410, "01f0", "h", sizeof(short), 0xf001);
  TEST1 (10420, "f001", "h", sizeof(short), 0x01f0);

  TEST1 (10500, "0201", "H", sizeof(short), 0x0102);
  TEST1 (10510, "01f0", "H", sizeof(short), 0xf001);
  TEST1 (10520, "f001", "H", sizeof(short), 0x01f0);

  TEST1 (10600, "04030201", "i", sizeof(int), 0x01020304);
  TEST1 (10610, "040302f1", "i", sizeof(int), 0xf1020304);
  TEST1 (10620, "f4030201", "i", sizeof(int), 0x010203f4);

  TEST1 (10700, "04030201", "I", sizeof(unsigned int), 0x01020304);
  TEST1 (10710, "040302f1", "I", sizeof(unsigned int), 0xf1020304);
  TEST1 (10720, "f4030201", "I", sizeof(unsigned int), 0x010203f4);

  /* 'native mode' - should replicate full size of long, ie. 4 or 8 bytes
   * depending on architecture */
  if (sizeof (long) == 4) {
    TEST1 (10800, "04030201", "l", sizeof (glong), (glong) 0x01020304);
    TEST1 (10810, "040302f1", "l", sizeof (glong), (glong) 0xf1020304);
    TEST1 (10820, "f4030201", "l", sizeof (glong), (glong) 0x010203f4);
  
    TEST1 (10900, "04030201", "L", sizeof (gulong), (gulong) 0x01020304);
    TEST1 (10910, "040302f1", "L", sizeof (gulong), (gulong) 0xf1020304);
    TEST1 (10920, "f4030201", "L", sizeof (gulong), (gulong) 0x010203f4);
  } else  if (sizeof (long) == 8) {
    TEST1 (10800, "0403020100000000", "l", sizeof (glong), (glong) 0x01020304);
    TEST1 (10810, "040302f100000000", "l", sizeof (glong), (glong) 0xf1020304);
    TEST1 (10820, "f403020100000000", "l", sizeof (glong), (glong) 0x010203f4);
  
    TEST1 (10900, "0403020100000000", "L", sizeof (gulong), (gulong) 0x01020304);
    TEST1 (10910, "040302f100000000", "L", sizeof (gulong), (gulong) 0xf1020304);
    TEST1 (10920, "f403020100000000", "L", sizeof (gulong), (gulong) 0x010203f4);
  }

  /* in little-endian or big-endian 'standard mode' the sizes are independent
   * of the architecture (ie. always 4 bytes for long); we know we're little
   * endian here */
  TEST1 (10800, "04030201", "<l", sizeof (guint32), (glong) 0x01020304);
  TEST1 (10810, "040302f1", "<l", sizeof (guint32), (glong) 0xf1020304);
  TEST1 (10820, "f4030201", "<l", sizeof (guint32), (glong) 0x010203f4);
  
  TEST1 (10900, "04030201", "<L", sizeof (guint32), (gulong) 0x01020304);
  TEST1 (10910, "040302f1", "<L", sizeof (guint32), (gulong) 0xf1020304);
  TEST1 (10920, "f4030201", "<L", sizeof (guint32), (gulong) 0x010203f4);

  /* pointers are always in native size, but endianness should still matter
   * (since it hasn't been documented not to be so) */
#if SIZEOF_VOIDP == 4
  TEST1 (11100, "04030201", "v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "040302f1", "v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "f4030201", "v", sizeof(void*), PTR_CONSTANT (0x010203f4));
  /* with little endian modifier */
  TEST1 (11100, "04030201", "<v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "040302f1", "<v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "f4030201", "<v", sizeof(void*), PTR_CONSTANT (0x010203f4));
  /* with big endian modifier */
  TEST1 (11100, "01020304", ">v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "f1020304", ">v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "010203f4", ">v", sizeof(void*), PTR_CONSTANT (0x010203f4));
#elif SIZEOF_VOIDP == 8
  TEST1 (11100, "0807060504030201", "v", sizeof(void*), PTR_CONSTANT (0x0102030405060708));
  TEST1 (11110, "08070605040302f1", "v", sizeof(void*), PTR_CONSTANT (0xf102030405060708));
  TEST1 (11120, "f8070605f4030201", "v", sizeof(void*), PTR_CONSTANT (0x010203f4050607f8));
  /* with little endian modifier */
  TEST1 (11100, "0807060504030201", "<v", sizeof(void*), PTR_CONSTANT (0x0102030405060708));
  TEST1 (11110, "08070605040302f1", "<v", sizeof(void*), PTR_CONSTANT (0xf102030405060708));
  TEST1 (11120, "f8070605f4030201", "<v", sizeof(void*), PTR_CONSTANT (0x010203f4050607f8));
  /* with big endian modifier */
  TEST1 (11100, "0102030405060708", ">v", sizeof(void*), PTR_CONSTANT (0x0102030405060708));
  TEST1 (11110, "f102030405060708", ">v", sizeof(void*), PTR_CONSTANT (0xf102030405060708));
  TEST1 (11120, "010203f4050607f8", ">v", sizeof(void*), PTR_CONSTANT (0x010203f4050607f8));
#else
# error "sizeof(void *) neither 4 nor 8!?"
#endif

  TEST3 (11200, "00010002",         "bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (11210, "0403020108070605", "ii",  8, 0x01020304, 0x05060708);
  TEST2 (11220, "0403020108070605", "2i",  8, 0x01020304, 0x05060708);
}
GNET_END_TEST;

GNET_START_TEST (test_pack_native_big_endian)
{
  /* NATIVE BIG ENDIAN */

  TEST1 (10400, "0102", "h", sizeof(short), 0x0102);
  TEST1 (10410, "f001", "h", sizeof(short), 0xf001);
  TEST1 (10420, "01f0", "h", sizeof(short), 0x01f0);

  TEST1 (10500, "0102", "H", sizeof(short), 0x0102);
  TEST1 (10510, "f001", "H", sizeof(short), 0xf001);
  TEST1 (10520, "01f0", "H", sizeof(short), 0x01f0);

  TEST1 (10600, "01020304", "i", sizeof(int), 0x01020304);
  TEST1 (10610, "f1020304", "i", sizeof(int), 0xf1020304);
  TEST1 (10620, "010203f4", "i", sizeof(int), 0x010203f4);

  TEST1 (10700, "01020304", "I", sizeof(unsigned int), 0x01020304);
  TEST1 (10710, "f1020304", "I", sizeof(unsigned int), 0xf1020304);
  TEST1 (10720, "010203f4", "I", sizeof(unsigned int), 0x010203f4);

  /* 'native mode' - should replicate full size of long, ie. 4 or 8 bytes
   * depending on architecture */
  if (sizeof (long) == 4) {
    TEST1 (10800, "01020304", "l", sizeof (glong), (glong) 0x01020304);
    TEST1 (10810, "f1020304", "l", sizeof (glong), (glong) 0xf1020304);
    TEST1 (10820, "010203f4", "l", sizeof (glong), (glong) 0x010203f4);

    TEST1 (10900, "01020304", "L", sizeof (gulong), (gulong) 0x01020304);
    TEST1 (10910, "f1020304", "L", sizeof (gulong), (gulong) 0xf1020304);
    TEST1 (10920, "010203f4", "L", sizeof (gulong), (gulong) 0x010203f4);
  } else  if (sizeof (long) == 8) {
    TEST1 (10800, "0000000001020304", "l", sizeof (glong), (glong) 0x01020304);
    TEST1 (10810, "00000000f1020304", "l", sizeof (glong), (glong) 0xf1020304);
    TEST1 (10820, "00000000010203f4", "l", sizeof (glong), (glong) 0x010203f4);

    TEST1 (10900, "0000000001020304", "L", sizeof (gulong), (gulong) 0x01020304);
    TEST1 (10910, "00000000f1020304", "L", sizeof (gulong), (gulong) 0xf1020304);
    TEST1 (10920, "00000000010203f4", "L", sizeof (gulong), (gulong) 0x010203f4);
  }

  /* in little-endian or big-endian 'standard mode' the sizes are independent
   * of the architecture (ie. always 4 bytes for long); we know we're little
   * endian here */
  TEST1 (10800, "01020304", ">l", sizeof (guint32), (glong) 0x01020304);
  TEST1 (10810, "f1020304", ">l", sizeof (guint32), (glong) 0xf1020304);
  TEST1 (10820, "010203f4", ">l", sizeof (guint32), (glong) 0x010203f4);
  
  TEST1 (10900, "01020304", ">L", sizeof (guint32), (gulong) 0x01020304);
  TEST1 (10910, "f1020304", ">L", sizeof (guint32), (gulong) 0xf1020304);
  TEST1 (10920, "010203f4", ">L", sizeof (guint32), (gulong) 0x010203f4);

  /* pointers are always in native size, but endianness modifiers should
   * still work (as it hasn't been documented otherwise) */
#if SIZEOF_VOIDP == 4
  TEST1 (11100, "01020304", "v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "f1020304", "v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "010203f4", "v", sizeof(void*), PTR_CONSTANT (0x010203f4));
  /* with little endian modifier */
  TEST1 (11100, "04030201", "<v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "040302f1", "<v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "f4030201", "<v", sizeof(void*), PTR_CONSTANT (0x010203f4));
  /* with big endian modifier */
  TEST1 (11100, "01020304", ">v", sizeof(void*), PTR_CONSTANT (0x01020304));
  TEST1 (11110, "f1020304", ">v", sizeof(void*), PTR_CONSTANT (0xf1020304));
  TEST1 (11120, "010203f4", ">v", sizeof(void*), PTR_CONSTANT (0x010203f4));
#elif SIZEOF_VOIDP == 8
  TEST1 (11100, "0102030405060708", "v", sizeof(void*), PTR_CONSTANT(0x0102030405060708));
  TEST1 (11110, "f1020304f5f6f7f8", "v", sizeof(void*), PTR_CONSTANT(0xf1020304f5f6f7f8));
  TEST1 (11120, "010203f4050607f8", "v", sizeof(void*), PTR_CONSTANT(0x010203f4050607f8));
  /* with little endian modifier */
  TEST1 (11100, "0807060504030201", "<v", sizeof(void*), PTR_CONSTANT(0x0102030405060708));
  TEST1 (11110, "f8f7f6f504030201", "<v", sizeof(void*), PTR_CONSTANT(0xf1020304f5f6f7f8));
  TEST1 (11120, "f8070605f4030201", "<v", sizeof(void*), PTR_CONSTANT(0x010203f4050607f8));
  /* with big endian modifier */
  TEST1 (11100, "0102030405060708", ">v", sizeof(void*), PTR_CONSTANT(0x0102030405060708));
  TEST1 (11110, "f1020304f5f6f7f8", ">v", sizeof(void*), PTR_CONSTANT(0xf1020304f5f6f7f8));
  TEST1 (11120, "010203f4050607f8", ">v", sizeof(void*), PTR_CONSTANT(0x010203f4050607f8));
#else
# error "sizeof(void *) neither 4 nor 8!?"
#endif

  TEST3 (11200, "00000102", "bhb", 4,  0x00, 0x0001, 0x02);
  TEST2 (11201, "0102030405060708", "ii", 8, 0x01020304, 0x05060708);
  TEST2 (11202, "0102030405060708", "2i", 8, 0x01020304, 0x05060708);
}
GNET_END_TEST;

static Suite *
gnetpack_suite (void)
{
  Suite *s = suite_create ("GNetPacking");
  TCase *tc_chain = tcase_create ("packing");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_pack_strings);
  tcase_add_test (tc_chain, test_pack_native_common);
  if (G_BYTE_ORDER == G_LITTLE_ENDIAN) {
    tcase_add_test (tc_chain, test_pack_native_little_endian);
  } else {
    tcase_add_test (tc_chain, test_pack_native_big_endian);
  }
  tcase_add_test (tc_chain, test_pack_big_endian);
  tcase_add_test (tc_chain, test_pack_little_endian);
  tcase_add_test (tc_chain, test_pack_failures);

  return s;
}

GNET_CHECK_MAIN (gnetpack);

