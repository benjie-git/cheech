/* GNet GInetAddr unit test (deterministic, computation-based tests only)
 * Copyright (C) 2002 David Helder
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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

#ifdef HAVE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <string.h>
#include <stdio.h>

/*** IPv4 tests */

GNET_START_TEST (test_inetaddr_ipv4)
{
  GInetAddr* inetaddr;
  GInetAddr* inetaddr2;
  gchar* cname;
  gchar* cname2;
  gchar bytes[GNET_INETADDR_MAX_LEN];

  /* new (canonical, IPv4) */
  inetaddr = gnet_inetaddr_new ("141.213.11.124", 23);
  fail_unless (inetaddr != NULL);
  
  /* get_port */
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string (cname, "141.213.11.124");
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  fail_unless (inetaddr2 != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  fail_unless (cname2 != NULL);
  fail_unless_equals_string (cname2, "141.213.11.124");
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr2), 42);
  g_free(cname2);

  /* equal, noport_equal */
  fail_unless (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  fail_if (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  fail_unless_equals_int (gnet_inetaddr_hash (inetaddr), 2379549526u);
  /* hash port */
  fail_unless (gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x8d\xd5\xb\x7c", 4);
  fail_unless (inetaddr != NULL);

  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string ("141.213.11.124", cname);
  g_free (cname);

  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 0);
  gnet_inetaddr_set_port (inetaddr, 2345);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);

  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 4);

  gnet_inetaddr_set_bytes (inetaddr, "\x7c\xb\xd5\x8d", 4);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless_equals_string ("124.11.213.141", cname);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);
  g_free (cname);

  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 4);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  fail_unless (memcmp(bytes, "\x7c\xb\xd5\x8d", 4) == 0);

#ifdef HAVE_IPV6

  /* IPv4 -> IPv6 via set_bytes */
  gnet_inetaddr_set_bytes (inetaddr, "\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
			   "\0\0\0\0"         "\0\0\0\x6f", 16);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  g_free (cname);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 2345);
  fail_unless_equals_int (gnet_inetaddr_get_length (inetaddr), 16);

  gnet_inetaddr_get_bytes (inetaddr, bytes);
  fail_unless (!memcmp(bytes, "\x3f\xfe\x0b\x00\x0c\x18\x1f\xff\0\0\0\0\0\0\0\x6f", 16));

#endif

  gnet_inetaddr_delete (inetaddr);
}
GNET_END_TEST;

#define IS_TEST(S,A,F) do {                                     \
  GInetAddr* _inetaddr;                                         \
  gchar* _cname;                                                \
  _inetaddr = gnet_inetaddr_new_nonblock (A, 0);                \
  fail_unless (_inetaddr != NULL);                              \
  _cname = gnet_inetaddr_get_canonical_name (_inetaddr);        \
  fail_unless (_cname != NULL);                                 \
  fail_unless_equals_string (_cname, (A));                      \
  fail_unless (F (_inetaddr));                                  \
  g_free (_cname);                                              \
  gnet_inetaddr_delete (_inetaddr);                             \
} while (0)

GNET_START_TEST (test_inetaddr_is_ipv4)
{
  /* IPv4 */
  IS_TEST ("IPv4", "141.213.11.124", 	     	  gnet_inetaddr_is_ipv4);

  /* IPv4 Loopback */
  IS_TEST ("IPv4 Loopback",  "127.0.0.1",     	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 Loopback2", "127.23.42.129", 	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv4 !Loopback", "128.23.42.129", 	  !gnet_inetaddr_is_loopback);

  /* IPv4 Multicast */
  IS_TEST ("IPv4 Multicast",   "224.0.0.0",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast2",  "224.0.0.1",       gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 Multicast3",  "239.255.255.255", gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast",  "223.255.255.255", !gnet_inetaddr_is_multicast);
  IS_TEST ("IPv4 !Multicast2", "240.0.0.0", 	  !gnet_inetaddr_is_multicast);

  /* IPv4 Private */
  IS_TEST ("IPv4 Private",   	"10.0.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private2",   	"10.255.255.255",  gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private",   	"9.255.255.255",   !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private2",   	"11.0.0.0", 	   !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private3",   	"172.16.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private4",   	"172.31.255.255",  gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private3",   	"172.15.255.255",  !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private4",   	"172.32.0.0",      !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private5",   	"192.168.0.0", 	   gnet_inetaddr_is_private);
  IS_TEST ("IPv4 Private6",   	"192.168.255.255", gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private5",   	"192.167.255.255", !gnet_inetaddr_is_private);
  IS_TEST ("IPv4 !Private6",   	"192.169.0.0",     !gnet_inetaddr_is_private);


  /* IPv4 Reserved */
  IS_TEST ("IPv4 Reserved",   	"0.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved2",   	"0.0.255.255", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved2",   "1.0.0.0", 	   !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved3",   	"240.0.0.0", 	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 Reserved4",   	"247.255.255.255", gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved3",   "239.255.255.255", !gnet_inetaddr_is_reserved);
  IS_TEST ("IPv4 !Reserved4",   "248.0.0.0", 	   !gnet_inetaddr_is_reserved);

  /* Internet */
  IS_TEST ("Internet1", "141.213.11.124", 	   gnet_inetaddr_is_internet);
}
GNET_END_TEST;

#if HAVE_IPV6

GNET_START_TEST (test_inetaddr_is_ipv6)
{
  /* IPv6 */
  IS_TEST ("!IPv4", "3ffe:b00:c18:1fff::6f", 	  !gnet_inetaddr_is_ipv4);
  IS_TEST ("IPv6", "3ffe:b00:c18:1fff::6f",  	  gnet_inetaddr_is_ipv6);
  IS_TEST ("!IPv6", "141.213.11.124", 	     	  !gnet_inetaddr_is_ipv6);

  /* IPv6 Loopback */
  IS_TEST ("IPv6 Loopback",   "::1", 	      	  gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback",  "::",           	  !gnet_inetaddr_is_loopback);
  IS_TEST ("IPv6 !Loopback2", "::201",        	  !gnet_inetaddr_is_loopback);

  /* IPv6 Multicast */
  IS_TEST ("IPv6 Multicast",   "ffff::1",         gnet_inetaddr_is_multicast);
  IS_TEST ("IPv6 !Multicast",  "feff::1",         !gnet_inetaddr_is_multicast);

  /* IPv6 Broadcast */
  IS_TEST ("IPv6 Broadcast",   "255.255.255.255", gnet_inetaddr_is_broadcast);
  IS_TEST ("IPv6 !Broadcast",  "255.255.255.254", !gnet_inetaddr_is_broadcast);

  /* IPv6 Private */
  IS_TEST ("IPv6 Private",   	"fe80::",     	   gnet_inetaddr_is_private);
  IS_TEST ("IPv6 Private2",   	"fecf:ffff::",     gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private",   	"fe7f:ffff::",     !gnet_inetaddr_is_private);
  IS_TEST ("IPv6 !Private2",   	"ff00::",          !gnet_inetaddr_is_private);

  /* IPv6 Reserved */
  IS_TEST ("IPv6 Reserved",   	"::",     	   gnet_inetaddr_is_reserved);
  IS_TEST ("IPv6 !Reserved",   	"1::",     	   !gnet_inetaddr_is_reserved);

  /* Internet */
  IS_TEST ("Internet2", "3ffe:b00:c18:1fff::6f",   gnet_inetaddr_is_internet);
  IS_TEST ("!Internet1",  "255.255.255.255",       !gnet_inetaddr_is_internet);
  IS_TEST ("!Internet2",  "ffff::1",     	   !gnet_inetaddr_is_internet);
}
GNET_END_TEST;

#endif /* HAVE_IPV6 */


/*** IPv6 tests ***/

#if HAVE_IPV6
GNET_START_TEST (test_inetaddr_ipv6)
{
  GInetAddr* inetaddr;
  GInetAddr* inetaddr2;
  gchar* cname;
  gchar* cname2;

  /* new (canonical, IPv6) */
  inetaddr = gnet_inetaddr_new ("3ffe:b00:c18:1fff::6f", 23);
  fail_unless (inetaddr != NULL);
  
  /* get_port */
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 23);

  /* set_port */
  gnet_inetaddr_set_port (inetaddr, 42);
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 42);

  /* get_canonical_name */
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp(cname, "3ffe:b00:c18:1fff::6f"));
  g_free (cname);

  /* clone */
  inetaddr2 = gnet_inetaddr_clone (inetaddr);
  fail_unless (inetaddr != NULL);
  cname2 = gnet_inetaddr_get_canonical_name (inetaddr2);
  fail_unless (cname2 != NULL);
  fail_unless (!strcasecmp(cname2, "3ffe:b00:c18:1fff::6f"));
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr2), 42);
  g_free(cname2);

  /* equal, noport_equal */
  fail_unless (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  gnet_inetaddr_set_port (inetaddr2, 23);
  fail_if (gnet_inetaddr_equal (inetaddr, inetaddr2));
  fail_unless (gnet_inetaddr_noport_equal (inetaddr, inetaddr2));
  
  /* hash */
  fail_unless_equals_int (gnet_inetaddr_hash (inetaddr), 870716602u);
  /* hash port */
  fail_unless (gnet_inetaddr_hash (inetaddr) != gnet_inetaddr_hash (inetaddr2));

  gnet_inetaddr_delete (inetaddr);
  gnet_inetaddr_delete (inetaddr2);

  /* bytes */
  inetaddr = gnet_inetaddr_new_bytes ("\x3f\xfe\x0b\x00" "\x0c\x18\x1f\xff"
				      "\0\0\0\0"         "\0\0\0\x6f", 16);
  fail_unless (inetaddr != NULL);
  cname = gnet_inetaddr_get_canonical_name (inetaddr);
  fail_unless (cname != NULL);
  fail_unless (!strcasecmp("3ffe:b00:c18:1fff::6f", cname));
  fail_unless_equals_int (gnet_inetaddr_get_port (inetaddr), 0);
  gnet_inetaddr_delete (inetaddr);
  g_free (cname);
}
GNET_END_TEST;
#endif /* HAVE_IPV6 */

GNET_START_TEST (test_inetaddr_is_internet_domainname)
{
  fail_unless (gnet_inetaddr_is_internet_domainname ("speak.eecs.umich.edu"));
  fail_unless (gnet_inetaddr_is_internet_domainname ("141.213.11.124"));
  fail_if (gnet_inetaddr_is_internet_domainname ("localhost"));
  fail_if (gnet_inetaddr_is_internet_domainname ("localhost.localdomain"));
  fail_if (gnet_inetaddr_is_internet_domainname ("speak"));
}
GNET_END_TEST;

static void
lookup_list_cb (GList * list, gpointer data)
{
  gboolean *p_called = (gboolean *) data;
  GList *l;

  *p_called = TRUE;

  /* g_print ("%d results\n", g_list_length (list)); */

  fail_unless (list != NULL);
  for (l = list; l != NULL; l = l->next) {
    GInetAddr *ia;
    gchar *name;

    ia = (GInetAddr *) l->data;
    name = gnet_inetaddr_get_canonical_name (ia);
    fail_unless (name != NULL);
    fail_unless_equals_int (gnet_inetaddr_get_port (ia), 80);
    g_free (name);
  }

  g_list_foreach (list, (GFunc) gnet_inetaddr_unref, NULL);
  g_list_free (list);
}

static void
lookup_list_cb_do_nothing (GList * list, gpointer data)
{
  g_list_foreach (list, (GFunc) gnet_inetaddr_unref, NULL);
  g_list_free (list);
}

static gboolean
do_nothing_timeout (gpointer foo)
{
  return FALSE; /* don't call again */
}

static void
destroy_notify (gpointer data)
{
  gboolean *p_bool = (gboolean *) data;

  *p_bool = TRUE;
}

static void
do_test_inetaddr_list_async (const gchar * hostname)
{
  GInetAddrNewListAsyncID list_async_id;
  GMainContext *ctx;
  gboolean called, destroyed;
  gint id;

  /* g_print ("list async: looking up %s ... ", hostname); */

  /* list async (default main context) */
  called = FALSE;
  list_async_id = gnet_inetaddr_new_list_async (hostname, 80,
      lookup_list_cb, &called);
  id = g_timeout_add (5 * 1000, (GSourceFunc) do_nothing_timeout, NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (called);
  g_source_remove (id);

  /* list async + cancel (default main context) */
  called = FALSE;
  list_async_id = gnet_inetaddr_new_list_async (hostname, 80,
      lookup_list_cb, &called);
  fail_unless (list_async_id != NULL);
  gnet_inetaddr_new_list_async_cancel (list_async_id);
  while (g_main_context_iteration (NULL, FALSE)) {
    ;
  }
  fail_unless (!called);

  /* list async (destroy notify) */
  destroyed = FALSE;
  list_async_id = gnet_inetaddr_new_list_async_full (hostname, 80,
      lookup_list_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (list_async_id != NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (destroyed);

  /* list async + cancel (destroy notify; thread unlikely to start) */
  destroyed = FALSE;
  list_async_id = gnet_inetaddr_new_list_async_full (hostname, 80,
      lookup_list_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (list_async_id != NULL);
  gnet_inetaddr_new_list_async_cancel (list_async_id);
  /* give thread a chance to execute and act on the cancelled flag */
  g_usleep (1 * G_USEC_PER_SEC);
  fail_unless (destroyed);

  /* list async + cancel (destroy notify II) */
  destroyed = FALSE;
  list_async_id = gnet_inetaddr_new_list_async_full (hostname, 80,
      lookup_list_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (list_async_id != NULL);
  /* give thread a chance to execute and block before we cancel it */
  g_usleep (1 * G_USEC_PER_SEC);
  gnet_inetaddr_new_list_async_cancel (list_async_id);
  fail_unless (destroyed);

  /* list async + cancel (destroy notify III) */
  destroyed = FALSE;
  list_async_id = gnet_inetaddr_new_list_async_full (hostname, 80,
      lookup_list_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (list_async_id != NULL);
  /* give thread a chance to execute and block and be done before cancelling */
  g_usleep (3 * G_USEC_PER_SEC);
  gnet_inetaddr_new_list_async_cancel (list_async_id);
  fail_unless (destroyed);

  /* list async (custom main context) */
  called = FALSE;
  ctx = g_main_context_new ();
  list_async_id = gnet_inetaddr_new_list_async_full (hostname, 80,
      lookup_list_cb, &called, (GDestroyNotify) NULL, ctx,
      G_PRIORITY_HIGH);
  /* give thread a chance to execute and block and be done */
  g_usleep (3 * G_USEC_PER_SEC);
  fail_unless (!called);
  /* there shouldn't be anyhing pending in the default context */
  fail_unless (!g_main_context_pending (NULL));
  /* but there should be something pending in OUR context now */
  fail_unless (g_main_context_pending (ctx));
  fail_unless (!called);
  /* let's iterate it and dispatch the callback */
  fail_unless (g_main_context_iteration (ctx, FALSE));
  fail_unless (called);
  g_main_context_unref (ctx);
}

GNET_START_TEST (test_inetaddr_list_async)
{
  do_test_inetaddr_list_async ("localhost");
/* FIXME: these might not work right yet because of the timings in the test
#ifdef GNET_ENABLE_NETWORK_TESTS
  do_test_inetaddr_list_async ("www.google.com");
  do_test_inetaddr_list_async ("www.heise.de");
#endif
*/
#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    g_print ("Sleeping for a while to let cancelled threads finish ...\n");
    /* sleep a while to give any cancelled threads a chance to quit, otherwise
     * valgrind will think the threads were leaked */
    g_usleep (60 * G_USEC_PER_SEC);
  }
#endif
}
GNET_END_TEST;

static void
lookup_name_cb (GInetAddr * ia, gpointer data)
{
  gboolean *p_called = (gboolean *) data;
  gchar *name;

  *p_called = TRUE;

  fail_unless (ia != NULL);

  name = gnet_inetaddr_get_canonical_name (ia);
  fail_unless (name != NULL);
  fail_unless_equals_int (gnet_inetaddr_get_port (ia), 80);
  g_free (name);

  gnet_inetaddr_unref (ia);
}

static void
lookup_name_cb_do_nothing (GInetAddr * ia, gpointer data)
{
  gnet_inetaddr_unref (ia);
}

static void
do_test_inetaddr_name_async (const gchar * hostname)
{
  GInetAddrNewAsyncID async_id;
  GMainContext *ctx;
  gboolean called, destroyed;
  gint id;

  /* name async (default main context) */
  called = FALSE;
  async_id = gnet_inetaddr_new_async (hostname, 80, lookup_name_cb, &called);
  fail_unless (async_id != NULL);
  id = g_timeout_add (5 * 1000, (GSourceFunc) do_nothing_timeout, NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (called);
  g_source_remove (id);

  /* name async + cancel (default main context) */
  called = FALSE;
  async_id = gnet_inetaddr_new_async (hostname, 80, lookup_name_cb, &called);
  fail_unless (async_id != NULL);
  gnet_inetaddr_new_async_cancel (async_id);
  while (g_main_context_iteration (NULL, FALSE)) { ; }
  fail_unless (!called);

  /* name async (destroy notify) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_new_async_full (hostname, 80,
      lookup_name_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify; thread unlikely to start) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_new_async_full (hostname, 80,
      lookup_name_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  gnet_inetaddr_new_async_cancel (async_id);
  /* give thread a chance to execute and act on the cancelled flag */
  g_usleep (1 * G_USEC_PER_SEC);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify II) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_new_async_full (hostname, 80,
      lookup_name_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  /* give thread a chance to execute and block before we cancel it */
  g_usleep (1 * G_USEC_PER_SEC);
  gnet_inetaddr_new_async_cancel (async_id);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify III) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_new_async_full (hostname, 80,
      lookup_name_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  /* give thread a chance to execute and block and be done before cancelling */
  g_usleep (3 * G_USEC_PER_SEC);
  gnet_inetaddr_new_async_cancel (async_id);
  fail_unless (destroyed);

  /* name async (custom main context) */
  called = FALSE;
  ctx = g_main_context_new ();
  async_id = gnet_inetaddr_new_async_full (hostname, 80, lookup_name_cb,
      &called, (GDestroyNotify) NULL, ctx, G_PRIORITY_HIGH);
  /* give thread a chance to execute and block and be done */
  g_usleep (3 * G_USEC_PER_SEC);
  fail_unless (!called);
  /* there shouldn't be anyhing pending in the default context */
  fail_unless (!g_main_context_pending (NULL));
  /* but there should be something pending in OUR context now */
  fail_unless (g_main_context_pending (ctx));
  fail_unless (!called);
  /* let's iterate it and dispatch the callback */
  fail_unless (g_main_context_iteration (ctx, FALSE));
  fail_unless (called);
  g_main_context_unref (ctx);
}

GNET_START_TEST (test_inetaddr_name_async)
{
  do_test_inetaddr_name_async ("localhost");

/* FIXME: these might not work right yet because of the timings in the test
#ifdef GNET_ENABLE_NETWORK_TESTS
  do_test_inetaddr_name_async ("www.google.com");
#endif
*/
#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    g_print ("Sleeping for a while to let cancelled threads finish ...\n");
    /* sleep a while to give any cancelled threads a chance to quit, otherwise
     * valgrind will think the threads were leaked */
    g_usleep (60 * G_USEC_PER_SEC);
  }
#endif
}
GNET_END_TEST;

static void
reverse_lookup_func (gchar * hostname, gpointer data)
{
  /* should never be called, since we don't iterate the default main context */
  g_assert_not_reached ();
}

static void
do_test_inetaddr_reverse_async_ipv4_cancel (const gchar * ip_string)
{
  GInetAddrGetNameAsyncID async_id;
  GInetAddr *ia;

  g_print ("Starting reverse lookup for %s ...", ip_string);
  fflush (stdout);
  ia = gnet_inetaddr_new_nonblock (ip_string, 80);
  fail_unless (ia != NULL);

  ASSERT_CRITICAL (gnet_inetaddr_get_name_async (ia, NULL, NULL));

  async_id = gnet_inetaddr_get_name_async (ia, reverse_lookup_func, NULL);
  fail_unless (async_id != NULL);
  gnet_inetaddr_delete (ia);

  g_usleep (g_random_int_range (0, G_USEC_PER_SEC * 2));
  gnet_inetaddr_get_name_async_cancel (async_id);
  g_print (" cancelled.\n");
}

GNET_START_TEST (test_inetaddr_reverse_async_ipv4_cancel)
{
  ASSERT_CRITICAL (
      gnet_inetaddr_get_name_async (NULL, reverse_lookup_func, NULL)
  );

  do_test_inetaddr_reverse_async_ipv4_cancel ("127.0.0.1");

#ifdef GNET_ENABLE_NETWORK_TESTS
  {
    gint i;

    for (i = 0; i < 10; ++i) {

      if (i == 1) {
        do_test_inetaddr_reverse_async_ipv4_cancel ("217.155.155.155");
      } else if (i == 2) {
        do_test_inetaddr_reverse_async_ipv4_cancel ("91.189.93.3");
      } else {
        gchar *ip_string;

        ip_string = g_strdup_printf ("%u.%u.%u.%u",
            g_random_int_range (4, 250), g_random_int_range (0, 255),
            g_random_int_range (0, 255), g_random_int_range (0, 255));
        do_test_inetaddr_reverse_async_ipv4_cancel (ip_string);
        g_free (ip_string);
      }
    }
  }
#endif

#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    g_print ("Sleeping for a while to let cancelled threads finish ...\n");
    /* sleep a while to give any cancelled threads a chance to quit, otherwise
     * valgrind will think the threads were leaked */
    g_usleep (60 * G_USEC_PER_SEC);
  }
#endif
}
GNET_END_TEST;

static GInetAddr *current_ia; /* NULL */

static void
lookup_reverse_cb (gchar * hostname, gpointer data)
{
  gboolean *p_called = (gboolean *) data;

  *p_called = TRUE;

  fail_unless (hostname != NULL);

  g_free (hostname);
}

static void
lookup_reverse_cb_do_nothing (gchar * hostname, gpointer data)
{
  if (current_ia != NULL && hostname != NULL) {
    gchar *can_name;

    can_name = gnet_inetaddr_get_canonical_name (current_ia);
    g_print ("%s --> %s\n", can_name, hostname);
    g_free (can_name);
  }

  g_free (hostname);
}

static void
do_test_inetaddr_reverse_async (const gchar * hostname)
{
  GInetAddrGetNameAsyncID async_id;
  GMainContext *ctx;
  GInetAddr *ia;
  gboolean called, destroyed;
  gchar *ip_string;
  gint id;

  /* we assume the given host has a 1:1 IP<->hostname mapping */
  ia = gnet_inetaddr_new (hostname, 80);
  fail_unless (ia != NULL);
  ip_string = gnet_inetaddr_get_canonical_name (ia);
  fail_unless (ip_string != NULL);
  gnet_inetaddr_unref (ia);
  ia = NULL;

  g_print ("%s --> %s\n", hostname, ip_string);

  ia = gnet_inetaddr_new_nonblock (ip_string, 80);
  fail_unless (ia != NULL);
  current_ia = ia;

  /* name async (default main context) */
  called = FALSE;
  async_id = gnet_inetaddr_get_name_async (ia, lookup_reverse_cb, &called);
  fail_unless (async_id != NULL);
  id = g_timeout_add (10 * 1000, (GSourceFunc) do_nothing_timeout, NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (called);
  g_source_remove (id);

  /* name async + cancel (default main context) */
  called = FALSE;
  async_id = gnet_inetaddr_get_name_async (ia, lookup_reverse_cb, &called);
  fail_unless (async_id != NULL);
  gnet_inetaddr_get_name_async_cancel (async_id);
  while (g_main_context_iteration (NULL, FALSE)) { ; }
  fail_unless (!called);

  /* name async (destroy notify) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_get_name_async_full (ia,
      lookup_reverse_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  g_main_context_iteration (NULL, TRUE);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify; thread unlikely to start) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_get_name_async_full (ia,
      lookup_reverse_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  gnet_inetaddr_get_name_async_cancel (async_id);
  /* give thread a chance to execute and act on the cancelled flag */
  g_usleep (1 * G_USEC_PER_SEC);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify II) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_get_name_async_full (ia,
      lookup_reverse_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  /* give thread a chance to execute and block before we cancel it */
  g_usleep (1 * G_USEC_PER_SEC);
  gnet_inetaddr_get_name_async_cancel (async_id);
  fail_unless (destroyed);

  /* name async + cancel (destroy notify III) */
  destroyed = FALSE;
  async_id = gnet_inetaddr_get_name_async_full (ia,
      lookup_reverse_cb_do_nothing, &destroyed, destroy_notify, NULL,
      G_PRIORITY_HIGH);
  fail_unless (async_id != NULL);
  /* give thread a chance to execute and block and be done before cancelling */
  g_usleep (3 * G_USEC_PER_SEC);
  gnet_inetaddr_get_name_async_cancel (async_id);
  fail_unless (destroyed);

  /* name async (custom main context) */
  called = FALSE;
  ctx = g_main_context_new ();
  async_id = gnet_inetaddr_get_name_async_full (ia, lookup_reverse_cb,
      &called, (GDestroyNotify) NULL, ctx, G_PRIORITY_HIGH);
  /* give thread a chance to execute and block and be done */
  g_usleep (10 * G_USEC_PER_SEC);
  fail_unless (!called);
  /* there shouldn't be anyhing pending in the default context */
  fail_unless (!g_main_context_pending (NULL));
  /* but there should be something pending in OUR context now */
  fail_unless (g_main_context_pending (ctx));
  fail_unless (!called);
  /* let's iterate it and dispatch the callback */
  fail_unless (g_main_context_iteration (ctx, FALSE));
  fail_unless (called);
  g_main_context_unref (ctx);

  g_free (ip_string);
  gnet_inetaddr_unref (ia);
  current_ia = NULL;
}

GNET_START_TEST (test_inetaddr_reverse_async)
{
  do_test_inetaddr_reverse_async ("localhost");

#ifdef GNET_ENABLE_NETWORK_TESTS
  do_test_inetaddr_reverse_async ("gabe.freedesktop.org");
#endif

#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    g_print ("Sleeping for a while to let cancelled threads finish ...\n");
    /* sleep a while to give any cancelled threads a chance to quit, otherwise
     * valgrind will think the threads were leaked */
    g_usleep (60 * G_USEC_PER_SEC);
  }
#endif
}
GNET_END_TEST;

static Suite *
gnetinetaddr_suite (void)
{
  Suite *s = suite_create ("GInetAddr");
  TCase *tc_chain = tcase_create ("inetaddr");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_inetaddr_is_internet_domainname);
  tcase_add_test (tc_chain, test_inetaddr_ipv4);
  tcase_add_test (tc_chain, test_inetaddr_is_ipv4);
  tcase_add_test (tc_chain, test_inetaddr_list_async);
  tcase_add_test (tc_chain, test_inetaddr_name_async);
  tcase_add_test (tc_chain, test_inetaddr_reverse_async);
  tcase_add_test (tc_chain, test_inetaddr_reverse_async_ipv4_cancel);
#if HAVE_IPV6
  tcase_add_test (tc_chain, test_inetaddr_ipv6);
  tcase_add_test (tc_chain, test_inetaddr_is_ipv6);
#endif


  return s;
}

GNET_CHECK_MAIN (gnetinetaddr);

