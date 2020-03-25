/* GNet Base64 unit test (deterministic, computation-based tests only)
 * Copyright (C) 2003 Alfred Reibenschuh
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

#include "gnetcheck.h"

const gchar sesame[] =
    "Aladdin:open sesame and many many more so come here and see the gold";
const gchar aladin[] =
    "QWxhZGRpbjpvcGVuIHNlc2FtZSBhbmQgbWFueSBtYW55IG1vcmUgc28gY29tZSBoZXJlIGFuZCBzZWUgdGhlIGdvbGQ=";
const gchar aladin_cr[] =
    "QWxhZGRpbjpvcGVuIHNlc2FtZSBhbmQgbWFueSBtYW55IG1vcmUgc28gY29tZSBoZXJlIGFu\n"
    "ZCBzZWUgdGhlIGdvbGQ=";

GNET_START_TEST (test_base64)
{
  gchar *buf1;
  gchar *buf2;
  gint  len1, len2;

  ASSERT_CRITICAL (gnet_base64_encode (NULL, 1, &len1, FALSE));
  ASSERT_CRITICAL (gnet_base64_encode (sesame, 1, NULL, FALSE));
  ASSERT_CRITICAL (gnet_base64_encode (sesame, -1, NULL, FALSE));
  ASSERT_CRITICAL (gnet_base64_decode (NULL, 1, &len1));
  ASSERT_CRITICAL (gnet_base64_decode (aladin, 1, NULL));

  len1 = 0;
  buf1 = gnet_base64_encode (sesame, strlen (sesame), &len1, FALSE);
  fail_unless (buf1 != NULL);
  /* length includes the final \0 */
  fail_unless_equals_int (len1, strlen (aladin) + 1);
  fail_unless_equals_string (buf1, aladin);
  g_free (buf1);

  len1 = 0; 
  buf1 = gnet_base64_encode (sesame, strlen (sesame), &len1, TRUE);
  fail_unless (buf1 != NULL);
  /* length includes the final \0 */
  fail_unless_equals_int (len1, strlen (aladin_cr) + 1);
  fail_unless_equals_string (buf1, aladin_cr);
  g_free (buf1);

  len2 = 0; 
  buf2 = gnet_base64_decode (aladin, strlen (aladin), &len2);
  fail_unless (buf2 != NULL);
  fail_unless_equals_int (len2, strlen (sesame));
  fail_unless_equals_string (buf2, sesame);
  g_free (buf2);

  /* and the same with -1 as length */
  len2 = 0; 
  buf2 = gnet_base64_decode (aladin, -1, &len2);
  fail_unless (buf2 != NULL);
  fail_unless_equals_int (len2, strlen (sesame));
  fail_unless_equals_string (buf2, sesame);
  g_free (buf2);

  len2 = 0; 
  buf2 = gnet_base64_decode (aladin_cr, strlen (aladin_cr), &len2);
  fail_unless (buf2 != NULL);
  fail_unless_equals_int (len2, strlen (sesame));
  fail_unless_equals_string (buf2, sesame);
  g_free (buf2);

  /* and the same with -1 as length */
  len2 = 0; 
  buf2 = gnet_base64_decode (aladin_cr, -1, &len2);
  fail_unless (buf2 != NULL);
  fail_unless_equals_int (len2, strlen (sesame));
  fail_unless_equals_string (buf2, sesame);
  g_free (buf2);
}

GNET_END_TEST;

/* to test for bug reported on mailing list around 24 Oct 2004: */
GNET_START_TEST (test_base64_8bit)
{
  gchar *buf1;
  gchar *buf2;
  gint  len1, len2;

  /* encode ... */
  len2 = 0; 
  buf2 = gnet_base64_encode ("A\x84\0", 3, &len2, TRUE);
  fail_unless (buf2 != NULL);
  fail_unless_equals_int (len2, strlen ("QYQA") + 1);
  fail_unless_equals_string (buf2, "QYQA");

  /* ... and decode */
  len1 = 0;
  buf1 = gnet_base64_decode (buf2, len2, &len1);
  fail_unless (buf1 != NULL);
  fail_unless_equals_int (len1, 3);
  fail_unless (memcmp (buf1, "A\x84\0", 3) == 0);
  g_free (buf1);
  g_free (buf2);
}

GNET_END_TEST;

static Suite *
gnetbase64_suite (void)
{
  Suite *s = suite_create ("GNetBase64");
  TCase *tc_chain = tcase_create ("base64");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_base64);
  tcase_add_test (tc_chain, test_base64_8bit);
  return s;
}

GNET_CHECK_MAIN (gnetbase64);

