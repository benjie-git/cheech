/* GNet GConnHttp unit test
 * Copyright (C) 2004, 2007 Tim-Philipp Müller <tim centricular net>
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

#define GNET_EXPERIMENTAL 1

#include "config.h"
#include "gnetcheck.h"

#include <string.h>       

static gboolean verbose; /* FALSE */

/* Print some information along the way (only for debugging purposes) */
static void
http_dbg_callback (GConnHttp *http, GConnHttpEvent *event, gpointer baz)
{
  if (!verbose)
    return;

  switch (event->type)
  {
    case GNET_CONN_HTTP_RESOLVED:
    {
      GConnHttpEventResolved *ev_resolved = (GConnHttpEventResolved*)event;
      if (ev_resolved->ia) {
        gchar *cname;

        cname = gnet_inetaddr_get_canonical_name (ev_resolved->ia);
        g_printerr ("RESOLVED: %s\n\n", cname);
        g_free (cname);
      } else {
        g_printerr ("RESOLVED: FAILED TO RESOLVE HOSTNAME.\n");
      }
    }
    break;

    case GNET_CONN_HTTP_RESPONSE:
    {
      GConnHttpEventResponse *ev_response = (GConnHttpEventResponse*)event;
      guint                   n;
      
      g_printerr ("RESPONSE: %u\n", ev_response->response_code); 
      for (n = 0;  ev_response->header_fields[n] != NULL;  ++n)
      {
        g_printerr ("HEADER: %s: %s\n", 
                ev_response->header_fields[n], 
                ev_response->header_values[n]);
      }
      g_printerr ("\n");
    }
    break;
    
    case GNET_CONN_HTTP_DATA_PARTIAL:
    {
      GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
      if (ev_data->content_length > 0)
        g_printerr ("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (%.1f%%)\n", ev_data->data_received,
                ((gfloat)ev_data->data_received/(gfloat)ev_data->content_length) * 100.0);
      else
        g_printerr ("PARTIAL DATA: %" G_GUINT64_FORMAT " bytes (content length unknown)\n", 
                ev_data->data_received);
    }
    break;
    
    case GNET_CONN_HTTP_DATA_COMPLETE:
    {
      GConnHttpEventData *ev_data = (GConnHttpEventData*)event;
      g_printerr ("\n");
      g_printerr ("COMPLETE:  %" G_GUINT64_FORMAT " bytes received.\n", ev_data->data_received);
    }
    break;

    case GNET_CONN_HTTP_REDIRECT:
    {
      GConnHttpEventRedirect *ev_redir = (GConnHttpEventRedirect*)event;
      if (!ev_redir->auto_redirect)
        g_printerr ("REDIRECT: New location => '%s' (not automatic)\n", ev_redir->new_location);
      else
        g_printerr ("REDIRECT: New location => '%s' (automatic)\n", ev_redir->new_location);
    }
    break;
    
    case GNET_CONN_HTTP_CONNECTED:
      g_printerr ("CONNECTED\n");
      break;

    /* Internal GConnHttp error */
    case GNET_CONN_HTTP_ERROR:
    {
      GConnHttpEventError *ev_error = (GConnHttpEventError*)event;
      g_printerr ("ERROR #%u: %s.\n", ev_error->code, ev_error->message);
    }
    break;
    
    case GNET_CONN_HTTP_TIMEOUT:
    {
      g_printerr ("GNET_CONN_HTTP_TIMEOUT.\n");
    }
    break;
                
    default:
      g_assert_not_reached();
  }
}

GNET_START_TEST (test_conn_http_run)
{
  /* at least some of these should lead to a redirect; sf.net is a redirect
   * to the same host */
  const gchar *uris[] = { "http://www.google.com", "http://www.google.co.uk",
      "http://www.google.de", "http://www.google.fr", "http://www.google.es",
      "http://www.gnome.org", "http://www.amazon.com", "http://sf.net",
      "http://news.bbc.co.uk" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (uris); ++i) {
    const gchar *uri = uris[i];
    GConnHttp *httpconn;
    gchar *buf;
    gsize len;
  
    httpconn = gnet_conn_http_new ();
    fail_unless (httpconn != NULL);
    
    fail_unless (gnet_conn_http_set_uri (httpconn, uri),
        "Couldn't set URI '%s' on GConnHttp", uri);

    g_print ("Getting %s ... ", uri);

    fail_unless (gnet_conn_http_run (httpconn, http_dbg_callback, NULL));
    
    gnet_conn_http_steal_buffer (httpconn, &buf, &len);
    g_print ("%s, received %d bytes.\n", (len > 0) ? "ok" : "FAILED", (int) len);
    fail_unless (len > 0);
    fail_unless (buf != NULL);

    gnet_conn_http_delete (httpconn);
    g_free (buf);
  }
}
GNET_END_TEST;

/*** GET ASYNC ***/

typedef struct {
  GMainLoop *loop;
  gboolean got_error;
  guint64 len;
} AsyncHelper;

static void
http_async_callback (GConnHttp *http, GConnHttpEvent *event, gpointer userdata)
{
  AsyncHelper *helper = (AsyncHelper *) userdata;

  if (verbose) {
    g_printerr ("%s: event->type = %u\n", __FUNCTION__, event->type);
    http_dbg_callback (http, event, NULL);
  }

  switch (event->type)
  {
    case GNET_CONN_HTTP_RESOLVED:
    case GNET_CONN_HTTP_RESPONSE:
    case GNET_CONN_HTTP_REDIRECT:
    case GNET_CONN_HTTP_CONNECTED:
      break;
    
    case GNET_CONN_HTTP_DATA_PARTIAL:
    {
      GConnHttpEventData *data_event = (GConnHttpEventData*) event;

      /* we received data, make sure these values are set */
      g_assert (data_event->buffer != NULL);
      g_assert (data_event->buffer_length > 0);
      g_assert (data_event->data_received > 0);
      helper->len = data_event->data_received;
      break;
    }

    case GNET_CONN_HTTP_ERROR:
    {
      GConnHttpEventError *err_event = (GConnHttpEventError*) event;
      if (verbose) {
        g_printerr ("Error: %s (code=%u)\n", err_event->message,
            err_event->code);
      }
      helper->got_error = TRUE;
    }
    /* fallthrough */

    case GNET_CONN_HTTP_DATA_COMPLETE:
    case GNET_CONN_HTTP_TIMEOUT:
      if (verbose)
        g_printerr ("%s: done. Deleting http object.\n", __FUNCTION__);
      /* make sure we can call _delete() from the callback */
      gnet_conn_http_delete (http);
      g_main_loop_quit (helper->loop);
      break;
                
    default:
      g_assert_not_reached();
  }
}

/* This mainly tests whether we can do gnet_conn_http_delete()
 * from within the async callback when we have the data.
*/
static gboolean
test_get_async (const gchar *uri)
{
  AsyncHelper helper;
  GConnHttp *httpconn;

  httpconn = gnet_conn_http_new ();

  if (!gnet_conn_http_set_uri (httpconn, uri)) {
    g_printerr ("Couldn't set URI '%s' on GConnHttp", uri);
    return FALSE;
  }

  g_print ("Getting (async) '%s' ... ", uri);

  helper.loop = g_main_loop_new (NULL, FALSE);

  helper.got_error = FALSE;
  helper.len = 0;

  gnet_conn_http_run_async (httpconn, http_async_callback, &helper);

  g_main_loop_run (helper.loop);

  g_print ("done, received %u bytes.\n", (guint) helper.len);

  g_main_loop_unref (helper.loop);
  helper.loop = NULL;

  /* http conn object has been deleted in callback */

  return (!helper.got_error);
}

GNET_START_TEST (test_conn_http_get_async)
{
  /* If this test returns, it went ok (check that an error event gets sent).
   * It should return FALSE though, since we should have gotten an error */
  fail_if (test_get_async ("http://non-exist.ant"));

  /* If this test returns and doesn't crash, it went ok
   * (the httpconn is deleted from within callback, which should work) */
  fail_unless (test_get_async ("http://www.google.com"));
  fail_unless (test_get_async ("http://www.google.co.uk"));
  fail_unless (test_get_async ("http://www.google.de"));
  fail_unless (test_get_async ("http://www.google.fr"));
  fail_unless (test_get_async ("http://www.google.es"));
}
GNET_END_TEST;

/*** POST ***/

/* PLEASE do not use this developer ID in 
 *  your own code, get your own one */
#define AWS_DEV_ID "1BXDDWFYXTVWC8WQXF02"
#define POST_ARTIST "Massive Attack"
#define POST_ALBUM "Blue lines"

static gchar *
do_post_for_uri (const gchar * uri)
{
  GConnHttp   *http;
  gchar       *postdata, *buf, *artist_esc, *album_esc;
  gsize        buflen;
  
  /* g_markup_printf_escaped() only exists in Glib-2.4 and later */
  artist_esc = g_markup_escape_text (POST_ARTIST, -1);
  album_esc = g_markup_escape_text (POST_ALBUM, -1);
  postdata = g_strdup_printf (
                  "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                  "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
                  " xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
                  " xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                  "<soap:Body>"
                  "<q1:ArtistSearchRequest xmlns:q1=\"http://soap.amazon.com\">"
                  "<ArtistSearchRequest href=\"#id1\" />"
                  "</q1:ArtistSearchRequest>"
                  "<q2:ArtistRequest id=\"id1\" xsi:type=\"q2:ArtistRequest\" xmlns:q2=\"http://soap.amazon.com\">"
                  "<artist xsi:type=\"xsd:string\">%s</artist>" /* <----------- */
                  "<page xsi:type=\"xsd:string\">1</page>"
                  "<mode xsi:type=\"xsd:string\">music</mode>"
                  "<tag xsi:type=\"xsd:string\">webservices-20</tag>"
                  "<type xsi:type=\"xsd:string\">lite</type>"
                  "<devtag xsi:type=\"xsd:string\">%s</devtag>"
                  "<keywords xsi:type=\"xsd:string\">%s</keywords>" /* <----------- */
                  "</q2:ArtistRequest>"
                  "</soap:Body>"
                  "</soap:Envelope>",
                  artist_esc, AWS_DEV_ID, album_esc);

  g_free (artist_esc);
  g_free (album_esc);

  http = gnet_conn_http_new();

  fail_unless (gnet_conn_http_set_uri (http, uri),
      "Could not set URI '%s'", uri);

  fail_unless (gnet_conn_http_set_method (http, GNET_CONN_HTTP_METHOD_POST,
      postdata, strlen(postdata)));

  fail_unless (gnet_conn_http_set_header (http, "SOAPAction", 
      "\"http://soap.amazon.com\"", GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK));
               
  fail_unless (gnet_conn_http_set_header (http, "Content-Type", 
      "text/xml; charset=utf-8", GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK));


  fail_unless (gnet_conn_http_run (http, http_dbg_callback, postdata));
  
  gnet_conn_http_steal_buffer(http, &buf, &buflen);
  g_print ("POST operation ok, received %u bytes.\n", (guint) buflen);
  gnet_conn_http_delete(http);
  g_free(postdata);
  return buf;
}

GNET_START_TEST (test_conn_http_post)
{
  gchar *buf, *tag;

  buf = do_post_for_uri ("http://soap.amazon.com/onca/soap3");

  /* now parse result */
  tag = strstr(buf, "</ListPrice>");
  if (tag)
  {
    *tag = 0x00;
    while (tag > buf && *tag != '>')
      --tag;
    ++tag;
  }
    
  if (tag && *tag == '$')
    g_print ("%s, %s - ListPrice = %s\n", POST_ARTIST, POST_ALBUM, tag);
  else
    g_print ("Could not find ListPrice for CD %s, %s in data returned :|\n",
        POST_ARTIST, POST_ALBUM);

  g_free(buf);
}
GNET_END_TEST;

static void
http_post_conn_func (GConn * conn, GConnEvent * event, gpointer data)
{
  guint *p_written = (guint *) data;

  if (event->type == GNET_CONN_WRITE)
    *p_written += 1;
}

static GConn *last_conn = NULL; /* save here so we can delete it d'oh */

static void
http_post_server_func (GServer * srv, GConn * conn, gpointer user_data)
{
  const gchar **lines = (const gchar **) user_data;
  gint num_lines = 0, lines_written = 0;

  last_conn = conn;

  fail_unless (conn != NULL, "Can't set up server, some error occured");

  fail_unless (gnet_conn_is_connected (conn));

  gnet_conn_set_callback (conn, http_post_conn_func, &lines_written);

  /* don't parse or check what the server sends us, just assume it does what
   * we expect it to and send out data as would be appropriate as response */
  while (lines != NULL && lines[0] != NULL) {
    gnet_conn_write (conn, (gchar *) lines[0], strlen (lines[0]));
    ++lines;
    ++num_lines;
  }
  /* can't disconnect/close here yet, or the pending data might not actually
   * be sent (see open bug about this); work around this by iterating the
   * main context until there are no more pending events, ie. the
   * writable-watch set up by gnet_conn_write() has been removed */
  while (lines_written < num_lines) {
    g_main_context_iteration (NULL, TRUE);
  }

  /* FIXME: gnet_conn_disconnect (conn); */
  /* FIXME: gnet_conn_unref (conn); */
}

GNET_START_TEST (test_conn_http_post_local)
{
  const gchar *lines[] = {
      "HTTP/1.1 100 Continue\r\n",
      "Connection: Keep-Alive\r\n",
      "\r\n",
      "HTTP/1.1 200 OK\r\n",
      "Date: Mon, 15 Oct 2007 14:40:20 GMT\r\n",
      "Server: Server\r\n",
      "nnCoection: close\r\n",
      "Content-Type: text/xml; charset=UTF-8\r\n",
      "Vary: Accept-Encoding,User-Agent\r\n",
      "nnCoection: close\r\n",
      "Transfer-Encoding: chunked\r\n",
      "Connection: Keep-Alive\r\n",
      "\r\n",
      "d\r\n",
      "<html></html>\r\n",
      "0\r\n",
      NULL
   };
  const gchar *lines2[] = {
      "HTTP/1.1 100 Continue\r\n",
      "\r\n",
      "HTTP/1.1 200 OK\r\n",
      "Date: Mon, 15 Oct 2007 14:40:20 GMT\r\n",
      "Server: Server\r\n",
      "nnCoection: close\r\n",
      "Content-Type: text/xml; charset=UTF-8\r\n",
      "Vary: Accept-Encoding,User-Agent\r\n",
      "nnCoection: close\r\n",
      "Transfer-Encoding: chunked\r\n",
      "Connection: Keep-Alive\r\n",
      "\r\n",
      "d\r\n",
      "<html></html>\r\n",
      "0\r\n",
      NULL
   };
  GInetAddr *ia;
  GServer *srv;
  gchar *uri;
  gchar *buf;

  /* Disable any SOCKS proxies if enabled, since this test works locally */
  gnet_socks_set_enabled (FALSE);

  /* Test case where we get headers after the HTTP/1.1 100 Continue */
  ia = gnet_inetaddr_new ("127.0.0.1", 0);
  fail_unless (ia != NULL);
  srv = gnet_server_new (ia, 0, http_post_server_func, lines);
  gnet_inetaddr_unref (ia);
  fail_unless (srv != NULL, "Could not bind to 127.0.0.1, check your setup");
  uri = g_strdup_printf ("http://127.0.0.1:%d", srv->port);
  buf = do_post_for_uri (uri);
  g_free (uri);
  fail_unless_equals_string (buf, "<html></html>");
  g_free (buf);
  gnet_conn_unref (last_conn);
  last_conn = NULL;

  /* Test case where we get no headers after the HTTP/1.1 100 Continue */
  ia = gnet_inetaddr_new ("127.0.0.1", 0);
  fail_unless (ia != NULL);
  srv = gnet_server_new (ia, 0, http_post_server_func, lines2);
  gnet_inetaddr_unref (ia);
  fail_unless (srv != NULL, "Could not bind to 127.0.0.1, check your setup");
  uri = g_strdup_printf ("http://127.0.0.1:%d", srv->port);
  buf = do_post_for_uri (uri);
  g_free (uri);
  fail_unless_equals_string (buf, "<html></html>");
  g_free (buf);
  gnet_conn_unref (last_conn);
  last_conn = NULL;
}
GNET_END_TEST;

GNET_START_TEST (test_gnet_http_get)
{
  const gchar *urls[] = {"http://www.gnetlibrary.org/src/",
      "http://www.heise.de" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (urls); ++i) {
    gchar *buf = NULL;
    gsize  buflen = 0;
    guint  code = 0;                

    g_print ("gnet_http_get %s ... ", urls[i]);

    fail_unless (gnet_http_get (urls[i], &buf, &buflen, &code),
        "buflen = %u, code = %u", (guint) buflen, code);
  
    g_print ("done - buflen = %u, code = %u\n", (guint) buflen, code);
    fail_unless (code == 200);
    fail_unless (buflen > 0);
    fail_unless (buf != NULL);
    /* make sure we can access it */
    fail_unless (buf[buflen-1] == 0 || buf[buflen-1] != 0);
    g_free (buf);
  }
}
GNET_END_TEST;

GNET_START_TEST (test_get_binary)
{
  gchar *uris[] = { "http://www.gnetlibrary.org/gnet.png" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (uris); ++i) {
    GConnHttp *httpconn;
    gchar     *buf;
    gsize      len;
  
    httpconn = gnet_conn_http_new ();

    g_print ("Getting binary file %s ... ", uris[i]);
    
    fail_unless (gnet_conn_http_set_uri (httpconn, uris[i]),
        "Can't set URI '%s' on GConnHttp!", uris[i]);

    fail_unless (gnet_conn_http_run (httpconn, NULL, NULL));
  
    gnet_conn_http_steal_buffer (httpconn, &buf, &len);
    fail_unless (len > 0);
    fail_unless (buf != NULL);
    /* check that we can access the data */
    fail_unless (buf[len-1] == 0 || buf[len-1] != 0);
  
    g_print ("ok, received %u bytes.\n", (guint) len);

    /* now check the data we've received, should be PNG */
    if (!buf || len < 8 || strncmp(buf, "\211PNG\r\n\032\n", 8) != 0)
      g_error ("missing PNG signature?!");
   
    if (len < (8+(4+4)) || strncmp(buf+8+4, "IHDR", 4) != 0)
      g_error ("missing IHDR chunk?!");
  
    g_print ("PNG image width x height = %u x %u\n",
             GUINT32_FROM_BE(*((guint32*)(buf+8+4+4))),
             GUINT32_FROM_BE(*((guint32*)(buf+8+4+4+4))));

    gnet_conn_http_delete(httpconn);
    g_free(buf);
  }
}
GNET_END_TEST;

static void
icy_async_cb (GConnHttp * http, GConnHttpEvent * event, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  if (verbose) {
    g_printerr ("%s: event->type = %u\n", __FUNCTION__, event->type);
    http_dbg_callback (http, event, NULL);
  }

  switch (event->type) {
    case GNET_CONN_HTTP_RESPONSE: {
      GConnHttpEventResponse *r = (GConnHttpEventResponse *) event;

      if (r->response_code == 200) {
        gboolean found_icy_name = FALSE;
        gchar **keys, **vals;

        keys = r->header_fields;
        vals = r->header_values;
        while (keys != NULL && *keys != NULL) {
          if (verbose)
            g_printerr ("%s: %20s: %s\n", __FUNCTION__, *keys, *vals);
          if (strcmp (*keys, "icy-name") == 0) {
            g_print ("icy name: %s\n", *vals);
            found_icy_name = TRUE;
            break;
          }
          ++keys;
          ++vals;
        }
        fail_unless (found_icy_name);
      }
      /* wait for data */
      break;
    }
    case GNET_CONN_HTTP_RESOLVED:
    case GNET_CONN_HTTP_REDIRECT:
    case GNET_CONN_HTTP_CONNECTED:
      break;
    case GNET_CONN_HTTP_DATA_COMPLETE:
    case GNET_CONN_HTTP_DATA_PARTIAL:
      g_main_loop_quit (loop);
      break;
    case GNET_CONN_HTTP_ERROR:
    case GNET_CONN_HTTP_TIMEOUT:
      g_print ("shoutcast test: timeout or error");
      g_main_loop_quit (loop);
      break;
    default:
      g_error ("Unexpected GConnHttpEventType value %d\n", event->type);
  }
}

/* we test that we accept an ICY response too here, and that things work
 * okay if we specify our own main context */
GNET_START_TEST (test_shoutcast)
{
  GMainContext *ctx;
  const gchar *uri;
  GConnHttp *httpconn;
  GMainLoop *loop;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  fail_if (g_main_context_pending (ctx));
  fail_if (g_main_context_pending (NULL));

  httpconn = gnet_conn_http_new ();
  uri = "http://mp3-gr-128.smgradio.com:80/";
  /* uri = "http://mp3-vr-128.smgradio.com:80/"; */
  fail_unless (gnet_conn_http_set_uri (httpconn, uri));
  fail_unless (gnet_conn_http_set_main_context (httpconn, ctx));

  fail_if (g_main_context_pending (ctx));
  fail_if (g_main_context_pending (NULL));

  gnet_conn_http_run_async (httpconn, icy_async_cb, loop);

  g_main_loop_run (loop);
  gnet_conn_http_delete (httpconn);

  fail_if (g_main_context_pending (NULL));

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}
GNET_END_TEST;

static Suite *
gnetconnhttp_suite (void)
{
  Suite *s = suite_create ("GConnHttp");
  TCase *tc_chain = tcase_create ("connhttp");
  gboolean run_network_checks;

  verbose = (g_getenv ("GNET_DEBUG") != NULL);

  tcase_set_timeout (tc_chain, 0);

  suite_add_tcase (s, tc_chain);

#ifdef GNET_ENABLE_NETWORK_TESTS
  run_network_checks = TRUE;
#else
  run_network_checks = FALSE;
#endif

  if (run_network_checks) {
    tcase_add_test (tc_chain, test_conn_http_run);
    tcase_add_test (tc_chain, test_conn_http_get_async);
    tcase_add_test (tc_chain, test_conn_http_post);
    tcase_add_test (tc_chain, test_gnet_http_get);
    tcase_add_test (tc_chain, test_get_binary);
    tcase_add_test (tc_chain, test_shoutcast);
  }

  tcase_add_test (tc_chain, test_conn_http_post_local);

  return s;
}

GNET_CHECK_MAIN (gnetconnhttp);

