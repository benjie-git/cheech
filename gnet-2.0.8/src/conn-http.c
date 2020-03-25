/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2004  Tim-Philipp Müller <tim centricular net>
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

/***************************************************************************
 *                                                                         *
 *   TODO:                                                                 *
 *    - support http_proxy environment variable (possible also via API)    *
 *    - support authentication                                             *
 *    - read RFC for Http/1.1 from cover to cover and check compliance     *
 *    - think about how to make an HTTPS easy - maybe add public vfuncs?   *
 *                                                                         *
 ***************************************************************************/

#include "conn-http.h"
#include "gnetconfig.h"

#include <string.h>
#include <stdlib.h>

#define GNET_CONN_HTTP_DEFAULT_MAX_REDIRECTS  5
#define GNET_CONN_HTTP_BUF_INCREMENT          8192    /* 8kB */

#define CONN_HTTP_MAGIC_SEQUENCE  499138271
#define GNET_IS_CONN_HTTP(conn)  ((conn)&&(((GConnHttp*)(conn))->stamp == CONN_HTTP_MAGIC_SEQUENCE))

typedef enum
{
	STATUS_NONE = 0,
	STATUS_SENT_REQUEST,
	STATUS_RECV_HEADERS,
	STATUS_RECV_BODY_NONCHUNKED,
	STATUS_RECV_CHUNK_SIZE,
	STATUS_RECV_CHUNK_BODY,
	STATUS_ERROR,
	STATUS_DONE
} GConnHttpStatus;

struct _GConnHttp
{
	guint                stamp;           /* magic cookie instead of a type system */

	GInetAddrNewAsyncID  ia_id;
	GInetAddr           *ia;

	GConn               *conn;
	
	gboolean             connection_close; /* Connection: close header was given */

	GConnHttpFunc        func;
	gpointer             func_data;

	guint                num_redirects;
	guint                max_redirects;
	gchar               *redirect_location; /* set if auto-redirection is 
	                                           required after body is received */
	
	GURI                *uri;
	GList               *req_headers;  /* request headers we send */
	GList               *resp_headers; /* response headers we got */

	guint                response_code; 

	GConnHttpMethod      method;
	GConnHttpStatus      status;

	guint                timeout;

	gchar               *post_data;
	gsize                post_data_len;
	gsize                post_data_term_len; /* for extra \n\r etc. */
	
	gsize                content_length;
	gsize                content_recv;
	gboolean             got_content_length; /* set if we got a content_length header */
	
	gboolean             tenc_chunked;  /* Transfer-Encoding: chunked */
	
	gchar               *buffer;
	gsize                bufalloc; /* number of bytes allocated             */
	gsize                buflen;   /* number of bytes of data in the buffer */

	GMainContext        *context;  /* we hold a reference, may be NULL */

	GMainLoop           *loop;

	guint                refcount; /* used internally only, if >1 we are in the callback */
};

typedef struct _GConnHttpHdr GConnHttpHdr;

struct _GConnHttpHdr
{
	gchar  *field;
	gchar  *value;  /* NULL = not set */
};

static const gchar *gen_headers[] =
{
	"Cache-Control", "Connection", "Date", "Pragma", "Trailer",
	"Transfer-Encoding", "Upgrade", "Via", "Warning"
};

static const gchar *req_headers[] =
{
	"Accept", "Accept-Charset", "Accept-Encoding", "Accept-Language",
	"Authorization", "Expect", "From", "Host", "If-Match", "If-Modified-Since",
	"If-None-Match", "If-Range", "If-Unmodified-Since", "Max-Forwards",
	"Proxy-Authorization", "Range", "Referer", "TE", "User-Agent",
	"Content-Length" /* for POST request */
};

#define is_general_header(field)  (is_in_str_arr(gen_headers,G_N_ELEMENTS(gen_headers),field))
#define is_request_header(field)  (is_in_str_arr(req_headers,G_N_ELEMENTS(req_headers),field))


static void       gnet_conn_http_delete_internal (GConnHttp *conn);

/***************************************************************************
 *
 *   gnet_conn_http_append_to_buf
 *
 ***************************************************************************/

static void
gnet_conn_http_append_to_buf (GConnHttp *conn, const gchar *data, gsize datalen)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (data != NULL);
	
	if (conn->buflen + datalen >= conn->bufalloc)
	{
		while (conn->buflen + datalen >= conn->bufalloc)
			conn->bufalloc += GNET_CONN_HTTP_BUF_INCREMENT;

		/* alloc one more to make sure buffer can be NUL-terminated later */
		conn->buffer = g_realloc (conn->buffer, conn->bufalloc + 1);
	}
        
	if (datalen > 0)
	{
		memcpy (conn->buffer + conn->buflen, data, datalen);
		conn->buflen += datalen;
	}
}

/***************************************************************************
 *
 *   gnet_conn_http_reset
 *
 *   reset certain values, but not the connection or URI or address
 *
 ***************************************************************************/

static void
gnet_conn_http_reset (GConnHttp *conn)
{
	GList *node;
	
	conn->num_redirects  = 0;
	conn->max_redirects  = GNET_CONN_HTTP_DEFAULT_MAX_REDIRECTS;
	g_free(conn->redirect_location);
	conn->redirect_location = NULL;
	
	conn->connection_close = FALSE;
	
	conn->content_length = 0;
	conn->content_recv = 0;
	conn->tenc_chunked = FALSE;
	conn->got_content_length = FALSE;

	/* Note: we keep the request headers as they are*/
	for (node = conn->resp_headers;  node;  node = node->next)
	{
		GConnHttpHdr *hdr = (GConnHttpHdr*)node->data;
		g_free(hdr->field);
		g_free(hdr->value);
		memset(hdr, 0xff, sizeof(GConnHttpHdr));
		g_free(hdr);
	}
	g_list_free(conn->resp_headers);
	conn->resp_headers = NULL;

	conn->response_code = 0;
	if (conn->method != GNET_CONN_HTTP_METHOD_POST)
	{
		g_free(conn->post_data);
		conn->post_data = NULL;
		conn->post_data_len = 0;
	}
	
	/* alloc one more to make sure buffer can be NUL-terminated later */
	conn->buffer   = g_realloc (conn->buffer, GNET_CONN_HTTP_BUF_INCREMENT + 1);
	conn->bufalloc = GNET_CONN_HTTP_BUF_INCREMENT;
	conn->buflen   = 0;
	
	conn->status = STATUS_NONE;
}

/**
 *  gnet_conn_http_new
 *
 *  Creates a #GConnHttp.
 *
 *  Returns: a #GConnHttp.
 *
 **/

GConnHttp *
gnet_conn_http_new (void)
{
	GConnHttp  *conn;

	conn = g_new0 (GConnHttp, 1);

	conn->stamp = CONN_HTTP_MAGIC_SEQUENCE;

	/* alloc one more to make sure buffer can be NUL-terminated later */
	conn->buffer   = g_malloc (GNET_CONN_HTTP_BUF_INCREMENT + 1);
	conn->bufalloc = GNET_CONN_HTTP_BUF_INCREMENT;
	conn->buflen   = 0;

	/* set default user agent */
	gnet_conn_http_set_user_agent (conn, NULL);

	gnet_conn_http_set_method (conn, GNET_CONN_HTTP_METHOD_GET, NULL, 0);

	gnet_conn_http_set_header (conn, "Accept", "*/*", 0);
	gnet_conn_http_set_header (conn, "Connection", "Keep-Alive", 0); 

	gnet_conn_http_set_timeout (conn, 30*1000); /* 30 secs */

	conn->refcount = 1;

	return conn;
}

/***************************************************************************
 *
 *   gnet_conn_http_new_event
 *
 ***************************************************************************/

static GConnHttpEvent * 
gnet_conn_http_new_event (GConnHttpEventType type)
{
	GConnHttpEvent *event = NULL;
	gsize           stsize = 0;

	switch (type)
	{
		case GNET_CONN_HTTP_RESOLVED:
			stsize = sizeof(GConnHttpEventResolved);
			break;

		case GNET_CONN_HTTP_RESPONSE:
			stsize = sizeof(GConnHttpEventResponse);
			break;

		case GNET_CONN_HTTP_REDIRECT:
			stsize = sizeof(GConnHttpEventRedirect);
			break;

		case GNET_CONN_HTTP_DATA_PARTIAL:
		case GNET_CONN_HTTP_DATA_COMPLETE:
			stsize = sizeof(GConnHttpEventData);
			break;
		
		case GNET_CONN_HTTP_ERROR:
			stsize = sizeof(GConnHttpEventError);
			break;

		case GNET_CONN_HTTP_CONNECTED:
		default:
			stsize = sizeof(GConnHttpEvent);
			break;
	}

	event = (GConnHttpEvent*) g_malloc0(stsize);
	event->type = type;
	event->stsize = stsize;

	return event;
}

/***************************************************************************
 *
 *   gnet_conn_http_free_event
 *
 ***************************************************************************/

static void
gnet_conn_http_free_event (GConnHttpEvent *event)
{
	g_return_if_fail (event != NULL);
	g_return_if_fail (event->stsize > 0);

	switch (event->type) {
		case GNET_CONN_HTTP_RESPONSE:
			g_strfreev(((GConnHttpEventResponse*)event)->header_fields);
			g_strfreev(((GConnHttpEventResponse*)event)->header_values);
			break;
		case GNET_CONN_HTTP_REDIRECT:
			g_free(((GConnHttpEventRedirect*)event)->new_location);
			break;
		case GNET_CONN_HTTP_ERROR:
			g_free(((GConnHttpEventError*)event)->message);
			break;
		default:
			break;
	}
	/* poison struct */
	memset(event, 0xff, event->stsize);
	g_free(event);
}

/***************************************************************************
 *
 *   gnet_conn_http_emit_event
 *
 *   Calls user callback with given event. You MUST check whether the
 *    connection has been deleted from the user callback via checking
 *    conn->recount == 0 and calling _delete_internal() if so at some
 *    point before giving control back to the main loop.
 *
 ***************************************************************************/

static void
gnet_conn_http_emit_event (GConnHttp *conn, GConnHttpEvent *event)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (event != NULL);

	++conn->refcount;

	if (conn->func)
		conn->func (conn, event, conn->func_data);

	g_return_if_fail (conn->refcount > 0);

	--conn->refcount;
}

/***************************************************************************
 *
 *   gnet_conn_http_emit_error_event
 *
 ***************************************************************************/

static void
gnet_conn_http_emit_error_event (GConnHttp *conn, GConnHttpError code, const gchar *format, ...)
{
	GConnHttpEventError *ev_err;
	GConnHttpEvent      *ev;
	va_list              args;
	
	g_return_if_fail (conn != NULL);
	
	conn->status = STATUS_ERROR;

	ev = gnet_conn_http_new_event(GNET_CONN_HTTP_ERROR);
	ev_err = (GConnHttpEventError*)ev;
	
	ev_err->code = code;
	
	va_start(args, format);
	ev_err->message = g_strdup_vprintf(format, args); 
	va_end(args);
	
	gnet_conn_http_emit_event(conn, ev);
	gnet_conn_http_free_event(ev);
	
	if (conn->loop)
		g_main_loop_quit(conn->loop);
}

/***************************************************************************
 *
 *   is_in_str_arr
 *
 ***************************************************************************/

static gboolean
is_in_str_arr (const gchar **arr, guint num, const gchar *field)
{
	guint i;

	g_return_val_if_fail (arr   != NULL, FALSE);
	g_return_val_if_fail (field != NULL, FALSE);

	for (i = 0;  i < num;  ++i)
	{
		if (g_ascii_strcasecmp(arr[i], field) == 0)
			return TRUE;
	}

	return FALSE;
}


/**
 *  gnet_conn_http_set_header
 *  @conn: a #GConnHttp
 *  @field: a header field, e.g. "Accept"
 *  @value: the header field value, e.g. "text/html"
 *  @flags: one or more flags of #GConnHttpHeaderFlags, or 0
 *
 *  Set header field to send with the HTTP request.
 *
 *  Returns: TRUE if the header field has been set or changed
 *
 **/

gboolean
gnet_conn_http_set_header (GConnHttp   *conn,
                           const gchar *field,
                           const gchar *value,
                           GConnHttpHeaderFlags flags)
{
	GConnHttpHdr *hdr;
	GList        *node;

	g_return_val_if_fail (conn  != NULL, FALSE);
	g_return_val_if_fail (field != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	/* Don't allow 'Host' to be set explicitely
	 *  we'll do that ourselves */
	if (g_ascii_strcasecmp(field, "Host") == 0)
		return FALSE;

	/* only allow uncommon header fields if
	 * explicitely allowed via the flags */
	if ((flags & GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK) == 0)
	{
		if (!(is_general_header(field) || is_request_header(field)))
			return FALSE;
	}

	for (node = conn->req_headers;  node;  node = node->next)
	{
		hdr = (GConnHttpHdr*)node->data;
		if (g_str_equal(hdr->field, field))
		{
			g_free(hdr->value);
			hdr->value = g_strdup(value);
			return TRUE;
		}
	}

	hdr = g_new0 (GConnHttpHdr, 1);
	hdr->field = g_strdup(field);
	hdr->value = g_strdup(value);

	conn->req_headers = g_list_append(conn->req_headers, hdr);

	return TRUE;
}


/**
 *  gnet_conn_http_set_user_agent
 *  @conn: a #GConnHttp
 *  @agent: the user agent string to send (will be supplemented by a GNet
 *          version number comment)
 *
 *  Convenience function. Wraps gnet_conn_http_set_header().
 *
 *  Returns: TRUE if the user agent string has been changed.
 *
 **/

gboolean
gnet_conn_http_set_user_agent (GConnHttp *conn, const gchar *agent)
{
	gboolean ret;
	gchar *full;

	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	if (agent == NULL)
	{
		agent = (const gchar *) g_get_prgname ();
		if (agent == NULL)
			agent = "GNet";
	}

	full = g_strdup_printf ("%s (GNet-%u.%u.%u)", agent, GNET_MAJOR_VERSION,
	                        GNET_MINOR_VERSION, GNET_MICRO_VERSION);

	ret = gnet_conn_http_set_header (conn, "User-Agent", full, 0);
	g_free (full);

	return ret;
}

static gboolean
gnet_conn_http_set_uri_internal (GConnHttp *conn, const gchar *uri,
    gboolean uri_is_escaped)
{
	gchar *old_hostname = NULL;

	g_assert (conn != NULL && uri != NULL);

	if (conn->uri)
	{
		old_hostname = g_strdup(conn->uri->hostname);
		gnet_uri_delete(conn->uri);
		conn->uri = NULL;
	}
	
	/* Add 'http://' prefix if no scheme/protocol is specified */
	if (strstr(uri,"://") == NULL)
	{
		gchar *full_uri = g_strconcat("http://", uri, NULL);
		conn->uri = gnet_uri_new(full_uri);
		g_free(full_uri);
	}
	else
	{
		if (g_ascii_strncasecmp(uri, "http:", 5) != 0)
			return FALSE; /* unsupported protocol */

		conn->uri = gnet_uri_new(uri);
	}
	
	if (conn->uri && old_hostname && g_ascii_strcasecmp(conn->uri->hostname, old_hostname) != 0)
	{
		if (conn->ia)
		{
			gnet_inetaddr_delete(conn->ia);
			conn->ia = NULL;
		}
		if (conn->conn)
		{
			gnet_conn_unref(conn->conn);
			conn->conn = NULL;
		}
	}

	g_free(old_hostname);

	if (conn->uri == NULL)
		return FALSE;

	gnet_uri_set_scheme(conn->uri, "http");

	if (!uri_is_escaped)
		gnet_uri_escape (conn->uri);

	return TRUE;
}

/**
 *  gnet_conn_http_set_uri
 *  @conn: a #GConnHttp
 *  @uri: URI string
 *
 *  Sets the URI to GET or POST, e.g. http://www.foo.com/bar.html. @uri is
 *  assumed to be unescaped, so all special URI characters will be escaped.
 *
 *  Returns: TRUE if the URI has been accepted.
 *
 **/

gboolean
gnet_conn_http_set_uri (GConnHttp *conn, const gchar *uri)
{
	g_return_val_if_fail (conn != NULL, FALSE);
	g_return_val_if_fail (uri  != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	return gnet_conn_http_set_uri_internal (conn, uri, FALSE);
}

/**
 *  gnet_conn_http_set_escaped_uri
 *  @conn: a #GConnHttp
 *  @uri: URI string with special characters already escaped
 *
 *  Sets the URI to GET or POST, e.g. http://www.foo.com/My%%20Documents/bar.txt
 *
 *  Returns: TRUE if the URI has been accepted.
 *
 **/

gboolean
gnet_conn_http_set_escaped_uri (GConnHttp *conn, const gchar *uri)
{
	g_return_val_if_fail (conn != NULL, FALSE);
	g_return_val_if_fail (uri  != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	return gnet_conn_http_set_uri_internal (conn, uri, TRUE);
}

/**
 *  gnet_conn_http_set_method
 *  @conn: a #GConnHttp
 *  @method: a #GConnHttpMethod
 *  @post_data: post data to send with POST method, or NULL
 *  @post_data_len: the length of the post data to send with POST method, or 0
 *
 *  Sets the HTTP request method. Default is the GET method.
 *
 *  Returns: TRUE if the method has been changed successfully.
 *
 **/

gboolean
gnet_conn_http_set_method (GConnHttp        *conn, 
                           GConnHttpMethod   method,
                           const gchar      *post_data,
                           gsize             post_data_len)
{
	g_return_val_if_fail (conn != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	switch (method)
	{
		case GNET_CONN_HTTP_METHOD_GET:
			conn->method = method;
			return TRUE;

		case GNET_CONN_HTTP_METHOD_POST:
		{
			g_return_val_if_fail (post_data != NULL, FALSE);
			g_return_val_if_fail (post_data_len > 0, FALSE);
			
			conn->method = method;
			
			g_free(conn->post_data);
			conn->post_data = g_memdup(post_data, post_data_len);
			conn->post_data = g_realloc(conn->post_data, post_data_len + 2 + 2 + 1);
			conn->post_data_len = post_data_len;
			
			conn->post_data[conn->post_data_len+0] = '\r';
			conn->post_data[conn->post_data_len+1] = '\n';
			conn->post_data[conn->post_data_len+2] = '\r';
			conn->post_data[conn->post_data_len+3] = '\n';
			conn->post_data[conn->post_data_len+4] = '\000';
			
			conn->post_data_term_len = 0;
			while (conn->post_data_len < 4 
			    || !g_str_equal(conn->post_data + conn->post_data_len + conn->post_data_term_len - 4, "\r\n\r\n")) 
				conn->post_data_term_len += 2;
			
			return TRUE;
		}

		default:
			break;
	}

	return FALSE;
}


/***************************************************************************
 *
 *   gnet_conn_http_conn_connected
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_connected (GConnHttp *conn)
{
	const gchar *resource;
	GString     *request;
	GList       *node;
	gchar       *res;

	gnet_conn_http_reset(conn);
	gnet_conn_timeout(conn->conn, conn->timeout);
	
	request = g_string_new(NULL);

	res = gnet_uri_get_string(conn->uri);
	resource = res + strlen(conn->uri->scheme) + strlen("://") + strlen(conn->uri->hostname);

	if (*resource == ':')
	{
		resource = strchr(resource, '/');
		if (resource == NULL || *resource == 0x00)
			resource = "/";
	}


	switch (conn->method)
	{
		case GNET_CONN_HTTP_METHOD_GET:
			g_string_append_printf (request, "GET %s HTTP/1.1\r\n", resource);
		break;

		case GNET_CONN_HTTP_METHOD_POST:
		{
			gchar  buf[16];
			

			/* Note: this must be 1.1 */
			g_string_append_printf (request, "POST %s HTTP/1.1\r\n", resource);
			
			g_snprintf(buf, sizeof(buf), "%u", (guint) conn->post_data_len);
			
			gnet_conn_http_set_header (conn, "Expect", "100-continue", 0);
			gnet_conn_http_set_header (conn, "Content-Length", buf, 0);
		}
		break;

		default:
			g_warning("Unknown http method in %s\n", __FUNCTION__);
			return;
	}

	for (node = conn->req_headers;  node;  node = node->next)
	{
		GConnHttpHdr *hdr = (GConnHttpHdr*)node->data;
		if (hdr->field && hdr->value && *hdr->field && *hdr->value)
		{
			g_string_append_printf(request, "%s: %s\r\n", hdr->field, hdr->value);
		}
	}

	if (conn->uri->port == 80)
	{
		g_string_append_printf(request, "Host: %s\r\n",
		                       conn->uri->hostname);
	}
	else
	{
		g_string_append_printf(request, "Host: %s:%u\r\n",
		                       conn->uri->hostname,
		                       conn->uri->port);
	}

	g_string_append(request, "\r\n");

/*	g_print ("Sending:\n%s\n", request->str); */
	gnet_conn_write(conn->conn, request->str, request->len);
	conn->status = STATUS_SENT_REQUEST;

	/* read response */
	gnet_conn_readline(conn->conn);

	g_string_free(request, TRUE);
	g_free(res);
}

/***************************************************************************
 *
 *   gnet_conn_http_done
 *
 *   Finished receiving data 
 *
 ***************************************************************************/

static void
gnet_conn_http_done (GConnHttp *conn)
{
	GConnHttpEventData *ev_data;
	GConnHttpEvent     *ev;
	
	conn->status = STATUS_DONE;

	gnet_conn_timeout (conn->conn, 0);

	/* we don't want to emit a DATA_COMPLETE event
	 *  if we are getting redirected, do we? */
	if (conn->redirect_location == NULL)
	{
		ev = gnet_conn_http_new_event (GNET_CONN_HTTP_DATA_COMPLETE);
		ev_data = (GConnHttpEventData*)ev;
		ev_data->buffer         = conn->buffer;
		ev_data->buffer_length  = conn->buflen;
		ev_data->content_length = conn->content_length;
		ev_data->data_received  = conn->content_recv;
		gnet_conn_http_emit_event (conn, ev);
		gnet_conn_http_free_event (ev);
	}

	if (conn->connection_close)
		gnet_conn_disconnect(conn->conn);
	
	/* need to do auto-redirect now? */
	if (conn->redirect_location && conn->refcount > 0)
	{
		if (gnet_conn_http_set_uri (conn, conn->redirect_location))
		{
			/* send request with new URI */
			gnet_conn_http_run_async (conn, conn->func, conn->func_data);

			return; /* do not quit own loop just yet */
		}
		
		gnet_conn_http_emit_error_event (conn, GNET_CONN_HTTP_ERROR_UNSPECIFIED, 
		                                 "Auto-redirect failed for some reason.");
	}
	
	if (conn->loop)
		g_main_loop_quit(conn->loop);
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_parse_response_headers
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_parse_response_headers (GConnHttp *conn)
{
	GConnHttpEventRedirect *ev_redirect;
	GConnHttpEventResponse *ev_response;
	GConnHttpEvent         *ev;
	const gchar            *new_location = NULL;
	guint                   num_headers, n;

	GList *node;
	
	num_headers = g_list_length(conn->resp_headers);

	ev = gnet_conn_http_new_event (GNET_CONN_HTTP_RESPONSE);
	ev_response = (GConnHttpEventResponse*)ev;
	ev_response->header_fields = g_new0(gchar *, num_headers+1);
	ev_response->header_values = g_new0(gchar *, num_headers+1);
	ev_response->response_code = conn->response_code;
	
	n = 0;
	conn->tenc_chunked = FALSE;
	for (node = conn->resp_headers;  node;  node = node->next)
	{
		GConnHttpHdr *hdr = (GConnHttpHdr*)node->data;
		
		ev_response->header_fields[n] = g_strdup(hdr->field); /* better safe than sorry */
		ev_response->header_values[n] = g_strdup(hdr->value);

		if (g_ascii_strcasecmp(hdr->field, "Content-Length") == 0)
		{
			conn->content_length = atoi(hdr->value);
			conn->got_content_length = TRUE;
		}
		else if (g_ascii_strcasecmp(hdr->field, "Transfer-Encoding") == 0 
		      && g_ascii_strcasecmp(hdr->value, "chunked") == 0)
		{
			conn->tenc_chunked = TRUE;
		}
		else if (g_ascii_strcasecmp(hdr->field, "Location") == 0)
		{
			new_location = hdr->value;
		}
		/* Note: amazon sends garbled 'Connection' string, but it 
		 *  might also be some apache module problem */
		else if (g_ascii_strcasecmp(hdr->field, "Connection") == 0
		      || g_ascii_strcasecmp(hdr->field, "Cneonction") == 0
		      || g_ascii_strcasecmp(hdr->field, "nnCoection") == 0)  
		{
			conn->connection_close = (g_ascii_strcasecmp(hdr->value, "close") == 0);
		}
		else
		{
			;
		}
		
		++n;
	}
	
	/* send generic response event first */
	gnet_conn_http_emit_event (conn, ev);
	gnet_conn_http_free_event (ev);
	
	/* If not a redirection code, continue normally */
	if (conn->response_code < 300 || conn->response_code >= 400)
		return; /* continue */
		
	ev = gnet_conn_http_new_event (GNET_CONN_HTTP_REDIRECT);
	ev_redirect = (GConnHttpEventRedirect*)ev;
	
	ev_redirect->num_redirects = conn->num_redirects;
	ev_redirect->max_redirects = conn->max_redirects;
	ev_redirect->auto_redirect = TRUE;

	if (conn->response_code == 301 && conn->method == GNET_CONN_HTTP_METHOD_POST)
		ev_redirect->auto_redirect = FALSE;

	if (conn->num_redirects >= conn->max_redirects)
		ev_redirect->auto_redirect = FALSE;

	/* No Location: header field? tough luck, can't do much */
	ev_redirect->new_location = g_strdup(new_location);
	if (new_location == NULL)
		ev_redirect->auto_redirect = FALSE;

	/* send generic redirect event (dispatch first to give
	 *  the client a chance to stop us before we do the
	 *  automatic redirection) */
	gnet_conn_http_emit_event (conn, ev);

	/* do the redirect later after receiving the body (if any) */
	if (ev_redirect->auto_redirect)
	{
		g_free (conn->redirect_location);
		conn->redirect_location = g_strdup(new_location);
	}

	gnet_conn_http_free_event (ev);

	return; /* continue and receive body */
}


/***************************************************************************
 *
 *   gnet_conn_http_conn_recv_response
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_recv_response (GConnHttp *conn, gchar *data, gsize len)
{
	gchar *endptr, *start;

	/* after a 100 Continue response, skip everything until we get a
	 * proper next response (there may be headers and there should
	 * definitely be an empty line following the 100 Continue response) */
	if (conn->method == GNET_CONN_HTTP_METHOD_POST &&
	    conn->response_code == 100 &&
	    !g_str_has_prefix (data, "HTTP/") &&
	    !g_str_has_prefix (data, "http/")) {
		gnet_conn_readline(conn->conn);
		return;
	}

	/* This should be a response line */
	if (g_ascii_strncasecmp (data, "ICY ", 4) != 0 &&
	    g_ascii_strncasecmp (data, "HTTP/", 5) != 0) {
		goto unsupported_proto;
	}

	start = strchr (data, ' ');
	if (start == NULL)
		goto malformed_response;

	/* FIXME: should we also check the HTTP protocol version? */
	conn->response_code = (guint) strtol(start+1, &endptr, 10);

	/* read first header */
	gnet_conn_readline(conn->conn);
		
	/* may we continue the POST request? */
	if (conn->response_code == 100 && conn->method == GNET_CONN_HTTP_METHOD_POST)
	{
		gnet_conn_write(conn->conn, conn->post_data, conn->post_data_len + conn->post_data_term_len);
		conn->status = STATUS_SENT_REQUEST; /* expecting the response for the content next */
		return;
	}

	/* note: redirection is handled after we have all headers */

	conn->status = STATUS_RECV_HEADERS; /* response ok - expect headers next */
	return;

/* ERRORS */
unsupported_proto:
  {
    conn->response_code = 0; 
    gnet_conn_http_emit_error_event (conn,
        GNET_CONN_HTTP_ERROR_PROTOCOL_UNSUPPORTED, 
        "Not a HTTP or ICY protocol response: '%s'", data);
    return;
  }
malformed_response:
  {
    conn->response_code = 0; 
    gnet_conn_http_emit_error_event (conn,
        GNET_CONN_HTTP_ERROR_PROTOCOL_UNSUPPORTED, 
        "Malformed response: '%s'", data);
    return;
  }
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_recv_headers
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_recv_headers (GConnHttp *conn, gchar *data, gsize len)
{
	gchar *colon;
	
	/* End of headers? */
	if (*data == 0x00 || g_str_equal(data,"\r\n") || g_str_equal(data,"\r") || g_str_equal(data,"\n"))
	{
		gnet_conn_http_conn_parse_response_headers (conn);

		if (conn->got_content_length && conn->content_length == 0)
		{
			/* no body to receive */
			gnet_conn_http_done (conn);
			return;
		}
		else if (conn->tenc_chunked)
		{
			/* read line with chunk size of first data chunk */
			gnet_conn_readline(conn->conn);
			conn->status = STATUS_RECV_CHUNK_SIZE;
			return;
		}
		else
		{
			gnet_conn_read(conn->conn);
			conn->status = STATUS_RECV_BODY_NONCHUNKED;
			return;
		}
		
		g_return_if_reached();
	}

	/* this is a normal header line then */
	colon = strchr(data, ':');
	if (colon)
	{
		GConnHttpHdr *hdr;

		*colon = 0x00;
	
		hdr = g_new0 (GConnHttpHdr, 1);
		hdr->field = g_strdup(data);
		hdr->value = g_strstrip(g_strdup(colon+1));
	
		conn->resp_headers = g_list_append(conn->resp_headers, hdr);
	}

	/* read next header line */
	gnet_conn_readline(conn->conn);
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_recv_chunk_size
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_recv_chunk_size (GConnHttp *conn, gchar *data, gsize len)
{
	gchar *endptr;
	gsize  chunksize;
			
	chunksize = (gsize) strtol (data, &endptr, 16);
	
	if (chunksize == 0)
	{
		gnet_conn_http_done(conn);
		return;
	}
	
	gnet_conn_readn(conn->conn, chunksize+2); /* including \r\n */
	conn->status = STATUS_RECV_CHUNK_BODY;
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_recv_chunk_body
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_recv_chunk_body (GConnHttp *conn, gchar *data, gsize len)
{
	/* do not count the '\r\n' at the end of a block */
	if (len >=2 && data[len-2] == '\r' && data[len-1] == '\n')
		len -= 2;
	
	conn->content_recv += len;
	gnet_conn_http_append_to_buf(conn, data, len);

	/* we don't want to emit data events if we're 
	 *  getting redirected anyway, do we? */
	if (conn->redirect_location == NULL)
	{
		GConnHttpEventData *ev_data;
		GConnHttpEvent     *ev;

		ev = gnet_conn_http_new_event(GNET_CONN_HTTP_DATA_PARTIAL);
		ev_data = (GConnHttpEventData*)ev;
		ev_data->buffer = conn->buffer;
		ev_data->buffer_length = conn->buflen;
		ev_data->content_length = conn->content_length;
		ev_data->data_received  = conn->content_recv;
		gnet_conn_http_emit_event(conn, ev);
		gnet_conn_http_free_event(ev);
	}

	/* read line with chunk size of next chunk */
	gnet_conn_readline(conn->conn);

	conn->status = STATUS_RECV_CHUNK_SIZE;
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_recv_nonchunked_data
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_recv_nonchunked_data (GConnHttp *conn, gchar *data, gsize len)
{
	if (conn->content_length > 0)
	{
		conn->content_recv += len;
		gnet_conn_http_append_to_buf(conn, data, len);

		if (conn->content_recv >= conn->content_length)
		{
			gnet_conn_http_done(conn);
			return;
		}
		
		gnet_conn_read(conn->conn);
	}
	else
	{
		conn->content_recv += len;
		gnet_conn_http_append_to_buf (conn, data, len);
		gnet_conn_read (conn->conn);
	}

	/* we don't want to emit data events if we're 
	 *  getting redirected anyway, do we? */
	if (conn->redirect_location == NULL)
	{
		GConnHttpEventData *ev_data;
		GConnHttpEvent     *ev;

		ev = gnet_conn_http_new_event(GNET_CONN_HTTP_DATA_PARTIAL);
		ev_data = (GConnHttpEventData*)ev;
		ev_data->buffer = conn->buffer;
		ev_data->buffer_length = conn->buflen;
		ev_data->content_length = conn->content_length;
		ev_data->data_received  = conn->content_recv;
		gnet_conn_http_emit_event(conn, ev);
		gnet_conn_http_free_event(ev);
	}
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_got_data
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_got_data (GConnHttp *conn, gchar *data, gsize len)
{
	gnet_conn_timeout (conn->conn, conn->timeout);
	
	switch (conn->status)
	{
		/* sent request - so this must be the response code */
		case STATUS_SENT_REQUEST:
			gnet_conn_http_conn_recv_response(conn, data, len);
			break;
		
		case STATUS_RECV_HEADERS:
			gnet_conn_http_conn_recv_headers(conn, data, len);
			break;

		case STATUS_RECV_BODY_NONCHUNKED:
			gnet_conn_http_conn_recv_nonchunked_data(conn, data, len);
			break;

		case STATUS_RECV_CHUNK_SIZE:
			gnet_conn_http_conn_recv_chunk_size(conn, data, len);
			break;

		case STATUS_RECV_CHUNK_BODY:
			gnet_conn_http_conn_recv_chunk_body(conn, data, len);
			break;

		default:
			gnet_conn_http_emit_error_event(conn, GNET_CONN_HTTP_ERROR_UNSPECIFIED,
			                                "%s: should not be reached. conn->status = %u",
			                                G_STRLOC, conn->status);
			break;
	}
}

/***************************************************************************
 *
 *   gnet_conn_http_conn_cb
 *
 *   GConn callback function
 *
 ***************************************************************************/

static void
gnet_conn_http_conn_cb (GConn *c, GConnEvent *event, GConnHttp *httpconn)
{
	GConnHttpEvent *ev;

	g_return_if_fail (GNET_IS_CONN_HTTP (httpconn));
	
	switch (event->type)
	{
		case GNET_CONN_ERROR:
			gnet_conn_disconnect(httpconn->conn);
			gnet_conn_http_emit_error_event(httpconn, GNET_CONN_HTTP_ERROR_UNSPECIFIED,
			                                "GNET_CONN_ERROR (unspecified Gnet GConn error)");
			if (httpconn->loop)
				g_main_loop_quit(httpconn->loop);
			break;
		
		case GNET_CONN_CONNECT:
			gnet_conn_http_conn_connected(httpconn);
			break;
		
		case GNET_CONN_CLOSE:
			gnet_conn_disconnect(httpconn->conn);
			/* _done() will take care of redirection and main loop quitting */
			gnet_conn_http_done(httpconn);
			break;
		
		case GNET_CONN_TIMEOUT:
			ev = gnet_conn_http_new_event(GNET_CONN_HTTP_TIMEOUT);
			gnet_conn_http_emit_event(httpconn, ev);
			gnet_conn_http_free_event(ev);
			if (httpconn->loop)
				g_main_loop_quit(httpconn->loop);
			break;
		
		case GNET_CONN_READ:
			/* Note: GConn translates reads of 0 bytes into CLOSE for us */
			gnet_conn_http_conn_got_data(httpconn, event->buffer, event->length);
			break;
		
		case GNET_CONN_WRITE:
		case GNET_CONN_READABLE:
		case GNET_CONN_WRITABLE:
			break;
	}

	if (httpconn->refcount == 0)
		gnet_conn_http_delete_internal (httpconn);
}

/***************************************************************************
 *
 *   gnet_conn_http_ia_cb
 *
 *   GInetAddr async resolve callback function
 *
 ***************************************************************************/

static void
gnet_conn_http_ia_cb (GInetAddr *ia, GConnHttp *conn)
{
	conn->ia_id = 0;

	g_return_if_fail (GNET_IS_CONN_HTTP (conn));

	if (ia != conn->ia || ia == NULL)
	{
		GConnHttpEventResolved *ev_resolved;
		GConnHttpEvent         *ev;
		
		conn->ia = ia;
	
		ev = gnet_conn_http_new_event (GNET_CONN_HTTP_RESOLVED);
		ev_resolved = (GConnHttpEventResolved*)ev;
		ev_resolved->ia = conn->ia;
		gnet_conn_http_emit_event (conn, ev);
		gnet_conn_http_free_event (ev);

		if (conn->refcount == 0)
		{
			gnet_conn_http_delete_internal (conn);
			return;
		}
	}
		
	/* could not resolve hostname? => emit error event and quit */
	if (ia == NULL)
	{
		if (conn->loop)
			g_main_loop_quit(conn->loop);

		gnet_conn_http_emit_error_event (conn, GNET_CONN_HTTP_ERROR_HOSTNAME_RESOLUTION,
		                                 "Could not resolve hostname '%s'",
		                                 conn->uri->hostname);

		return;
	}

	if (conn->conn == NULL)
	{
		conn->conn = gnet_conn_new_inetaddr (ia, (GConnFunc) gnet_conn_http_conn_cb, conn);
	
		if (conn->conn == NULL)
		{
			gnet_conn_http_emit_error_event(conn, GNET_CONN_HTTP_ERROR_UNSPECIFIED,
			                                "%s: Could not create GConn object.",
			                                G_STRLOC);
			return;
		}

		gnet_conn_set_main_context (conn->conn, conn->context);
		gnet_conn_timeout(conn->conn, conn->timeout); 
		gnet_conn_connect(conn->conn);
		gnet_conn_set_watch_error(conn->conn, TRUE);
		return;
	}

	/* re-use existing connection? */
	if (!gnet_conn_is_connected (conn->conn))
	{
		gnet_conn_timeout(conn->conn, conn->timeout);
		gnet_conn_connect(conn->conn);
	}
	else
	{
		gnet_conn_http_conn_connected(conn);
	}
}

/**
 *  gnet_conn_http_run_async
 *  @conn: a #GConnHttp
 *  @func: callback function to communicate progress and errors, or NULL
 *  @user_data: user data to pass to callback function, or NULL
 *
 *  Starts connecting and sending the specified http request. Will 
 *   return immediately. Assumes there is an existing and running 
 *   default Gtk/GLib/Gnome main loop. 
 *
 **/

void
gnet_conn_http_run_async (GConnHttp        *conn,
                          GConnHttpFunc     func,
                          gpointer          user_data)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (GNET_IS_CONN_HTTP (conn));
	g_return_if_fail (func != NULL || user_data == NULL);
	g_return_if_fail (conn->uri != NULL);
	g_return_if_fail (conn->ia_id == 0);
	
	conn->func = func;
	conn->func_data = user_data;

	if (conn->uri->port == 0)
		gnet_uri_set_port(conn->uri, 80);  // FIXME: 8080 for http proxies

	if (conn->ia == NULL)
	{
		conn->ia_id = gnet_inetaddr_new_async_full (conn->uri->hostname,
		    conn->uri->port, (GInetAddrNewAsyncFunc) gnet_conn_http_ia_cb,
		    conn, (GDestroyNotify) NULL, conn->context, G_PRIORITY_DEFAULT);
	}
	else
	{
		gnet_conn_http_ia_cb(conn->ia, conn);
	}
}

/**
 *  gnet_conn_http_run
 *  @conn: a #GConnHttp
 *  @func: callback function to communicate progress and errors, or NULL
 *  @user_data: user data to pass to callback function, or NULL
 *
 *  Starts connecting and sending the specified http request. Will 
 *   return once the operation has finished and either an error has 
 *   occured, or the data has been received in full.
 *
 *   This function will run its own main loop in the default GLib main context
 *   (or the user-specified main context, if one was specified with
 *   gnet_conn_http_set_main_context()), which means that if your application
 *   is based on Gtk+ or sets up GLib timeouts or idle callbacks, it is
 *   possible that those callback functions are invoked while you are waiting
 *   for gnet_conn_http_run() to return. This means you shouldn't make
 *   assumptions about any state you set up before calling this function,
 *   because it might have been changed again from within a callback in the
 *   mean time (if this can happen or not depends on your callbacks and what
 *   they do of course).
 *
 *   Returns: TRUE if no error occured before connecting
 *
 **/

gboolean
gnet_conn_http_run (GConnHttp        *conn,
                    GConnHttpFunc     func,
                    gpointer          user_data)
{
	g_return_val_if_fail (conn      != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);
	g_return_val_if_fail (conn->uri != NULL, FALSE);
	g_return_val_if_fail (conn->ia_id == 0, FALSE);

	conn->func = func;
	conn->func_data = user_data;

	if (conn->uri->port == 0)
		gnet_uri_set_port(conn->uri, 80);  // FIXME: 8080 for http proxies

	if (conn->ia == NULL)
	{
		conn->ia_id = gnet_inetaddr_new_async_full (conn->uri->hostname,
		    conn->uri->port, (GInetAddrNewAsyncFunc) gnet_conn_http_ia_cb,
		    conn, (GDestroyNotify) NULL, conn->context, G_PRIORITY_DEFAULT);
	}
	else
	{
		gnet_conn_http_ia_cb(conn->ia, conn);
	}

	conn->loop = g_main_loop_new (NULL, FALSE);
	
	g_main_loop_run (conn->loop);

	if (conn->status == STATUS_DONE)
	{
		if (conn->content_length > 0)
			return (conn->content_recv >= conn->content_length);
		
		return (conn->content_recv > 0);
	}
	
	return FALSE;
}


/**
 *  gnet_conn_http_steal_buffer
 *  @conn: a #GConnHttp
 *  @buffer: where to store a pointer to the buffer data
 *  @length: where to store the length of the buffer data
 *
 *  Empties the current buffer and returns the contents. The 
 *   main purpose of this function is to make it possible to 
 *   just use gnet_conn_http_run(), check its return value, and 
 *   then get the buffer data without having to install a 
 *   callback function. Also needed to empty the buffer regularly
 *   while receiving large amounts of data.
 *
 *  The caller (you) needs to free the buffer with g_free() when done.
 *
 *  Returns: TRUE if buffer and length have been set
 *
 **/

gboolean         
gnet_conn_http_steal_buffer (GConnHttp        *conn,
                             gchar           **buffer,
                             gsize            *length)
{
	g_return_val_if_fail (conn   != NULL, FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (length != NULL, FALSE);
	g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);

	if (conn->status == STATUS_NONE 
	 || conn->status == STATUS_SENT_REQUEST
	 || conn->status == STATUS_ERROR)
		return FALSE;

	*length = conn->buflen;
	*buffer = conn->buffer;

	/* we allocated +1 bytes earlier on, so that there
	 *  is always space for an additional NUL here */
	conn->buffer[conn->buflen] = '\0';

	/* alloc one more to make sure buffer can be NUL-terminated later */
	conn->buffer   = g_malloc (GNET_CONN_HTTP_BUF_INCREMENT + 1);
	conn->bufalloc = GNET_CONN_HTTP_BUF_INCREMENT;
	conn->buflen   = 0;
	
	return TRUE;
}


/**
 *  gnet_conn_http_set_max_redirects
 *  @conn: a #GConnHttp
 *  @num: the maximum number of allowed automatic redirects
 *
 *  Sets the maximum allowed number of automatic redirects.
 *   Note that the HTTP protocol specification (RFC2616) specifies 
 *   occasions where the client must not redirect automatically 
 *   without user intervention. In those cases, no automatic redirect
 *   will be performed, even if the limit has not been reached yet.
 *
 **/

void             
gnet_conn_http_set_max_redirects (GConnHttp *conn, guint num)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (num > 100);
	
	conn->max_redirects = num;
}

/***************************************************************************
 *
 *   gnet_conn_http_delete_internal
 *
 ***************************************************************************/

static void
gnet_conn_http_delete_internal (GConnHttp *conn)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (conn->refcount == 0);

	if (conn->ia_id > 0)
		gnet_inetaddr_new_async_cancel(conn->ia_id);

	if (conn->ia)
		gnet_inetaddr_delete(conn->ia);
		
	if (conn->conn)
		gnet_conn_unref(conn->conn);

	/* concatenate them into resp_headers, so that 
	 *  they will be freed in gnet_conn_http_reset() */
	conn->resp_headers = g_list_concat(conn->resp_headers, conn->req_headers);
	conn->req_headers = NULL;

	gnet_conn_http_reset(conn);

	if (conn->uri)
		gnet_uri_delete(conn->uri);
	
	if (conn->loop != NULL  &&  g_main_loop_is_running(conn->loop))
	{
		g_warning("conn->loop != NULL and still running. This indicates"
		          "\ta bug in your code! You are not allowed to call\n"
		          "\tgnet_conn_http_delete() before gnet_conn_http_run()\n"
		          "\thas returned. Use gnet_conn_http_cancel() instead.\n");
	}
	
	if (conn->loop)
		g_main_loop_unref(conn->loop);

	if (conn->context)
		g_main_context_unref (conn->context);

	g_free(conn->post_data);

	g_free(conn->buffer);

	memset(conn, 0xff, sizeof(GConnHttp));
	g_free(conn);
}

/**
 *  gnet_conn_http_delete
 *  @conn: a #GConnHttp
 *
 *  Deletes a #GConnHttp and frees all associated resources.
 *
 **/

void             
gnet_conn_http_delete (GConnHttp *conn)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (GNET_IS_CONN_HTTP (conn));
	g_return_if_fail (conn->refcount > 0);

	--conn->refcount;

	/* if the refcount was greater than 1, we are
	 *  still calling out into the user callback.
	 *  In that case, keep the object alive for now,
	 *  but invalidate it */
	if (conn->refcount > 0)
	{
		conn->stamp = 0;
		return;
	}

	gnet_conn_http_delete_internal (conn);
}


/**
 *  gnet_conn_http_cancel
 *  @conn: a #GConnHttp
 *
 *  Cancels the current http transfer (if any) and makes
 *   gnet_conn_http_run() return immediately. Will do nothing
 *   if the transfer was started with gnet_conn_http_run_async().
 *
 **/

void             
gnet_conn_http_cancel (GConnHttp *conn)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (GNET_IS_CONN_HTTP (conn));
	
	if (conn->loop)
		g_main_loop_quit(conn->loop);
}


/**
 *  gnet_conn_http_set_timeout
 *  @conn: a #GConnHttp
 *  @timeout: timeout in milliseconds
 *
 *  Sets a timeout on the http connection.
 *
 **/

void             
gnet_conn_http_set_timeout (GConnHttp *conn, guint timeout)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (GNET_IS_CONN_HTTP (conn));

	conn->timeout = timeout;
}

/***************************************************************************
 *
 *   gnet_http_get_cb
 *
 ***************************************************************************/

static void
gnet_http_get_cb (GConnHttp *conn, GConnHttpEvent *event, gpointer user_data)
{
	guint *p_response = (guint*) user_data;
  
	if (p_response != NULL  &&  event->type == GNET_CONN_HTTP_RESPONSE)
	{
		GConnHttpEventResponse *revent;
                revent = (GConnHttpEventResponse*) event;
		*p_response = revent->response_code;
	}
}

/**
 *  gnet_http_get
 *  @url: a URI, e.g. http://www.foo.com
 *  @buffer: where to store a pointer to the data retrieved
 *  @length: where to store the length of the data retrieved
 *  @response: where to store the last HTTP response code 
 *  received from the HTTP server, or NULL.
 *
 *  Convenience function that just retrieves
 *   the provided URI without the need to 
 *   set up a full #GConnHttp. Uses 
 *   gnet_conn_http_run() internally.
 *
 *  Caller (you) needs to free the buffer with g_free() when
 *   no longer needed.
 *
 *  This function will run its own main loop in the default GLib main context,
 *   which means that if your application is based on Gtk+ or sets up GLib
 *   timeouts or idle callbacks, it is possible that those callback functions
 *   are invoked while you are waiting for gnet_http_get() to return. This
 *   means you shouldn't make assumptions about any state you set up before
 *   calling this function, because it might have been changed again from
 *   within a callback in the mean time (if this can happen or not depends on
 *   your callbacks and what they do of course).
 *
 *  Returns: TRUE if @buffer, @length and @response are set,
 *   otherwise FALSE.
 *
 **/

gboolean 
gnet_http_get (const gchar  *url, 
               gchar       **buffer, 
               gsize        *length, 
               guint        *response)
{
	GConnHttp *conn;
	gboolean   ret;        

	g_return_val_if_fail (url != NULL && *url != 0x00, FALSE);
	g_return_val_if_fail (buffer != NULL, FALSE);
	g_return_val_if_fail (length != NULL, FALSE);

	if (response)
		*response = 0;

	ret = FALSE;
	conn = gnet_conn_http_new();

	if (gnet_conn_http_set_uri(conn,url))
	{
		if (gnet_conn_http_run(conn, gnet_http_get_cb, response))
		{
			if (gnet_conn_http_steal_buffer(conn, buffer, length))
			{
				ret = TRUE;
			}
		}
	}
               
	gnet_conn_http_delete(conn);

	return ret;
}

/**
 *  gnet_conn_http_set_main_context:
 *  @conn: a #GConnHttp
 *  @context: a #GMainContext, or NULL to use the default GLib main context
 *
 *  Sets the GLib #GMainContext to use for asynchronous operations. You should
 *  call this function right after you create @conn. You must not call this
 *  function after the actual connection process has started.
 *
 *  You are very unlikely to ever need this function.
 *
 *  Returns: TRUE on success, FALSE on failure.
 *
 *  Since: 2.0.8
 **/
gboolean
gnet_conn_http_set_main_context (GConnHttp * conn, GMainContext * context)
{
  g_return_val_if_fail (conn != NULL, FALSE);
  g_return_val_if_fail (GNET_IS_CONN_HTTP (conn), FALSE);
  g_return_val_if_fail (conn->conn == NULL && conn->ia_id == NULL, FALSE);

  if (conn->context != context) {
    if (conn->context)
      g_main_context_unref (conn->context);
    if (context)
      conn->context = g_main_context_ref (context);
    else
      conn->context = NULL;
  }

  return TRUE;
}

