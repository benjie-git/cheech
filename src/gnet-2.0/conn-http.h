/* GNet - Networking library
 * Copyright (C) 2000-2004  David Helder
 * Copyright (C) 2004       Tim-Philipp Müller <tim centricular net>
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


#ifndef _GNET_CONN_HTTP_H
#define _GNET_CONN_HTTP_H

#include "conn.h"
#include "uri.h"
#include "inetaddr.h"

G_BEGIN_DECLS

/**
 *   GConnHttpHeaderFlags
 *   @GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK: do not check whether
 *   the header is a standard header
 *
 *   Flags for gnet_conn_http_set_header().
 *
 **/
typedef enum
{
  GNET_CONN_HTTP_FLAG_SKIP_HEADER_CHECK  = 1
} GConnHttpHeaderFlags;

/**
 *   GConnHttpMethod
 *   @GNET_CONN_HTTP_METHOD_GET: HTTP GET method
 *   @GNET_CONN_HTTP_METHOD_POST: HTTP POST method
 *
 *   HTTP request method.  Use with gnet_conn_http_set_method().
 *
 **/
typedef enum
{
 GNET_CONN_HTTP_METHOD_GET,
 GNET_CONN_HTTP_METHOD_POST
} GConnHttpMethod;

/**
 *  GConnHttp
 *
 *  HTTP Connection. The entire GConnHttp struct is opaque and private. 
 *
 **/
typedef struct _GConnHttp GConnHttp;

/**
 *   GConnHttpError
 *   @GNET_CONN_HTTP_ERROR_UNSPECIFIED: connection error
 *   @GNET_CONN_HTTP_ERROR_PROTOCOL_UNSUPPORTED: unsupported protocol (ie. not http)
 *   @GNET_CONN_HTTP_ERROR_HOSTNAME_RESOLUTION: the hostname could not be resolved
 *
 *   Error codes.  Used by #GConnHttpEventError.  Note that 
 *    errors by the HTTP server will be communicated to the
 *    client via the #GConnHttpEventResponse event.
 *
 **/
typedef enum
{
 GNET_CONN_HTTP_ERROR_UNSPECIFIED,
 GNET_CONN_HTTP_ERROR_PROTOCOL_UNSUPPORTED,
 GNET_CONN_HTTP_ERROR_HOSTNAME_RESOLUTION
} GConnHttpError;


/***************************************************************************
 *                                                                         *
 *   Event structures - work similar to GdkEvent                           *
 *                                                                         *
 ***************************************************************************/

/**
 *   GConnHttpEventType
 *   @GNET_CONN_HTTP_RESOLVED: the host name has been 
 *   resolved or host name resolution failed. The event
 *   structure will be a #GConnHttpEventResolved structure
 *   @GNET_CONN_HTTP_CONNECTED: the TCP connection to 
 *   the HTTP server has been established
 *   @GNET_CONN_HTTP_RESPONSE: the HTTP server has sent 
 *   a response and response headers. The event
 *   structure will be a #GConnHttpEventResponse structure
 *   @GNET_CONN_HTTP_REDIRECT: the HTTP server has sent 
 *   a redirect response. The event structure will be a
 *   #GConnHttpEventRedirect structure
 *   @GNET_CONN_HTTP_DATA_PARTIAL: data has been received.
 *   The buffer is caller-owned (ie. owned by GNet), but 
 *   may be emptied using gnet_conn_http_steal_buffer(). The
 *   event structure will be a #GConnHttpEventData structure
 *   @GNET_CONN_HTTP_DATA_COMPLETE: data has been received
 *   in full.  The buffer is caller-owned (ie. owned by GNet), 
 *   but may be emptied using gnet_conn_http_steal_buffer(). The
 *   event structure will be a #GConnHttpEventData structure
 *   @GNET_CONN_HTTP_TIMEOUT: the connection timed out
 *   @GNET_CONN_HTTP_ERROR: #GConnHttp problem (not a server 
 *   error response). The event structure will be a 
 *   #GConnHttpEventError structure
 *
 *   GConnHttp event type. 
 *
 **/
typedef enum
{
 GNET_CONN_HTTP_RESOLVED,           /* resolved host name           */
 GNET_CONN_HTTP_CONNECTED,          /* connected to host            */
 GNET_CONN_HTTP_RESPONSE,           /* got response (incl. headers) */
 GNET_CONN_HTTP_REDIRECT,           /* got redirect                 */
 GNET_CONN_HTTP_DATA_PARTIAL,       /* we got some data             */
 GNET_CONN_HTTP_DATA_COMPLETE,      /* we got all data              */
 GNET_CONN_HTTP_TIMEOUT,            /* the connection timed out     */
 GNET_CONN_HTTP_ERROR               /* GConnHttp problem            */
} GConnHttpEventType;

typedef struct _GConnHttpEvent         GConnHttpEvent;
typedef struct _GConnHttpEventResolved GConnHttpEventResolved;
typedef struct _GConnHttpEventResponse GConnHttpEventResponse;
typedef struct _GConnHttpEventRedirect GConnHttpEventRedirect;
typedef struct _GConnHttpEventData     GConnHttpEventData; 
typedef struct _GConnHttpEventError    GConnHttpEventError; 

/**
 *  GConnHttpEvent
 *  @type: event type
 *
 *  GConnHttp Base Event.  Check event->type and then cast the event
 *   structure into the corresponding specialised event structure.
 *
 **/
struct _GConnHttpEvent
{
 GConnHttpEventType   type;           /* type of event (see above)         */
 
 /*< private >*/
 gsize                stsize;         /* size of underlying event struct   */
 gpointer             padding[4];     /* padding for future expansion      */
};

/**
 *  GConnHttpEventResolved
 *  @ia: a #GInetAddr of the resolved host name.
 *
 *  GConnHttp Host Name Resolved Event.  Emitted when the host name has 
 *   been resolved to an IP address, primarily to give progress feedback 
 *   to the user. @ia will be NULL if the host name could not be resolved.
 *
 **/
struct _GConnHttpEventResolved
{
 /*< private >*/
 GConnHttpEvent       parent;         /* GConnHttpEvent structure          */
 
 /*< public >*/
 GInetAddr           *ia;             /* GInetAddr of the host name        */
 
 /*< private >*/
 gpointer             padding[4];     /* padding for future expansion      */
};

/**
 *  GConnHttpEventRedirect
 *  @num_redirects: number of redirects so far
 *  @max_redirects: maximum number of redirects allowed
 *  @new_location: redirect location, or NULL if not provided
 *  @auto_redirect: FALSE if action by the client is needed. Set 
 *                  to FALSE to prevent automatic redirection
 *
 *  Emitted when the server sends a redirect response.
 *
 **/
struct _GConnHttpEventRedirect
{
 /*< private >*/
 GConnHttpEvent       parent;         /* GConnHttpEvent structure          */
 
 /*< public >*/
 guint                num_redirects;  /* number of redirects so far        */
 guint                max_redirects;  /* max. num. of redirects allowed    */
 gchar               *new_location;   /* redirect location if provided     */
 gboolean             auto_redirect;  /* FALSE if action is needed         */
 
 /*< private >*/
 gpointer             padding[4];     /* padding for future expansion      */
};

/**
 *  GConnHttpEventResponse
 *  @response_code: response code from the HTTP server (e.g. 200 or 404)
 *  @header_fields: array of header field strings, NULL-terminated
 *  @header_values: array of header value strings, NULL-terminated
 *
 *  Emitted when the server has sent a response and response headers.
 *
 **/
struct _GConnHttpEventResponse
{
 /*< private >*/
 GConnHttpEvent       parent;         /* GConnHttpEvent structure          */

 /*< public >*/
 guint                response_code;  /* response code, e.g. 200, or 404   */
 gchar              **header_fields;  /* NULL-terminated array of strings  */
 gchar              **header_values;  /* NULL-terminated array of strings  */

 /*< private >*/
 gpointer             padding[8];     /* padding for future expansion      */
};

/**
 *  GConnHttpEventData
 *  @content_length: set if available, otherwise 0
 *  @data_received: total amount of data received so far
 *  @buffer: buffer with data received so far. Use 
 *  gnet_conn_http_steal_buffer() to empty the buffer.
 *  @buffer_length: buffer length
 *
 *  Emitted when data has been received. Useful for progress feedback 
 *   to the user or to process data before all of it has been received.
 *   The client is responsible for emptying the buffer regularly when the 
 *   amount of data received or expected is larger than the amount that
 *   should be kept in memory (e.g. in the case of large binary files).
 *
 **/
struct _GConnHttpEventData
{
 /*< private >*/
 GConnHttpEvent       parent;         /* GConnHttpEvent structure          */

 /*< public >*/
 guint64              content_length; /* set if available, otherwise 0     */
 guint64              data_received;  /* bytes received so far             */
 const gchar         *buffer;         /* buffer                            */
 gsize                buffer_length;  /* buffer length                     */
 
 /*< private >*/
 gpointer             padding[8];     /* padding for future expansion      */
};

/**
 *  GConnHttpEventError
 *  @code: one of the #GConnHttpError codes
 *  @message: clear-text error message (for debugging purposes only)
 *
 *  Emitted when an error has occured. Note that HTTP server errors are
 *   communicated to the client by means of a #GConnHttpEventResponse event.
 *
 **/
struct _GConnHttpEventError
{
 /*< private >*/
 GConnHttpEvent       parent;         /* GConnHttpEvent structure          */
 
 /*< public >*/
 GConnHttpError       code;           /* error code                        */
 gchar               *message;        /* message (use for debugging only)  */
 
 /*< private >*/
 gpointer             padding[4];     /* padding for future expansion      */
};

/***************************************************************************
 *                                                                         *
 *   GConnHttp callback function prototype                                 *
 *                                                                         *
 ***************************************************************************/

/**
 *  GConnHttpFunc
 *  @conn: #GConnHttp
 *  @event: event (caller-owned, do not free)
 *  @user_data: user data specified in gnet_conn_http_run() 
 *              or gnet_conn_http_run_async()
 * 
 *  Callback for #GConnHttp.
 *
 *  Check event->type and then cast the event into the appropriate
 *   event structure. event->type will be one of
 *
 *  %GNET_CONN_HTTP_RESOLVED: this event occurs when the host name
 *  has been resolved or host name resolution failed
 *
 *  %GNET_CONN_HTTP_CONNECTED: the TCP connection to the 
 *  HTTP server has been established
 *
 *  %GNET_CONN_HTTP_RESPONSE: the HTTP server has sent a response
 *  and response headers
 *
 *  %GNET_CONN_HTTP_REDIRECT: the HTTP server has sent a redirect
 *  response
 *
 *  %GNET_CONN_HTTP_DATA_PARTIAL: data has been read. The buffer is
 *  owned by GNet and you must not modify it or free it. You can
 *  take ownership of the buffer with gnet_conn_http_steal_buffer()
 *
 *  %GNET_CONN_HTTP_DATA_COMPLETE: data has been received in full.
 *  The buffer is owned by GNet and you must not modify it or free 
 *  it. You can acquire ownership of the buffer by calling 
 *  gnet_conn_http_steal_buffer()
 *
 *  %GNET_CONN_HTTP_TIMEOUT: the connection timed out
 *
 *  %GNET_CONN_HTTP_ERROR: #GConnHttp problem (not a server error response)
 *
 **/
typedef void   (*GConnHttpFunc) (GConnHttp *conn, GConnHttpEvent *event, gpointer user_data);

/***************************************************************************
 *                                                                         *
 *   GConnHttp API functions                                               *
 *                                                                         *
 ***************************************************************************/

GConnHttp       *gnet_conn_http_new                (void);

gboolean         gnet_conn_http_set_uri            (GConnHttp        *conn,
                                                    const gchar      *uri);

gboolean         gnet_conn_http_set_escaped_uri    (GConnHttp        *conn,
                                                    const gchar      *uri);

gboolean         gnet_conn_http_set_header         (GConnHttp           *conn,
                                                    const gchar         *field,
                                                    const gchar         *value,
                                                    GConnHttpHeaderFlags flags);

void             gnet_conn_http_set_max_redirects  (GConnHttp        *conn,
                                                    guint             num);

void             gnet_conn_http_set_timeout        (GConnHttp        *conn,
                                                    guint             timeout);

gboolean         gnet_conn_http_set_user_agent     (GConnHttp        *conn,
                                                    const gchar      *agent);

gboolean         gnet_conn_http_set_method         (GConnHttp        *conn,
                                                    GConnHttpMethod   method,
                                                    const gchar      *post_data,
                                                    gsize             post_data_len);

gboolean         gnet_conn_http_set_main_context   (GConnHttp        *conn,
                                                    GMainContext     *context);

void             gnet_conn_http_run_async          (GConnHttp        *conn,
                                                    GConnHttpFunc     func,
                                                    gpointer          user_data);

gboolean         gnet_conn_http_run                (GConnHttp        *conn,
                                                    GConnHttpFunc     func,
                                                    gpointer          user_data);

gboolean         gnet_conn_http_steal_buffer       (GConnHttp        *conn,
                                                    gchar           **buffer,
                                                    gsize            *length);

void             gnet_conn_http_cancel             (GConnHttp        *conn);

void             gnet_conn_http_delete             (GConnHttp        *conn);

gboolean         gnet_http_get                     (const gchar      *url, 
                                                    gchar           **buffer, 
                                                    gsize            *length, 
                                                    guint            *response);

G_END_DECLS

#endif /* _GNET_CONN_HTTP_H */
