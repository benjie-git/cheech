/* GNet URI parser unit test
 * Copyright (C) 2001 David Helder
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

#include <string.h>

#if 0
#define SAFESTRCMP(A,B) (((A)&&(B))?(strcmp((A),(B))):((A)||(B)))
#endif

GNET_START_TEST (test_uri_errors)
{
  GURI guri;
  gchar *uri;

  /* Empty string is an error */
  fail_unless (gnet_uri_new("") == NULL);
  uri = g_strdup ("");
  fail_unless (!gnet_uri_parse_inplace (&guri, uri, NULL, 0));
  g_free (uri);

  /* String of whitespace is an error */
  fail_unless (gnet_uri_new(" \n\t\r") == NULL);
  uri = g_strdup (" \n\t\r");
  fail_unless (!gnet_uri_parse_inplace (&guri, uri, NULL, 0));
  g_free (uri);
}

GNET_END_TEST;

/*** test parsing */

struct URITest
{
  gchar* str;
  gchar* pretty;
  struct
  {
    gchar* scheme;
    gchar* userinfo;	
    gchar* hostname;
    gint   port;
    gchar* path;
    gchar* query;
    gchar* fragment;
  } uri;
};

struct URITest tests[] = 
{
  /* VALID URIS.  PARSING AND PRINTING OF THESE SHOULD NOT CHANGE */

  /* scheme/path */
  { "scheme:", NULL, 
    {"scheme", NULL, NULL, 0, NULL, NULL, NULL}},

  { "scheme:path", NULL, 
    {"scheme", NULL, NULL, 0, "path", NULL, NULL}},

  { "path", NULL, 
    {NULL, NULL, NULL, 0, "path", NULL, NULL}},

  { "/path", NULL, 
    {NULL, NULL, NULL, 0, "/path", NULL, NULL}},

  /* hostname/port */
  { "scheme://hostname/path", NULL, 
    {"scheme", NULL, "hostname", 0, "/path", NULL, NULL}},

  { "scheme://hostname:123/path", NULL, 
    {"scheme", NULL, "hostname", 123, "/path", NULL, NULL}},

  /* ipv6 hostname/port */
  { "scheme://[01:23:45:67:89:ab:cd:ef]/path", NULL, 
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef", 0, "/path", NULL, NULL}},

  { "scheme://[01:23:45:67:89:ab:cd:ef]:123/path", NULL, 
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef", 123, "/path", NULL, NULL}},

  /* query/fragment */
  { "path?query", NULL, 
    {NULL, NULL, NULL, 0, "path", "query", NULL}},

  { "path?query#fragment", NULL, 
    {NULL, NULL, NULL, 0, "path", "query", "fragment"}},

  { "scheme:path?query#fragment", NULL, 
    {"scheme", NULL, NULL, 0, "path", "query", "fragment"}},

  /* full */
  { "scheme://hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, "hostname", 123, "/path", "query", "fragment"}},

  /* user/pass */
  { "scheme://userinfo@hostname", NULL, 
    {"scheme", "userinfo", "hostname", 0, NULL, NULL, NULL }},

  { "scheme://userinfo@hostname:123/path?query#fragment", NULL, 
    {"scheme", "userinfo", "hostname", 123, "/path", "query", "fragment"}},

  { "scheme://user:pass@hostname", NULL, 
    {"scheme", "user:pass", "hostname", 0, NULL, NULL, NULL}},

  { "scheme://user:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", "user:pass", "hostname", 123, "/path", "query", "fragment"}},

  /* FUNNY URIS.  PARSING AND PRINTING OF THESE MAY CHANGE */

  { "scheme://hostname:123path?query#fragment", 
    "scheme://hostname:123/path?query#fragment",  /* PRETTY */
    {"scheme", NULL, "hostname", 123, "path", "query", "fragment"}},

  { "scheme:hostname:123/path?query#fragment", NULL, 
    {"scheme", NULL, NULL, 0, "hostname:123/path", "query", "fragment"}},

  { "scheme://:pass@hostname:123/path?query#fragment", NULL, 
    {"scheme", ":pass", "hostname", 123, "/path", "query", "fragment"}},

  /* IPv6 hostname without brackets */
  { "scheme://01:23:45:67:89:ab:cd:ef:123/path", 
    "scheme://01:23/:45:67:89:ab:cd:ef:123/path",  /* PRETTY */
    {"scheme", NULL, "01", 23, ":45:67:89:ab:cd:ef:123/path", NULL, NULL}},

  /* Brackets that don't close - hostname will be everything */
  { "scheme://[01:23:45:67:89:ab:cd:ef:123/path", 
    "scheme://[01:23:45:67:89:ab:cd:ef:123/path]",
    {"scheme", NULL, "01:23:45:67:89:ab:cd:ef:123/path", 0, NULL, NULL, NULL}},

  /* Skip initial white space */
  { " \f\n\r\t\vscheme:", "scheme:", 
    {"scheme", NULL, NULL, 0, NULL, NULL, NULL}},

  { " \f\n\r\t\vpath", "path",
    {NULL, NULL, NULL, 0, "path", NULL, NULL}},

  /* file URI */
  { "file://host/home/joe/foo.txt", NULL, 
    {"file", NULL, "host", 0, "/home/joe/foo.txt", NULL, NULL}},
  { "file:///home/joe/foo.txt", NULL, 
    {"file", NULL, NULL, 0, "/home/joe/foo.txt", NULL, NULL}}
};

static void
test_single_uri (struct URITest * test, GURI * uri,
    gboolean test_escape_unescape)
{
  gchar *pretty;

  if (g_getenv ("GNET_DEBUG")) {
    g_printerr ("%s: uri='%s'\n", __FUNCTION__, test->str);
  }

  pretty = gnet_uri_get_string (uri);
  fail_unless (pretty != NULL, "no pretty string for '%s'", test->str);

  if (test->pretty) {
    fail_unless_equals_string (pretty, test->pretty);
  } else {
    fail_unless_equals_string (pretty, test->str);
  }

#define fail_unless_equals_string_safe(a,b) \
  { \
     fail_unless (((a) != NULL && (b) != NULL) || ((a) == NULL && (b) == NULL));  \
     if ((a) && (b)) {                                                            \
       fail_unless_equals_string ((a), (b));                                      \
     }                                                                            \
  }

  fail_unless_equals_string_safe (uri->scheme, test->uri.scheme);
  fail_unless_equals_string_safe (uri->userinfo, test->uri.userinfo);
  fail_unless_equals_string_safe (uri->hostname, test->uri.hostname);
  fail_unless_equals_string_safe (uri->scheme, test->uri.scheme);
  fail_unless_equals_int (uri->port, test->uri.port);
  fail_unless_equals_string_safe (uri->path, test->uri.path);
  fail_unless_equals_string_safe (uri->query, test->uri.query);
  fail_unless_equals_string_safe (uri->fragment, test->uri.fragment);

  if (test_escape_unescape) {
    gchar *unescape;
    gchar *escape;

    gnet_uri_escape (uri);
    escape = gnet_uri_get_string (uri);
    fail_unless (escape != NULL,
        "gnet_uri_escape() failed for %s", test->str);

    gnet_uri_unescape (uri);
    unescape = gnet_uri_get_string (uri);
    fail_unless (unescape != NULL,
        "gnet_uri_unescape() failed for %s after escape", test->str);
      
    fail_unless_equals_string (pretty, unescape);
    g_free (escape);
    g_free (unescape);
  }

  g_free (pretty);
}

GNET_START_TEST (test_uri_parsing)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (tests); ++i) {
    /* first the normal heap allocated one */
    {
      GURI *uri;

      uri = gnet_uri_new (tests[i].str);
      fail_unless (uri != NULL, "gnet_uri_new(%s) failed", tests[i].str);
      test_single_uri (&tests[i], uri, TRUE);
      gnet_uri_delete (uri);
    }

    /* and the same with a stack-allocated one */
    {
      GURI uri;
      gchar hostname[100], *uricpy;

      uricpy = g_strdup (tests[i].str);
      fail_unless (gnet_uri_parse_inplace (&uri, uricpy, hostname, 100),
          "gnet_uri_parse_inplace(%s) failed", tests[i].str);
      test_single_uri (&tests[i], &uri, FALSE);
      g_free (uricpy);
    }
  }
}

GNET_END_TEST;

/*** test escaping ***/

struct {
  gchar* escaped;
  gchar* unescaped;
  gchar* escaped2;
} escape_tests[] =  {
  { "http://userinfo@www.example.com:80/path?query#fragment",
    "http://userinfo@www.example.com:80/path?query#fragment" , NULL},
  { "http://userinfo@www.example.com:80/~path?query#fragment",
    "http://userinfo@www.example.com:80/~path?query#fragment" , NULL},
  { "http://%5euser%5einfo%5e@www.example.com:80/~%5epa%5eth%5e?%5equ%5eery%5e#%5efra%5egment%5e",
    "http://^user^info^@www.example.com:80/~^pa^th^?^qu^ery^#^fra^gment^" , NULL},
  { "http://%5e%5e%5euser%5e%5e%5einfo%5e%5e%5e@www.example.com:80/~%5e%5e%5epa%5e%5e%5eth%5e%5e%5e?%5e%5e%5equ%5e%5e%5eery%5e%5e%5e#%5e%5e%5efra%5e%5e%5egment%5e%5e%5e",
    "http://^^^user^^^info^^^@www.example.com:80/~^^^pa^^^th^^^?^^^qu^^^ery^^^#^^^fra^^^gment^^^" , NULL},
  { "http://user%40info@www.example.com:80/path?query#fragment",
    "http://user@info@www.example.com:80/path?query#fragment" , NULL},
  { "http://user%40info@www.example.com:80/path?query#fragment",
    "http://user@info@www.example.com:80/path?query#fragment" , NULL},

  { "http://www.example.com/pa%th", "http://www.example.com/pa%th",
    "http://www.example.com/pa%25th"},
  { "http://www.example.com/%e9%e9.html",
    "http://www.example.com/\xe9\xe9.html", NULL }
};

GNET_START_TEST (test_uri_escaping)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (escape_tests); ++i) {
    gchar *escape;
    gchar *unescape;
    GURI *uri;

    uri = gnet_uri_new (escape_tests[i].escaped);
    fail_unless (uri != NULL, "gnet_uri_new(%s) failed",
        escape_tests[i].escaped);

    gnet_uri_unescape (uri);
    unescape = gnet_uri_get_string (uri);
    fail_unless (unescape != NULL,
        "Couldn't unescape '%s'", escape_tests[i].escaped);
    fail_unless_equals_string (unescape, escape_tests[i].unescaped);
    g_free (unescape);

    gnet_uri_escape (uri);
    escape = gnet_uri_get_string (uri);
    fail_unless (escape != NULL,
        "Couldn't re-escape '%s'", escape_tests[i].unescaped);

    /* and check that the escape is correct */
    if (escape_tests[i].escaped2) {
      fail_unless_equals_string (escape, escape_tests[i].escaped2);
    } else {
      fail_unless_equals_string (escape, escape_tests[i].escaped);
    }

    g_free (escape);
    gnet_uri_delete (uri);
  }
}

GNET_END_TEST;

static Suite *
gneturi_suite (void)
{
  Suite *s = suite_create ("GURI");
  TCase *tc_chain = tcase_create ("uri");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_uri_errors);
  tcase_add_test (tc_chain, test_uri_escaping);
  tcase_add_test (tc_chain, test_uri_parsing);

  return s;
}

GNET_CHECK_MAIN (gneturi);

