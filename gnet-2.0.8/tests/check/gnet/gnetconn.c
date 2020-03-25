/* GNet GConn unit tests
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

static void
conn_fail_cb (GConn * conn, GConnEvent * event, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  /* the domain doesn't exist, it should give us an error */
  fail_unless_equals_int (event->type, GNET_CONN_ERROR);

  /* test that we can delete the connection in the callback */
  gnet_conn_delete (conn);

  g_main_loop_quit (loop);
}

static void
conn_cb (GConn * conn, GConnEvent * event, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (event->type) {
    case GNET_CONN_CONNECT: {
      const gchar req[] = "GET / HTTP/1.0\r\n"
          "Accept: */*\r\nConnection: Close\r\n"
          "\r\n";

      g_print ("Connected.\n");
      /* using _write_direct() here on purpose to test that code path too */
      gnet_conn_write_direct (conn, g_strdup (req), strlen (req),
          (GDestroyNotify) g_free);
      break;
    }
    case GNET_CONN_WRITE:
      g_print ("Sent request.\n");
      gnet_conn_read (conn);
      break;
    case GNET_CONN_READ:
      g_print (".");
      /* write (1, event->buffer, event->length); */
      gnet_conn_read (conn);
      break;
    case GNET_CONN_READABLE:
      g_error ("READABLE event shouldn't have happened without explicit watch");
      break;
    case GNET_CONN_WRITABLE:
      g_error ("WRITABLE event shouldn't have happened without explicit watch");
      break;
    case GNET_CONN_CLOSE:
      g_print ("\nConnection closed by server.\n");
      g_main_loop_quit (loop);
      break;
    case GNET_CONN_TIMEOUT:
      g_print ("Timeout.\n");
      g_main_loop_quit (loop);
      break;
    case GNET_CONN_ERROR:
      g_print ("Connection error.\n");
      g_main_loop_quit (loop);
      break;
    default:
      g_error ("Unexpected event type %d", event->type);
      break;
  }
}

GNET_START_TEST (test_conn_new)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GConn *conn;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  conn = gnet_conn_new ("www.microsoft.com", 80, conn_cb, loop);
  /* should always succeed, we're not doing any lookups or connecting yet */
  fail_unless (conn != NULL);

  gnet_conn_timeout (conn, 5 * 1000);
  gnet_conn_set_main_context (conn, ctx);

  fail_if (g_main_context_pending (ctx));

  gnet_conn_connect (conn);

  g_main_loop_run (loop);

  gnet_conn_disconnect (conn);
  gnet_conn_delete (conn);

  fail_if (g_main_context_pending (NULL));
  fail_if (g_main_context_pending (ctx));

  /* test failure case too */
  conn = gnet_conn_new ("does.not.exist", 80, conn_fail_cb, loop);
  /* should always succeed, we're not doing any lookups or connecting yet */
  fail_unless (conn != NULL);

  gnet_conn_timeout (conn, 5 * 1000);
  gnet_conn_set_main_context (conn, ctx);

  fail_if (g_main_context_pending (ctx));

  gnet_conn_connect (conn);

  g_main_loop_run (loop);

  /* we've deleted the connection in the callback */

  g_main_context_unref (ctx);
  g_main_loop_unref (loop);

#ifdef HAVE_VALGRIND
  if (RUNNING_ON_VALGRIND) {
    /* give DNS lookup thread a chance to finish, it's not stuck in a blocking
     * operation, so a short sleep should be fine; valgrind will think the
     * thread was leaked if we don't do this */
    g_usleep (1 * G_USEC_PER_SEC);
  }
#endif
}
GNET_END_TEST;

GNET_START_TEST (test_conn_new_inetaddr)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GInetAddr *ia;
  GConn *conn;

  ia = gnet_inetaddr_new ("www.sun.com", 80);

  /* it's not fatal if this doesn't work, it's not what we're testing here */
  if (ia == NULL) {
    g_print ("Failed to resolve www.sun.com, skipping test.\n");
    return;
  }

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  ASSERT_CRITICAL (fail_if (gnet_conn_new_inetaddr (NULL, conn_cb, loop)));

  conn = gnet_conn_new_inetaddr (ia, conn_cb, loop);
  gnet_inetaddr_unref (ia);

  /* should always succeed, we're not connecting yet */
  fail_unless (conn != NULL);

  gnet_conn_timeout (conn, 5 * 1000);
  gnet_conn_set_main_context (conn, ctx);

  fail_if (g_main_context_pending (ctx));

  gnet_conn_connect (conn);

  g_main_loop_run (loop);

  gnet_conn_disconnect (conn);
  gnet_conn_delete (conn);

  fail_if (g_main_context_pending (NULL));

  g_main_context_unref (ctx);
  g_main_loop_unref (loop);
}
GNET_END_TEST;

GNET_START_TEST (test_conn_new_socket)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GTcpSocket *socket;
  GConn *conn;

  socket = gnet_tcp_socket_connect ("www.oracle.com", 80);

  /* it's not fatal if this doesn't work, it's not what we're testing here */
  if (socket == NULL) {
    g_print ("Failed to resolve+connect to www.oracole.com, skipping test.\n");
    return;
  }

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  ASSERT_CRITICAL (fail_if (gnet_conn_new_socket (NULL, conn_cb, loop)));
  
  conn = gnet_conn_new_socket (socket, conn_cb, loop);

  /* conn took ownership of the socket */
  socket = NULL;

  /* should always succeed */
  fail_unless (conn != NULL);

  gnet_conn_timeout (conn, 5 * 1000);
  gnet_conn_set_main_context (conn, ctx);

  fail_if (g_main_context_pending (ctx));

  /* we're already connected, so do fake call to callback so request is sent */
  {
    GConnEvent event = { GNET_CONN_CONNECT, NULL, 0 };

    conn_cb (conn, &event, loop);
  }

  g_main_loop_run (loop);

  gnet_conn_disconnect (conn);
  gnet_conn_delete (conn);

  fail_if (g_main_context_pending (NULL));

  g_main_context_unref (ctx);
  g_main_loop_unref (loop);
}
GNET_END_TEST;

static Suite *
gnetconn_suite (void)
{
  Suite *s = suite_create ("GConn");
  TCase *tc_chain = tcase_create ("conn");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);

#ifdef GNET_ENABLE_NETWORK_TESTS
  tcase_add_test (tc_chain, test_conn_new);
  tcase_add_test (tc_chain, test_conn_new_inetaddr);
  tcase_add_test (tc_chain, test_conn_new_socket);
#endif

  return s;
}

GNET_CHECK_MAIN (gnetconn);

