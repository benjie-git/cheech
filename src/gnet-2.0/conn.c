/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2003-2004  Andrew Lanoix
 * Copyright (C) 2007  Tim-Philipp Müller <tim centricular net>
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


#include "conn.h"

#include <stdio.h>
#include <memory.h> /* required for windows */
#include <string.h> /* needed for g_memmove/memmove */

#include "gnet-private.h"

#define IS_CONNECTED(C)  ((C)->socket != NULL)
#define BUFFER_LEN	 1024

/* FIXME: these macros are horrid, get rid of them */
#define IS_WATCHING(C, FLAG) (((C)->watch_flags & (FLAG))?TRUE:FALSE)

#define ADD_WATCH(C, FLAG)	do {			\
  if (!IS_WATCHING(C,FLAG)) 	{			\
    (C)->watch_flags |= (FLAG);				\
    if ((C)->iochannel) {				\
      if ((C)->watch)					\
        _gnet_source_remove ((C)->context, (C)->watch);	\
      (C)->watch = _gnet_io_watch_add_full ((C)->context,\
          G_PRIORITY_DEFAULT, (C)->iochannel,		\
          (C)->watch_flags, async_cb, (C), NULL);	\
 }}} while (0)

#define REMOVE_WATCH(C, FLAG)	do {			\
  if (IS_WATCHING(C,FLAG)) 	{			\
    (C)->watch_flags &= ~(FLAG);			\
    if ((C)->iochannel) {				\
      if ((C)->watch)					\
        _gnet_source_remove ((C)->context, (C)->watch);	\
        (C)->watch = 0;					\
      if ((C)->watch_flags != 0)			\
        (C)->watch = _gnet_io_watch_add_full ((C)->context, \
            G_PRIORITY_DEFAULT, (C)->iochannel,		\
            (C)->watch_flags, async_cb, (C), NULL);	\
 }}} while (0)

#define UNSET_WATCH (C, FLAG)


typedef struct _Write
{
  gchar* 	buffer;
  gint 		length;
  GDestroyNotify buffer_destroy_cb;
} Write;


typedef struct _Read
{
  gint mode;

} Read;



static void 	ref_internal (GConn* conn);
static void 	unref_internal (GConn* conn);

static void 	conn_new_cb (GTcpSocket* socket, gpointer user_data);

static void 	conn_connect_cb (GTcpSocket* socket,
				 GTcpSocketConnectAsyncStatus status, 
				 gpointer user_data);



static gboolean async_cb (GIOChannel* iochannel, GIOCondition condition, 
			  gpointer data);

static void	conn_read_full (GConn* conn, gint mode);
static void	conn_check_read_queue (GConn* conn);
static void     conn_read_async_cb (GConn* conn);
static gboolean process_read_buffer_cb (gpointer data);
static gint	bytes_processable (GConn* conn);
static gint	process_read_buffer (GConn* conn);


static void 	conn_write_async_cb (GConn* conn);
static void 	conn_check_write_queue (GConn* conn);

static gboolean conn_timeout_cb (gpointer data);




/**
 *  gnet_conn_new
 *  @hostname: name of host to connect to
 *  @port: port to connect to
 *  @func: function to call on #GConn events
 *  @user_data: data to pass to @func on callbacks
 *
 *  Creates a #GConn.  A connection is not made until
 *  gnet_conn_connect() is called.  The callback @func is called when
 *  events occur.
 *
 *  Returns: a #GConn.
 *
 **/
GConn*
gnet_conn_new (const gchar* hostname, gint port, 
	       GConnFunc func, gpointer user_data)
{
  GConn* conn;

  g_return_val_if_fail (hostname, NULL);

  conn = g_new0 (GConn, 1);
  conn->ref_count = 1;
  conn->hostname = g_strdup(hostname);
  conn->port = port;
  conn->inetaddr = gnet_inetaddr_new_nonblock (hostname, port);
  conn->func = func;
  conn->user_data = user_data;

  return conn;
}



/**
 *  gnet_conn_new_inetaddr
 *  @inetaddr: address of host to connect to
 *  @func: function to call on #GConn events
 *  @user_data: data to pass to @func on callbacks
 *
 *  Creates a #GConn.  A connection is not made until
 *  gnet_conn_connect() is called.  The callback @func is called when
 *  events occur.
 *
 *  Returns: a #GConn.
 *
 **/
GConn*   
gnet_conn_new_inetaddr (const GInetAddr* inetaddr, 
			GConnFunc func, gpointer user_data)
{
  GConn* conn;

  g_return_val_if_fail (inetaddr, NULL);

  conn = g_new0 (GConn, 1);
  conn->ref_count = 1;
  conn->hostname = gnet_inetaddr_get_canonical_name (inetaddr);
  conn->port = gnet_inetaddr_get_port (inetaddr);
  conn->inetaddr = gnet_inetaddr_clone (inetaddr);
  conn->func = func;
  conn->user_data = user_data;

  return conn;
}



/**
 *  gnet_conn_new_socket
 *  @socket: TCP Socket (callee owned)
 *  @func: function to call on #GConn events
 *  @user_data: data to pass to @func on callbacks
 *
 *  Creates a #GConn.  The #GConn is created from the @socket.  The
 *  socket is callee owned - do not delete it (meaning: #GConn will take
 *  ownership of the socket).  The callback is called when events occur.
 *
 *  Returns: a #GConn.
 *
 **/
GConn*
gnet_conn_new_socket (GTcpSocket* socket, 
		      GConnFunc func, gpointer user_data)
{
  GConn* conn;

  g_return_val_if_fail (socket, NULL);

  conn = g_new0 (GConn, 1);
  conn->ref_count = 1;
  conn->socket = socket;
  conn->iochannel = gnet_tcp_socket_get_io_channel (socket);
  conn->inetaddr = gnet_tcp_socket_get_remote_inetaddr (socket);
  conn->hostname = gnet_inetaddr_get_canonical_name (conn->inetaddr);
  conn->port = gnet_inetaddr_get_port (conn->inetaddr);
  conn->func = func;
  conn->user_data = user_data;

  return conn;

}

/**
 *  gnet_conn_set_main_context
 *  @conn: a #GConn
 *  @context: a #GMainContext, or NULL to use the default GLib main context
 *
 *  Sets the GLib #GMainContext to use for asynchronous operations. You should
 *  call this function right after you create @conn. You must not call this
 *  function after the actual connection process has started or watches have
 *  been set up (e.g. for reading, writing or errors).
 *
 *  You are very unlikely to ever need this function.
 *
 *  Returns: TRUE on success, FALSE on failure.
 *
 *  Since: 2.0.8
 **/
gboolean
gnet_conn_set_main_context (GConn * conn, GMainContext * context)
{
  g_return_val_if_fail (conn != NULL, FALSE);

  /* Must not be called if already connected or connection pending; should
   * be okay if the socket is connected but no watches set up yet */
  g_return_val_if_fail (conn->connect_id == 0 && conn->new_id == 0, FALSE);
  g_return_val_if_fail (conn->watch == 0, FALSE);

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

/**
 *  gnet_conn_delete
 *  @conn: a #GConn
 *
 *  Deletes a #GConn.
 *
 *  Deprecated: Use g_conn_unref(), which does the same.
 **/
void
gnet_conn_delete (GConn* conn)
{
  if (conn != NULL)
    gnet_conn_unref(conn);
}


/**
 *  gnet_conn_ref
 *  @conn: a #GConn
 *
 *  Adds a reference to a #GConn.
 *
 **/
void
gnet_conn_ref (GConn* conn)
{
  g_return_if_fail (conn);

  conn->ref_count++;
}


/**
 *  gnet_conn_unref
 *  @conn: a #GConn
 *
 *  Removes a reference from a #GConn.  A #GConn is deleted when the
 *  reference count reaches 0.
 *
 **/
void
gnet_conn_unref (GConn* conn)
{
  g_return_if_fail (conn);

  conn->ref_count--;
  if (conn->ref_count > 0 || conn->ref_count_internal > 0)
    return;

  gnet_conn_disconnect (conn);

  g_free (conn->hostname);

  if (conn->inetaddr)
    gnet_inetaddr_delete (conn->inetaddr);

  g_free (conn->buffer);

  g_free (conn);
}


/* increment internal ref count.  This is used to prevent deletion
   during a callback.  We don't want the conn to be deleted during a
   callback if we must still access the conn after the callback.  */
static void 
ref_internal (GConn* conn) 
{
  g_return_if_fail (conn);

  conn->ref_count_internal++;
}


static void
unref_internal (GConn* conn)
{
  g_return_if_fail (conn);

  conn->ref_count_internal--;
  if (conn->ref_count == 0 && conn->ref_count_internal == 0)
    {
      /* set ref_count to 1 to force deletion on unref. */
      conn->ref_count = 1;
      gnet_conn_unref (conn);
    }
}



/**
 *  gnet_conn_set_callback
 *  @conn: a #GConn
 *  @func: function to call on #GConn events
 *  @user_data: data to pass to @func on callbacks
 *
 *  Sets the #GConnEvent callback for a #GConn.  The callback @func is
 *  called when events occur.
 *
 **/
void	
gnet_conn_set_callback (GConn* conn, GConnFunc func, gpointer user_data)
{
  g_return_if_fail (conn);
  conn->func = func;
  conn->user_data = user_data;
}



/**
 *  gnet_conn_connect
 *  @conn: a #GConn
 *
 *  Establishes a connection.  If the connection is pending or already
 *  established, this function does nothing.  The callback is called
 *  when the connection is established or an error occurs.
 *
 **/
void
gnet_conn_connect (GConn * conn)
{
  g_return_if_fail (conn != NULL);
  g_return_if_fail (conn->func != NULL);

  /* Ignore if connected or connection pending */
  if (conn->connect_id != 0 || conn->new_id != 0 || conn->socket != NULL)
    return;

  /* Make asynchronous connection */
  if (conn->inetaddr) {
    conn->new_id = gnet_tcp_socket_new_async_full (conn->inetaddr,
        conn_new_cb, conn, (GDestroyNotify) NULL, conn->context,
        G_PRIORITY_DEFAULT);
  } else if (conn->hostname) {
    conn->connect_id = gnet_tcp_socket_connect_async_full (conn->hostname,
        conn->port, conn_connect_cb, conn, (GDestroyNotify) NULL,
        conn->context, G_PRIORITY_DEFAULT);
  } else {
    g_return_if_reached ();
  }
}


static void
conn_new_cb (GTcpSocket* socket, gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  GConnEvent event;

  g_return_if_fail (conn);

  conn->new_id = NULL;

  if (socket)
    {
      conn->socket = socket;
      conn->iochannel = gnet_tcp_socket_get_io_channel (socket);

      conn_check_write_queue (conn);
      conn_check_read_queue (conn);
      if (conn->watch_flags) ADD_WATCH(conn, 0);

      event.type = GNET_CONN_CONNECT;
    }
  else
    {
      event.type = GNET_CONN_ERROR;
    }

  event.buffer = NULL;
  event.length = 0;
  (conn->func)(conn, &event, conn->user_data);
}


static void
conn_connect_cb (GTcpSocket* socket, 
		 GTcpSocketConnectAsyncStatus status, 
		 gpointer user_data)
{
  GConn* conn = (GConn*) user_data;
  GConnEvent event;

  g_return_if_fail (conn);

  conn->connect_id = NULL;

  if (status == GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK)
    {
      conn->socket = socket;
      conn->inetaddr = gnet_tcp_socket_get_remote_inetaddr (socket);
      conn->iochannel = gnet_tcp_socket_get_io_channel (socket);

      conn_check_write_queue (conn);
      conn_check_read_queue (conn);
      if (conn->watch_flags) ADD_WATCH(conn, 0);

      event.type = GNET_CONN_CONNECT;
    }
  else
    {
      event.type = GNET_CONN_ERROR;
    }

  event.buffer = NULL;
  event.length = 0;
  (conn->func)(conn, &event, conn->user_data);
}


static void
conn_write_free(Write *write)
{
  if (write->buffer_destroy_cb)
    write->buffer_destroy_cb(write->buffer);
  g_free (write);
}


/**
 *  gnet_conn_disconnect
 *  @conn: a #GConn
 *
 *  Closes the connection.  The connection can later be reestablished
 *  by calling gnet_conn_connect() again.  If the connection was not
 *  established, this function does nothing.
 *
 **/
void
gnet_conn_disconnect (GConn* conn)
{
  GList* i;

  g_return_if_fail (conn);

  if (conn->watch)
    {
      _gnet_source_remove (conn->context, conn->watch);
      conn->watch = 0;
    }
  conn->watch_flags = 0;

  conn->watch_readable = FALSE;
  conn->watch_writable = FALSE;

  if (conn->iochannel)
    conn->iochannel = NULL;	/* do not unref */

  if (conn->socket)
    {
      gnet_tcp_socket_delete (conn->socket);
      conn->socket = NULL;
    }

  if (conn->connect_id)
    {
      gnet_tcp_socket_connect_async_cancel (conn->connect_id);
      conn->connect_id = NULL;
    }

  if (conn->new_id)
    {
      gnet_tcp_socket_new_async_cancel (conn->new_id);
      conn->new_id = NULL;
    }

  for (i = conn->write_queue; i != NULL; i = i->next)
    {
      conn_write_free(i->data);
    }
  g_list_free (conn->write_queue);
  conn->write_queue = NULL;
  conn->bytes_written = 0;

  for (i = conn->read_queue; i != NULL; i = i->next)
    {
      Read* read = i->data;
      g_free (read);
    }
  g_list_free (conn->read_queue);
  conn->read_queue = NULL;
  conn->bytes_read = 0;
  conn->read_eof = FALSE;
  if (conn->process_buffer_timeout)
    {
      _gnet_source_remove (conn->context, conn->process_buffer_timeout);
      conn->process_buffer_timeout = 0;
    }

  if (conn->timer)
    {
      _gnet_source_remove (conn->context, conn->timer);
      conn->timer = 0;
    }
}


/**
 *  gnet_conn_is_connected
 *  @conn: a #GConn
 *
 *  Checks if the connection is established.
 *
 *  Returns: TRUE if the connection is established, FALSE otherwise.
 *
 **/
gboolean
gnet_conn_is_connected (const GConn* conn)
{
  g_return_val_if_fail (conn, FALSE);

  if (IS_CONNECTED(conn))
    return TRUE;
  
  return FALSE;
}


/* **************************************** */

static gboolean
async_cb (GIOChannel* iochannel, GIOCondition condition, gpointer data)
{
  GConn* conn = (GConn*) data;
  GConnEvent event = {GNET_CONN_ERROR, NULL, 0};
  
  ref_internal (conn);

  if (condition & (G_IO_ERR | G_IO_NVAL))
    {
      /* Upcall ERROR */
      gnet_conn_disconnect (conn);
      if (conn->func)
	(conn->func) (conn, &event, conn->user_data);

      unref_internal (conn);
      return FALSE;
    }

  if (condition & G_IO_IN)
    {
      /* Upcall */
      if (conn->watch_readable)		/* READABLE */
	{
	  GConnEvent event;
	  
	  event.type = GNET_CONN_READABLE;
	  event.buffer = NULL;
	  event.length = 0;

	  g_return_val_if_fail (conn->func, FALSE);
	  (conn->func) (conn, &event, conn->user_data);
	}
      else
	{
	  conn_read_async_cb (conn);	/* READ? */
	}

      if (conn->ref_count == 0 || !IS_CONNECTED(conn))
	{
	  unref_internal (conn);
	  return FALSE;
	}
    }

  if (condition & G_IO_OUT)
    {
      /* Upcall */
      if (conn->watch_writable)		/* WRITABLE */
	{
	  GConnEvent event;
	  
	  event.type = GNET_CONN_WRITABLE;
	  event.buffer = NULL;
	  event.length = 0;

	  g_return_val_if_fail (conn->func, FALSE);
	  (conn->func) (conn, &event, conn->user_data);
	}
      else				/* WRITE? */
	{
	  conn_write_async_cb (conn);
	}

      if (conn->ref_count == 0 || !IS_CONNECTED(conn))
	{
	  unref_internal (conn);
	  return FALSE;
	}
    }

  if (condition & G_IO_HUP)
    {
      gnet_conn_disconnect (conn);
      event.type = GNET_CONN_CLOSE;
      if (conn->func)
	(conn->func) (conn, &event, conn->user_data);
    }

  unref_internal (conn);
  /* Assume we want another callback, even though the source may have
     been destroyed though.  The Glib main loop handles this
     properly. */

  return TRUE; 
}


/* **************************************** */


/**
 *  gnet_conn_read:
 *  @conn: a #GConn
 *
 *  Begins an asynchronous read.  The connection callback is called
 *  when any data has been read.  This function may be called again
 *  before the asynchronous read completes.
 *
 **/
void
gnet_conn_read (GConn* conn)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  conn_read_full (conn, 0);
}


/**
 *  gnet_conn_readn:
 *  @conn: a #GConn
 *  @length: Number of bytes to read
 *
 *  Begins an asynchronous read of exactly @length bytes.  The
 *  connection callback is called when the data has been read.  This
 *  function may be called again before the asynchronous read
 *  completes.
 *
 **/
void
gnet_conn_readn (GConn* conn, gint n)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);
  g_return_if_fail (n > 0);

  conn_read_full (conn, n);

}



/**
 *  gnet_conn_readline:
 *  @conn: a #GConn
 *
 *  Begins an asynchronous line read.  The connection callback is
 *  called when a line has been read.  Lines are terminated with \n,
 *  \r, \r\n, or \0.  The terminator is \0'ed out in the buffer.  The
 *  terminating \0 is accounted for in the buffer length.  This
 *  function may be called again before the asynchronous read
 *  completes.
 *
 **/
void
gnet_conn_readline (GConn* conn)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  conn_read_full (conn, -1);
}



static void
conn_read_full (GConn* conn, gint mode)
{
  Read* read;

  g_return_if_fail (conn);

  /* Create the buffer */
  if (!conn->buffer)
    {
      conn->buffer = g_malloc (BUFFER_LEN);
      conn->length = BUFFER_LEN;
      conn->bytes_read = 0;
    }

  /* Add to read queue */
  read = g_new0 (Read, 1);
  read->mode = mode;
  conn->read_queue = g_list_append (conn->read_queue, read);

  /* Check the read queue */
  conn_check_read_queue (conn);
}


static void
conn_check_read_queue (GConn* conn)
{
  /* Ignore if we are unconnected or there are no reads */
  if (!IS_CONNECTED(conn) || !conn->read_queue)
    return;

  /* Ignore if we will process the buffer or are already watch IN */
  if (conn->process_buffer_timeout || IS_WATCHING(conn, G_IO_IN))
    return;


  /* Set up process_read_buffer_cb() if there's data available and we
     can process it, OR if we've read EOF.  EOF is 

     EOF is handled when it is read, regardless of the read queue.  We
     don't worry about it here.

     OPTIMIZATION: There may be reads in the queue that cannot be
     processed given the data we have and we would need to read in
     more data.  We watch IO_IN in this case.  */
  if ((conn->bytes_read && bytes_processable(conn) > 0) || conn->read_eof) {
    conn->process_buffer_timeout = _gnet_timeout_add_full (conn->context,
        G_PRIORITY_DEFAULT, 0, process_read_buffer_cb, conn, NULL);
  } else {
  /* Otherwise, if there is no read watch, set one, so we can read
   * more bytes */
    ADD_WATCH(conn, G_IO_IN);
  }
}


static void
conn_read_async_cb (GConn* conn)
{
  guint  bytes_to_read;
  gchar* buffer_start;
  GIOError error;
  gsize  bytes_read;
  
  /* Resize the buffer if it's full. */
  if (conn->length == conn->bytes_read)
    {
      conn->length *= 2;
      conn->buffer = g_realloc(conn->buffer, conn->length);
    }

  /* Calculate buffer start and length */
  bytes_to_read = conn->length - conn->bytes_read;
  buffer_start = &conn->buffer[conn->bytes_read];
  g_return_if_fail (bytes_to_read > 0);

  /* Read data into buffer */
  error = g_io_channel_read (conn->iochannel, buffer_start,
			     bytes_to_read, &bytes_read);

  /* Try again later if necessary */
  if (error == G_IO_ERROR_AGAIN)
    return;

  /* Fail if this is an error */
  else if (error != G_IO_ERROR_NONE)
    {
      GConnEvent event = {GNET_CONN_ERROR, NULL, 0};

      ref_internal (conn);

      gnet_conn_disconnect (conn);
      (conn->func) (conn, &event, conn->user_data);

      unref_internal (conn);

      return;
    }

  /* If we read nothing, that means EOF and we're done.  We do not
     callback with anything that might be in the buffer. */
  else if (bytes_read == 0)
    {
      conn->read_eof = TRUE;
    }
  
  /* Otherwise, we read something */
  else
    {
      conn->bytes_read += bytes_read;
    }

  /*** Process what we read *** */

  /* Process the read buffer */
  ref_internal (conn);
  do
    {
      /* Process data */
      bytes_read = process_read_buffer (conn);
      /* conn may be disconnected at this point */

      /* Stop if conn deleted */
      if (conn->ref_count == 0)	
	{
	  unref_internal (conn);
	  return;
	}

    } while (bytes_read > 0);

  unref_internal (conn);	
  /* conn is still good (though possibly disconnected).  Otherwise, we
     would have returned in the do-loop. */

  /* Process EOF if:
       - we read EOF
       - the connection is still open 
       - there are reads in the read queue (we won't give the user
           a CLOSE unless they make a read that triggers it.)
  */
  if (conn->read_eof && IS_CONNECTED(conn) && conn->read_queue)	
    {
      GConnEvent event = {GNET_CONN_CLOSE, NULL, 0};

      /* (we know conn is still good - otherwise it would have been
         deleted in the do-loop) */

      gnet_conn_disconnect (conn);
      (conn->func) (conn, &event, conn->user_data);
      return;	/* we're done with conn for now */
    }

  /* Remove read watch if no more reads */
  if (!conn->read_queue)
    {
      REMOVE_WATCH(conn, G_IO_IN);
    }
}



/* Called to dispatch reads to user callback */
static gboolean
process_read_buffer_cb (gpointer data)
{
  GConn* conn = (GConn*) data;
  gint bytes_read;

  g_return_val_if_fail (conn, FALSE);

  conn->process_buffer_timeout = 0;

  /* Ignore if nothing to read */
  if (conn->bytes_read == 0 || conn->read_queue == NULL)
    return FALSE;

  /* Process reads */
  ref_internal (conn);
  do
    {
      /* Process data */
      bytes_read = process_read_buffer (conn);
      /* conn may be disconnected at this point */

      /* Stop if conn deleted */
      if (conn->ref_count == 0)	
	{
	  unref_internal (conn);
	  return FALSE;
	}
    } while (bytes_read > 0);

  unref_internal (conn);
  /* conn is still good (though possibly disconnected).  Otherwise, we
     would have returned in the do-loop. */

  /* Process EOF (if we read EOF) and are still connected */
  if (conn->read_eof && IS_CONNECTED(conn))	
    {
      GConnEvent event = {GNET_CONN_CLOSE, NULL, 0};

      gnet_conn_disconnect (conn);
      (conn->func) (conn, &event, conn->user_data);
      return FALSE;	/* we're done with conn for now*/
    }

  /* Set read watch if we're still connected and there's more to read */
  if (IS_CONNECTED(conn) && conn->read_queue)
    {
      ADD_WATCH (conn, G_IO_IN);
    }

  return FALSE;
}


/* Calculate number of bytes that can be processed by the top read */
static gint
bytes_processable (GConn* conn)
{
  Read* read;

  g_return_val_if_fail (conn, 0);

  /* If there is no data to process or reads to process, return 0 */
  if (conn->bytes_read == 0 || conn->read_queue == NULL)
    return 0;

  /* Get a read off the queue */
  read = (Read*) conn->read_queue->data;

  switch (read->mode)
    {
      /* Read any */
    case 0:		
      {
	return conn->bytes_read;
	break;
      }

      /* Read line */
    case -1:		
      {
	guint i;

	/* Look for \n, \r, or \r\n */
	for (i = 0; i < conn->bytes_read; ++i)
	  {
	    /* \n and \0  */
	    if (conn->buffer[i] == '\0' ||
		conn->buffer[i] == '\n')
	      return i+1;

	    /* \r Only counted if we know the next character.  If
	       it's a \n, it'd need to be \0'ed out. */
	    else if (conn->buffer[i] == '\r' &&
		     ((i+1) < conn->bytes_read))
	      {
		if (conn->buffer[i+1] == '\n')
		  return i+2;
		else
		  return i+1;
	      }
	  }
	break;
      }

    default:		/* Read n */
      {
	if (conn->bytes_read >= read->mode)
	  return read->mode;
	break;
      }
    }

  return 0;
}


/* Return bytes read */
static gint
process_read_buffer (GConn* conn)
{
  Read* read;
  gint bytes_processed = 0;
  gint bytes_read = 0;

  g_return_val_if_fail (conn, FALSE);

  /* If there is no data to process or reads to process, return 0 */
  if (conn->bytes_read == 0 || conn->read_queue == NULL)
    return 0;

  /* Get a read off the queue */
  read = (Read*) conn->read_queue->data;

  ref_internal (conn);

  switch (read->mode)
    {
      /* Read any */
    case 0:		
      {
	bytes_processed = bytes_read = conn->bytes_read;

	break;
      }

      /* Read line */
    case -1:		
      {
	guint i;

	/* Look for \n, \r, or \r\n */
	for (i = 0; i < conn->bytes_read; ++i)
	  {
	    /* \0  */
	    if (conn->buffer[i] == '\0')
	      {
		bytes_processed = bytes_read = i + 1;
		break;
	      }

	    /* \n */
	    else if (conn->buffer[i] == '\n')
	      {
		conn->buffer[i] = '\0';
		bytes_processed = bytes_read = i + 1;
		break;
	      }

	    /* \r  Only counted if we know the next character.  If it's
	       a \n, it needs to be \0'ed out. */
	    else if (conn->buffer[i] == '\r' &&
		     ((i+1) < conn->bytes_read))
	      {
		if (conn->buffer[i+1] == '\n')
		  {
		    conn->buffer[i] = '\0';
		    conn->buffer[i+1] = '\0';
		    bytes_read = i + 1;
		    bytes_processed = i + 2;
		  }
		else
		  {
		    conn->buffer[i] = '\0';
		    bytes_processed = bytes_read = i + 1;
		  }
		break;
	      }
	  }

	break;
      }

    default:		/* Read n */
      {
	if (conn->bytes_read >= read->mode)
	  bytes_processed = bytes_read = read->mode;

	break;
      }
    }

  if (bytes_read)
    {
      GConnEvent event;

      event.type = GNET_CONN_READ;
      event.buffer = conn->buffer;
      event.length = bytes_read;

      (conn->func) (conn, &event, conn->user_data);
    }
  /* Note: User may have disconnected after the callback */

  /* If read successful and we're still connected, move bytes over and
     remove read */
  if (bytes_processed && IS_CONNECTED(conn))
    {
      g_assert (conn->bytes_read >= bytes_processed);/* Sanity check */

      /* Move bytes over */
      g_memmove (conn->buffer, &conn->buffer[bytes_processed], 
		 conn->bytes_read - bytes_processed);
      conn->bytes_read -= bytes_processed;

      /* Remove read from queue */
      conn->read_queue = g_list_remove (conn->read_queue, read);
      g_free (read);
    }

  unref_internal (conn);
      
  return bytes_processed;
}




/* **************************************** */



/**
 *  gnet_conn_write_direct
 *  @conn: a #GConn
 *  @buffer: buffer to write from
 *  @length: length of @buffer
 *  @buffer_destroy_cb: function to call when buffer is no longer needed
 *
 *  Sets up an asynchronous write to @conn from @buffer.  The buffer is
 *  created by the caller and will not be copied.  The caller needs to
 *  make sure the buffer stays valid until @buffer_destroy_cb is called.
 *  This function can be called again before the asynchronous write
 *  completes.  @buffer_destroy_cb may be %NULL.
 *
 **/
void
gnet_conn_write_direct (GConn* conn, gchar* buffer, gint length,
			GDestroyNotify buffer_destroy_cb)
{
  Write* write;

  g_return_if_fail (conn != NULL);
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (length >= 0);

  if (length == 0)
    return;

  /* Add to queue */
  write = g_new0 (Write, 1);
  write->buffer = buffer;
  write->length = length;
  write->buffer_destroy_cb = buffer_destroy_cb;
  conn->write_queue = g_list_append (conn->write_queue, write);

  conn_check_write_queue (conn);
}

/**
 *  gnet_conn_write
 *  @conn: a #GConn
 *  @buffer: buffer to write from
 *  @length: length of @buffer
 *
 *  Sets up an asynchronous write to @conn from @buffer.  The buffer is
 *  copied, so it may be deleted by the caller.  This function can be
 *  called again before the asynchronous write completes.
 *
 **/
void
gnet_conn_write (GConn* conn, gchar* buffer, gint length)
{
  gchar *copy;

  g_return_if_fail (conn != NULL);
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (length >= 0);

  if (length == 0)
    return;

  copy = g_memdup (buffer, length);
  gnet_conn_write_direct (conn, copy, length, (GDestroyNotify) g_free);
}


static void
conn_check_write_queue (GConn* conn)
{
  /* Ignore if we are unconnected or there is no writes */
  if (!IS_CONNECTED(conn) || !conn->write_queue)
    return;

  /* Ignore if we are already watching OUT */
  if (IS_WATCHING(conn, G_IO_OUT))
    return;

  /* Watch for write */
  ADD_WATCH (conn, G_IO_OUT);
}


static void
conn_write_async_cb (GConn* conn)
{
  Write*     write;
  GIOError   error;
  guint      bytes_to_write;
  gchar*     buffer_start;
  gsize      bytes_written;
  GConnEvent event = {GNET_CONN_ERROR, NULL, 0};

  write = (Write*) conn->write_queue->data;
  g_return_if_fail (write != NULL);

  /* Calculate start of buffer */
  buffer_start = &write->buffer[conn->bytes_written];
  bytes_to_write = write->length - conn->bytes_written;

  /* Write */
  error = g_io_channel_write(conn->iochannel, buffer_start,
			     bytes_to_write, &bytes_written);

  /* Check for error.  If error, disconnect and notify */
  if (error != G_IO_ERROR_NONE)
    {
      gnet_conn_disconnect (conn);
      (conn->func) (conn, &event, conn->user_data);
      /* conn may be deleted now */
      return;
    }
  /* Otherwise, write is good */

  /* Increment bytes written count */
  conn->bytes_written += bytes_written;

  /* Check if we're done writing this queued write */
  if (conn->bytes_written == write->length)
    {
      /* Remove and delete the queued write */
      conn->write_queue = g_list_remove (conn->write_queue, write);
      conn_write_free(write);

      conn->bytes_written = 0;

      /* Remove watch if there are no more queued writes */
      if (conn->write_queue == NULL)
	REMOVE_WATCH (conn, G_IO_OUT);

      /* Notify */
      event.type = GNET_CONN_WRITE;
      (conn->func)(conn, &event, conn->user_data);
      /* conn may be disconnected or deleted now */
    }

  /* Otherwise, keep watching for output */
  return;
}



/* **************************************** */

/**
 * gnet_conn_set_watch_error
 * @conn: a #GConn
 * @enable: enable the %GNET_CONN_READABLE event?
 *
 * Enables (or disables) the %GNET_CONN_ERROR event for a #GConn.  If
 * enabled, the %GNET_CONN_ERROR event occurs when an error occurs.
 * The @conn is disconnected before the callback is made.
 *
 **/
void
gnet_conn_set_watch_error (GConn* conn, gboolean enable)
{
  g_return_if_fail (conn);

  if (enable)
    ADD_WATCH(conn, G_IO_ERR | G_IO_HUP | G_IO_NVAL);
  else
    REMOVE_WATCH(conn, G_IO_ERR | G_IO_HUP | G_IO_NVAL);
}


/**
 * gnet_conn_set_watch_readable
 * @conn: a #GConn
 * @enable: enable the %GNET_CONN_READABLE event?
 *
 * Enables (or disables) the %GNET_CONN_READABLE event for a #GConn.
 * If enabled, the %GNET_CONN_READABLE event occurs when data can be
 * read from the socket.  Read from the iochannel member of the @conn.
 * Do not enable this while using gnet_conn_read(), gnet_conn_readn(),
 * or gnet_conn_readline().
 *
 **/
void
gnet_conn_set_watch_readable (GConn* conn, gboolean enable)
{
  g_return_if_fail (conn);
  g_return_if_fail (conn->func);

  conn->watch_readable = enable;
  if (enable)
    ADD_WATCH (conn, G_IO_IN);
  else
    REMOVE_WATCH (conn, G_IO_IN);
}


/**
 * gnet_conn_set_watch_writable
 * @conn: a #GConn
 * @enable: enable the #GNET_CONN_WRITABLE event?
 *
 * Enables (or disables) the %GNET_CONN_WRITABLE event for a #GConn.
 * If enabled, the #GNET_CONN_WRITABLE event occurs when data can be
 * written to the socket.  Write to the iochannel member of the @conn.
 * Do not enable this while using gnet_conn_write().
 *
 **/
void
gnet_conn_set_watch_writable (GConn* conn, gboolean enable)
{
  g_return_if_fail (conn);

  conn->watch_writable = enable;
  if (enable)
    ADD_WATCH (conn, G_IO_OUT);
  else
    REMOVE_WATCH (conn, G_IO_OUT);
}


/* **************************************** */


/**
 *  gnet_conn_timeout
 *  @conn: a #GConn
 *  @timeout: Timeout (in milliseconds)
 * 
 *  Sets a timeout on a #GConn.  When the timer expires, the
 *  %GNET_CONN_STATUS_TIMEOUT event occurs.  If there already is a
 *  timeout set on @conn, the old timeout is canceled.  Set @timeout
 *  to 0 to cancel the current timeout.
 *
 **/
void
gnet_conn_timeout (GConn* conn, guint timeout)
{
  g_return_if_fail (conn != NULL);

  if (conn->timer) {
    _gnet_source_remove (conn->context, conn->timer);
    conn->timer = 0;
  }

  if (timeout) {
    g_return_if_fail (conn->func != NULL);
    conn->timer = _gnet_timeout_add_full (conn->context, G_PRIORITY_DEFAULT,
        timeout, conn_timeout_cb, conn, (GDestroyNotify) NULL);
  }
}


static gboolean 
conn_timeout_cb (gpointer data)
{
  GConn* conn = (GConn*) data;
  GConnEvent event;

  g_return_val_if_fail (conn, FALSE);

  conn->timer = 0;
  
  event.type = GNET_CONN_TIMEOUT;
  event.buffer = NULL;
  event.length = 0;
  (conn->func)(conn, &event, conn->user_data);

  return FALSE;
}
