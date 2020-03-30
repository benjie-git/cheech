/* GNet - Networking library
 * Copyright (C) 2000  David Helder
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


#ifndef _GNET_CONN_H
#define _GNET_CONN_H

#include <glib.h>
#include "tcp.h"
#include "iochannel.h"

G_BEGIN_DECLS

/**
 *   GConnEventType
 *   @GNET_CONN_ERROR: Connection error
 *   @GNET_CONN_CONNECT: Connection complete
 *   @GNET_CONN_CLOSE: Connection closed
 *   @GNET_CONN_TIMEOUT: Timeout
 *   @GNET_CONN_READ: Read complete
 *   @GNET_CONN_WRITE: Write complete
 *   @GNET_CONN_READABLE: Connection is readable
 *   @GNET_CONN_WRITABLE: Connection is writable
 *
 *   Event type.  Used by #GConnEvent.
 *
 **/
typedef enum {
  GNET_CONN_ERROR,
  GNET_CONN_CONNECT,
  GNET_CONN_CLOSE,
  GNET_CONN_TIMEOUT,
  GNET_CONN_READ,
  GNET_CONN_WRITE,
  GNET_CONN_READABLE,
  GNET_CONN_WRITABLE
} GConnEventType;


/**
 *  GConnEvent
 *  @type: event type
 *  @buffer: buffer
 *  @length: buffer length
 *
 *  GConn Event.  @buffer and @length are set only on #GNET_CONN_READ
 *  events.  The buffer is caller-owned.
 *
 **/
typedef struct _GConnEvent GConnEvent;
struct _GConnEvent
{
  GConnEventType type;
  gchar*	 buffer;
  gint		 length;
};



/**
 *  GConn
 *  @hostname: host name
 *  @port: port
 *  @iochannel: IO channel
 *  @socket: socket
 *  @inetaddr: address
 *  @ref_count: [private]
 *  @ref_count_internal: [private]
 *  @connect_id: [private]
 *  @new_id: [private]
 *  @write_queue: [private]
 *  @bytes_written: [private]
 *  @buffer: [private]
 *  @length: [private]
 *  @bytes_read: [private]
 *  @read_eof: [private]
 *  @read_queue: [private]
 *  @process_buffer_timeout: [private]
 *  @watch_readable: [private]
 *  @watch_writable: [private]
 *  @watch_flags: [private]
 *  @watch: [private]
 *  @timer: [private]
 *  @func: [private]
 *  @user_data: [private]
 *
 *  TCP Connection.  Some of the fields are public, but do not set
 *  these fields.
 *
 **/
typedef struct _GConn GConn;


/**
 *  GConnFunc
 *  @conn: #GConn
 *  @event: event (caller owned)
 *  @user_data: user data specified in gnet_conn_new()
 * 
 *  Callback for #GConn.  
 *
 *  Possible events:
 *
 *  %GNET_CONN_ERROR: #GConn error.  The event occurs if the connection
 *  fails somehow.  The connection is closed before this event occurs.
 *
 *  %GNET_CONN_CONNECT: Completion of gnet_conn_connect().
 *
 *  %GNET_CONN_CLOSE: Connection has been closed.  The event does not
 *  occur as a result of calling gnet_conn_disconnect(),
 *  gnet_conn_unref(), or gnet_conn_delete().
 *
 *  %GNET_CONN_TIMEOUT: Timer set by gnet_conn_timeout() expires.
 *
 *  %GNET_CONN_READ: Data has been read.  This event occurs as a result
 *  of calling gnet_conn_read(), gnet_conn_readn(), or
 *  gnet_conn_readline().  buffer and length are set in the event
 *  object.  The buffer is caller owned.
 *
 *  %GNET_CONN_WRITE: Data has been written.  This event occurs as a
 *  result of calling gnet_conn_write() or gnet_conn_write_direct().
 *
 *  %GNET_CONN_READABLE: The connection is readable.
 *
 *  %GNET_CONN_WRITABLE: The connection is writable.
 *
 **/
typedef void (*GConnFunc)(GConn* conn, GConnEvent* event, gpointer user_data);


struct _GConn
{
  /* Public */
  gchar*			hostname;
  gint				port;

  GIOChannel* 			iochannel;
  GTcpSocket* 			socket;
  GInetAddr*			inetaddr;


  /* Private */
  guint				ref_count;
  guint				ref_count_internal;

  /* Connect */
  GTcpSocketConnectAsyncID 	connect_id;
  GTcpSocketNewAsyncID 		new_id;

  /* Write */
  GList*			write_queue;
  guint				bytes_written;

  /* Read */
  gchar* 			buffer;
  guint 			length;
  guint 			bytes_read;
  gboolean			read_eof;
  GList*			read_queue;
  guint				process_buffer_timeout;

  /* Readable/writable */
  gboolean			watch_readable;
  gboolean			watch_writable;

  /* IO watch */
  guint				watch_flags;
  guint				watch;

  /* Timer */
  guint				timer;

  /* User data */
  GConnFunc			func;
  gpointer			user_data;

  GMainContext                * context;
  gint                          priority;
};



/* ********** */

GConn*     gnet_conn_new (const gchar* hostname, gint port, 
			  GConnFunc func, gpointer user_data);
GConn*     gnet_conn_new_inetaddr (const GInetAddr* inetaddr, 
				   GConnFunc func, gpointer user_data);
GConn*	   gnet_conn_new_socket (GTcpSocket* socket,
				 GConnFunc func, gpointer user_data);

void 	   gnet_conn_delete (GConn* conn);
	      
void	   gnet_conn_ref (GConn* conn);
void	   gnet_conn_unref (GConn* conn);
	      
void	   gnet_conn_set_callback (GConn* conn, 
				   GConnFunc func, gpointer user_data);

gboolean   gnet_conn_set_main_context (GConn * conn, GMainContext * context);

/* ********** */

void	   gnet_conn_connect (GConn* conn);
void	   gnet_conn_disconnect (GConn* conn);
	      
gboolean   gnet_conn_is_connected (const GConn* conn);
	      

/* ********** */

void	   gnet_conn_read (GConn* conn);
void	   gnet_conn_readn (GConn* conn, gint length);
void	   gnet_conn_readline (GConn* conn);

void	   gnet_conn_write (GConn* conn, gchar* buffer, gint length);
void	   gnet_conn_write_direct (GConn* conn, gchar* buffer, gint length,
				   GDestroyNotify buffer_destroy_cb);

void	   gnet_conn_set_watch_readable (GConn* conn, gboolean enable);
void	   gnet_conn_set_watch_writable (GConn* conn, gboolean enable);
void	   gnet_conn_set_watch_error    (GConn* conn, gboolean enable);

void	   gnet_conn_timeout (GConn* conn, guint timeout);

G_END_DECLS

#endif /* _GNET_CONN_H */
