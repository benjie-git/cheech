/* GNet GTcpSocket unit test
 * Copyright (C) 2006 Tim-Philipp Müller  <tim centricular net>
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

typedef enum {
  TEST_STATE_IDLE = 0,
  TEST_STATE_CONNECTING,
  TEST_STATE_CONNECTED
} GNetTestState;

static GTcpSocketConnectAsyncID  cur_id;      /* NULL */
static GTcpSocket               *cur_socket;  /* NULL */
static GNetTestState             cur_state = TEST_STATE_IDLE;
static gint                      counter = 0;

static void
default_log_handler (const gchar *domain, GLogLevelFlags level,
    const gchar * msg, gpointer data)
{
  if (g_getenv ("GNET_DEBUG") != NULL)
  {
    g_print ("%s%s%s", (domain) ? domain: "", (domain) ? ": " : "", msg);
  } else {
    static gint counter = 0;

    if (++counter % 25 == 0)
      g_print (".");
  }
}

static void
sock_cb (GTcpSocket * socket, GTcpSocketConnectAsyncStatus status, gpointer data)
{
  gint c = GPOINTER_TO_INT (data);

  g_debug ("%p (%03d) socket=%p, status=%d\n", g_thread_self (), c, socket, status);

  g_assert (cur_id != NULL);
  g_assert (cur_socket == NULL);

  switch (status) {
    case GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK: {
      cur_state = TEST_STATE_CONNECTED;
      g_debug ("%p (%03d) connected!\n", g_thread_self (), c);
      cur_socket = socket;
      cur_id = NULL;
      break;
    }
    case GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR: {
      g_debug ("%p (%03d) lookup failed!?\n", g_thread_self (), c);
      gnet_tcp_socket_delete (socket); /* does nothing, socket is NULL */
      cur_socket = NULL;
      cur_id = NULL;
      cur_state = TEST_STATE_IDLE;
      break;
    }
    case GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR: {
      g_debug ("%p (%03d) connection refused! (tcp error)\n", g_thread_self (), c);
      gnet_tcp_socket_delete (socket); /* does nothing, socket is NULL */
      cur_socket = NULL;
      cur_id = NULL;
      cur_state = TEST_STATE_IDLE;
      break;
    }
    default:
      g_assert_not_reached ();
  }
}

static gboolean
idle_cb (gpointer data)
{
#ifdef GNET_ENABLE_NETWORK_TESTS
  const gchar *hostnames[] = { "localhost", "127.0.0.1", "www.google.com",
      "www.gnome.org", "www.yahoo.com", "www.ebay.com", "www.amazon.co.jp" };
#else
  const gchar *hostnames[] = { "localhost", "127.0.0.1" };
#endif

  switch (cur_state) {
    case TEST_STATE_IDLE: {
      const gchar *hostname;

      g_assert (cur_id == NULL);
      g_assert (cur_socket == NULL);

      hostname = hostnames[counter % G_N_ELEMENTS (hostnames)];

      ++counter;
      g_debug ("%p (%03d) idle: connecting to %s ...\n", g_thread_self (), counter, hostname);
      cur_id = gnet_tcp_socket_connect_async ("hostname", 1500, sock_cb, GINT_TO_POINTER (counter));
      if (cur_id != NULL) {
        cur_state = TEST_STATE_CONNECTING;
      } else {
        g_debug ("%p (%03d) idle: connecting attempt FAILED!\n", g_thread_self (), counter);
      }
      break;
    }
    case TEST_STATE_CONNECTING: {
      g_assert (cur_id != NULL);
      g_assert (cur_socket == NULL);

      g_debug ("%p (%03d) connecting: cancelling connect ... \n", g_thread_self (), counter);
      gnet_tcp_socket_connect_async_cancel (cur_id);
      cur_state = TEST_STATE_IDLE;
      cur_id = NULL;
      break;
    }
    case TEST_STATE_CONNECTED: {
      g_assert (cur_id == NULL);
      g_assert (cur_socket != NULL);

      g_debug ("%p  (%03d) connected: disconnecting ...\n", g_thread_self (), counter);
      gnet_tcp_socket_delete (cur_socket);
      cur_socket = NULL;
      cur_state = TEST_STATE_IDLE;
      break;
    }
    default:
      g_assert_not_reached ();
  }

  /* try to make it a bit less deterministic, also to avoid us creating too
   * many threads and pthread aborting because of that */
  if (((g_random_int () & (3 << 15)) >> 15) == 3) {
    g_usleep (G_USEC_PER_SEC/4);
  }

  return TRUE; /* call again */
}

static gboolean
stop_connecting (gpointer id)
{
  g_source_remove (GPOINTER_TO_INT (id));
  return FALSE;
}

/* Do lots of connection attemps/cancelling thereof in quick order and
 * make sure nothing funny is happening. Run in valgrind for extra fun.
 * (see http://bugzilla.gnome.org/show_bug.cgi?id=305854)
 *
 * NOTE: this _might_ abort at some point if we create too many threads,
 * that's kind of to be expected and depends a lot on the environment.
 * We should use thread pools for DNS lookups.
 */
GNET_START_TEST (test_tcp_socket_async_connect_cancel)
{
  GMainLoop *loop;
  gint idle_id;

  g_log_set_default_handler (default_log_handler, NULL);

  loop = g_main_loop_new (NULL, FALSE);

  idle_id = g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) idle_cb, NULL, NULL);

  /* stop connecting after 15 seconds */
  g_timeout_add (15 * 1000, (GSourceFunc) stop_connecting, GINT_TO_POINTER (idle_id));

  /* stop test after 16 seconds */
  g_timeout_add (16 * 1000, (GSourceFunc) g_main_loop_quit, loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);

  g_print ("%u attempts\n", counter);
}

GNET_END_TEST;

static Suite *
gnettcpsocket_suite (void)
{
  Suite *s = suite_create ("GTcpSocket");
  TCase *tc_chain = tcase_create ("tcpsocket");

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_tcp_socket_async_connect_cancel);
  return s;
}

GNET_CHECK_MAIN (gnettcpsocket);

