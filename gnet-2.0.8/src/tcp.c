/* GNet - Networking library
 * Copyright (C) 2000-2003  David Helder
 * Copyright (C) 2000-2004  Andrew Lanoix
 * Copyright (C) 2007       Tim-Philipp Müller <tim centricular net>
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

#include "gnet-private.h"
#include "socks-private.h"
#include "tcp.h"

static gboolean gnet_tcp_socket_new_async_cb (GIOChannel * iochannel,
    GIOCondition condition, gpointer data);
static void gnet_tcp_socket_connect_inetaddr_cb (GList * ia_list, gpointer data);
static void gnet_tcp_socket_connect_tcp_cb (GTcpSocket * socket, gpointer data);

typedef struct _GTcpSocketAsyncState
{
  GTcpSocket             * socket;
  GTcpSocketNewAsyncFunc   func;
  gpointer                 data;
  GDestroyNotify           notify;
  gint                     flags;
  GIOChannel             * iochannel;
  guint                    connect_watch;
#ifdef GNET_WIN32
  gint                     errorcode;
#endif
  GMainContext           * context; /* we hold a ref */
  gint                     priority;
} GTcpSocketAsyncState;

typedef struct _GTcpSocketConnectState
{
  GList * ia_list;
  GList * ia_next;

  GInetAddrNewListAsyncID  inetaddr_id;
  GTcpSocketNewAsyncID     tcp_id;

  gboolean in_callback;

  GTcpSocketConnectAsyncFunc func;
  gpointer                   data;
  GDestroyNotify             notify;

  GMainContext             * context; /* we hold a ref */
  gint                       priority;
} GTcpSocketConnectState;

/**
 *  gnet_tcp_socket_connect
 *  @hostname: host name
 *  @port: port
 *
 *  Creates a #GTcpSocket and connects to @hostname:@port.  This
 *  function blocks (while gnet_tcp_socket_connect_async() does not).
 *  To get the #GInetAddr of the #GTcpSocket, call
 *  gnet_tcp_socket_get_remote_inetaddr().
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 **/
GTcpSocket*
gnet_tcp_socket_connect (const gchar* hostname, gint port)
{
  GList* ia_list;
  GList* i;
  GTcpSocket* socket;

  ia_list = gnet_inetaddr_new_list (hostname, port);
  if (ia_list == NULL)
    return NULL;

  socket = NULL;
  for (i = ia_list; i != NULL; i = i->next)
    {
      GInetAddr* ia;

      ia = (GInetAddr*) i->data;
      socket = gnet_tcp_socket_new (ia);
      if (socket)
	break;
    }
  for (i = ia_list; i != NULL; i = i->next)
    gnet_inetaddr_delete ((GInetAddr*) i->data);
  g_list_free (ia_list);

  return socket;
}


/**
 *  gnet_tcp_socket_connect_async_full
 *  @hostname: host name
 *  @port: port
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *  @notify: function to call to free @data, or NULL
 *  @context: the #GMainContext to use for notifications, or NULL for the
 *      default GLib main context.  If in doubt, pass NULL.
 *  @priority: the priority with which to schedule notifications in the
 *      main context, e.g. #G_PRIORITY_DEFAULT or #G_PRIORITY_HIGH.
 *
 *  Asynchronously creates a #GTcpSocket and connects to
 *  @hostname:@port.  The callback is called when the connection is
 *  made or an error occurs.  The callback will not be called during
 *  the call to this function.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_connect_async_cancel() to cancel the
 *  connection.
 *
 *  Since: 2.0.8
 **/
GTcpSocketConnectAsyncID
gnet_tcp_socket_connect_async_full (const gchar * hostname, gint port, 
    GTcpSocketConnectAsyncFunc func, gpointer data, GDestroyNotify notify,
    GMainContext * context, gint priority)
{
  GTcpSocketConnectState* state;

  g_return_val_if_fail (hostname != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  if (context == NULL)
    context = g_main_context_default ();

  state = g_new0 (GTcpSocketConnectState, 1);

  state->func = func;
  state->data = data;
  state->notify = notify;
  state->context = g_main_context_ref (context);
  state->priority = priority;

  state->inetaddr_id = gnet_inetaddr_new_list_async_full (hostname, port,
      gnet_tcp_socket_connect_inetaddr_cb, state, (GDestroyNotify) NULL,
      state->context, priority);

  /* On failure, gnet_inetaddr_new_list_async_full() returns NULL.  It will
   * not call the callback before it returns. */
  if (state->inetaddr_id == NULL) {
    if (state->notify)
      state->notify (state->data);
    g_main_context_unref (state->context);
    g_free (state);
    return NULL;
  }

  return state;
}

/**
 *  gnet_tcp_socket_connect_async
 *  @hostname: host name
 *  @port: port
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *
 *  Asynchronously creates a #GTcpSocket and connects to
 *  @hostname:@port.  The callback is called when the connection is
 *  made or an error occurs.  The callback will not be called during
 *  the call to this function.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_connect_async_cancel() to cancel the
 *  connection.
 *
 **/
GTcpSocketConnectAsyncID
gnet_tcp_socket_connect_async (const gchar * hostname, gint port, 
    GTcpSocketConnectAsyncFunc func, gpointer data)
{
  GTcpSocketConnectAsyncID async_id;

  g_return_val_if_fail (hostname != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  async_id = gnet_tcp_socket_connect_async_full (hostname, port, func, data,
      (GDestroyNotify) NULL, NULL, G_PRIORITY_DEFAULT);

  return async_id;
}

static void
gnet_tcp_socket_connect_inetaddr_cb (GList* ia_list, gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  if (ia_list != NULL) /* Success */
    {
      state->inetaddr_id = NULL;
      state->ia_list = ia_list;
      state->ia_next = ia_list;

      while (state->ia_next != NULL)
	{
	  GTcpSocketNewAsyncID tcp_id;
	  GInetAddr* ia;

	  ia = (GInetAddr*) state->ia_next->data;
	  state->ia_next = state->ia_next->next;

	  tcp_id = gnet_tcp_socket_new_async_full (ia,
              gnet_tcp_socket_connect_tcp_cb, state, (GDestroyNotify) NULL,
              state->context, state->priority);

	  if (tcp_id)	/* Success */
	    {
	      state->tcp_id = tcp_id;
	      return;
	    }
	}

      /* Failure: We could not async connect to any address.
         In practice, new_async() rarely fails immediately.  */
      state->in_callback = TRUE;
      (*state->func)(NULL, GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR, 
		     state->data);
      state->in_callback = FALSE;

      /* FIXME: don't abuse _cancel() as free function for the state */
      gnet_tcp_socket_connect_async_cancel (state);
    }
  else /* Failure */
    {
      state->in_callback = TRUE;
      (*state->func)(NULL, GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR, 
		     state->data);
      state->in_callback = FALSE;

      /* FIXME: don't abuse _cancel() as free function for the state */
      gnet_tcp_socket_connect_async_cancel (state);
    }
}


static void 
gnet_tcp_socket_connect_tcp_cb (GTcpSocket* socket, gpointer data)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) data;

  g_return_if_fail (state != NULL);

  state->tcp_id = NULL;

  /* Success */
  if (socket != NULL)
    {
      state->in_callback = TRUE;
      (*state->func)(socket, GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK, state->data);
      state->in_callback = FALSE;

      /* FIXME: don't abuse _cancel() as free function for the state */
      gnet_tcp_socket_connect_async_cancel (state);

      return;
    }

  /* Failure: Could not connect to address */

  /* Try other addresses */
  while (state->ia_next != NULL)
    {
      GInetAddr* ia;
      gpointer tcp_id = NULL;

      ia = (GInetAddr*) state->ia_next->data;
      state->ia_next = state->ia_next->next;

      tcp_id = gnet_tcp_socket_new_async_full (ia,
          gnet_tcp_socket_connect_tcp_cb, state, (GDestroyNotify) NULL,
          state->context, state->priority);

      if (tcp_id)	/* Success */
	{
	  state->tcp_id = tcp_id;
	  return;
	}
    }

  /* Failure: No more addresses */
  state->in_callback = TRUE;
  (*state->func)(NULL, GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR, state->data);
  state->in_callback = FALSE;

  /* FIXME: don't abuse _cancel() as free function for the state */
  gnet_tcp_socket_connect_async_cancel (state);
}


/**
 *  gnet_tcp_socket_connect_async_cancel
 *  @id: ID of the connection
 *
 *  Cancels an asynchronous connection that was started with
 *  gnet_tcp_socket_connect_async().
 * 
 */
void
gnet_tcp_socket_connect_async_cancel (GTcpSocketConnectAsyncID id)
{
  GTcpSocketConnectState* state = (GTcpSocketConnectState*) id;

  g_return_if_fail (state != NULL);

  /* Ignore if user called in the middle of a callback */
  if (state->in_callback)
    return;

  if (state->ia_list)
    {
      GList* i;
      
      for (i = state->ia_list; i != NULL; i = i->next)
	gnet_inetaddr_delete ((GInetAddr*) i->data);
      g_list_free (state->ia_list);
    }

  if (state->inetaddr_id)
      gnet_inetaddr_new_list_async_cancel (state->inetaddr_id);

  if (state->tcp_id)
      gnet_tcp_socket_new_async_cancel (state->tcp_id);

  if (state->notify)
    state->notify (state->data);

  g_main_context_unref (state->context);

  g_free (state);
}


/* **************************************** */


/**
 *  gnet_tcp_socket_new
 *  @addr: address
 *
 *  Creates a #GTcpSocket and connects to @addr.  This function
 *  blocks.  SOCKS is used if SOCKS is enabled.
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_new (const GInetAddr* addr)
{
  g_return_val_if_fail (addr != NULL, NULL);

  /* Use SOCKS if enabled */
  if (gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_new (addr);

  /* Otherwise, connect directly to the address */
  return gnet_tcp_socket_new_direct (addr);
}



/**
 *  gnet_tcp_socket_new_direct:
 *  @addr: address
 *
 *  Creates a #GTcpSocket and connects to @addr without using SOCKS.
 *  This function blocks.  Most users should use
 *  gnet_tcp_socket_new().
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_new_direct (const GInetAddr* addr)
{
  SOCKET		sockfd;
  GTcpSocket* 	s;
  int			rv;

  g_return_val_if_fail (addr != NULL, NULL);

  /* Create socket */
  sockfd = socket (GNET_INETADDR_FAMILY(addr), SOCK_STREAM, 0);
  if (!GNET_IS_SOCKET_VALID(sockfd))
    {
      g_warning ("socket() failed");
      return NULL;
    }

  /* Create GTcpSocket */
  s = g_new0 (GTcpSocket, 1);
  s->sockfd = sockfd;
  s->ref_count = 1;
  s->sa = addr->sa;

  /* Connect */
  rv = connect(sockfd, 
	       &GNET_SOCKADDR_SA(s->sa), GNET_SOCKADDR_LEN(s->sa));
  if (rv != 0)
    {
      GNET_CLOSE_SOCKET(s->sockfd);
      g_free (s);
      return NULL;
    }

  return s;
}


/* **************************************** */

/**
 *  gnet_tcp_socket_new_async_full:
 *  @addr: address
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *  @notify: function to call to free @data, or NULL
 *  @context: the #GMainContext to use for notifications, or NULL for the
 *      default GLib main context.  If in doubt, pass NULL.
 *  @priority: the priority with which to schedule notifications in the
 *      main context, e.g. #G_PRIORITY_DEFAULT or #G_PRIORITY_HIGH.
 *
 *  Asynchronously creates a #GTcpSocket and connects to @addr.  The
 *  callback is called once the connection is made or an error occurs.
 *  The callback will not be called during the call to this function.
 *
 *  SOCKS is used if SOCKS is enabled.  The SOCKS negotiation will
 *  block.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_new_async_cancel() to cancel the
 *  connection.
 *
 *  Since: 2.0.8
 **/
GTcpSocketNewAsyncID
gnet_tcp_socket_new_async_full (const GInetAddr * addr,
    GTcpSocketNewAsyncFunc func, gpointer data, GDestroyNotify notify,
    GMainContext * context, gint priority)
{
  GTcpSocketNewAsyncID async_id;

  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  /* Use SOCKS if enabled, otherwise, connect directly to the address */
  if (gnet_socks_get_enabled()) {
    async_id = _gnet_socks_tcp_socket_new_async_full (addr, func, data,
        notify, context, priority);
  } else {
    async_id = gnet_tcp_socket_new_async_direct_full (addr, func, data,
        notify, context, priority);
  }

  return async_id;
}

/**
 *  gnet_tcp_socket_new_async:
 *  @addr: address
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *
 *  Asynchronously creates a #GTcpSocket and connects to @addr.  The
 *  callback is called once the connection is made or an error occurs.
 *  The callback will not be called during the call to this function.
 *
 *  SOCKS is used if SOCKS is enabled.  The SOCKS negotiation will
 *  block.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_new_async_cancel() to cancel the
 *  connection.
 *
 **/
GTcpSocketNewAsyncID
gnet_tcp_socket_new_async (const GInetAddr * addr, GTcpSocketNewAsyncFunc func,
    gpointer data)
{
  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  return gnet_tcp_socket_new_async_full (addr, func, data,
      (GDestroyNotify) NULL, NULL, G_PRIORITY_DEFAULT);
}


/**
 *  gnet_tcp_socket_new_async_direct_full:
 *  @addr: address
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *  @notify: function to call to free @data, or NULL
 *  @context: the #GMainContext to use for notifications, or NULL for the
 *      default GLib main context.  If in doubt, pass NULL.
 *  @priority: the priority with which to schedule notifications in the
 *      main context, e.g. #G_PRIORITY_DEFAULT or #G_PRIORITY_HIGH.
 *
 *  Asynchronously creates a #GTcpSocket and connects to @addr without
 *  using SOCKS.  Most users should use gnet_tcp_socket_new_async()
 *  instead.  The callback is called once the connection is made or an
 *  error occurs.  The callback will not be called during the call to
 *  this function.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_new_async_cancel() to cancel the
 *  connection.
 *
 *  Since: 2.0.8
 **/
GTcpSocketNewAsyncID
gnet_tcp_socket_new_async_direct_full (const GInetAddr * addr, 
    GTcpSocketNewAsyncFunc func, gpointer data, GDestroyNotify notify,
    GMainContext * context, gint priority)
{
  SOCKET		sockfd;
#ifndef GNET_WIN32
  gint 			flags;
#else
  u_long		arg;
#endif
  GTcpSocket* 		s;
  GTcpSocketAsyncState* state;
  gint			status;

  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  if (context == NULL)
    context = g_main_context_default ();

  /* Create socket */
  sockfd = socket(GNET_INETADDR_FAMILY(addr), SOCK_STREAM, 0);
  if (!GNET_IS_SOCKET_VALID(sockfd))
    {
      g_warning ("socket() failed");
      return NULL;
    }

  /* Force the socket into non-blocking mode */
#ifndef GNET_WIN32
  /* Get the flags (should all be 0?) */
  flags = fcntl(sockfd, F_GETFL, 0);
  if (flags == -1) 
    {
      g_warning ("fcntl() failed");
      GNET_CLOSE_SOCKET(sockfd);    
      return NULL;
    }

  if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      g_warning ("fcntl() failed");
      GNET_CLOSE_SOCKET(sockfd);    
      return NULL;
    }
#else
  arg = 1;
  ioctlsocket(sockfd, FIONBIO, &arg);
#endif

  /* Create our structure */
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;

  /* Connect (but non-blocking!) */
  status = connect(s->sockfd, &GNET_INETADDR_SA(addr), 
		   GNET_INETADDR_LEN(addr));
  if (STATUS_IS_SOCKET_ERROR (status))
    {
      if (!ERROR_IS_CONNECT_IN_PROGRESS ())
	{
	  GNET_CLOSE_SOCKET(sockfd);    
	  g_free(s);
	  return NULL;
	}
    }

  /* Save address */ 
  s->sa = addr->sa;

  /* Note that if connect returns 0, then we're already connected and
     we could call the call back immediately.  But, it would probably
     make things too complicated for the user if we could call the
     callback before we returned from this function.  */

  /* Wait for the connection */
  state = g_new0 (GTcpSocketAsyncState, 1);
  state->socket = s;
  state->func = func;
  state->data = data;
  state->notify = notify;
#ifndef GNET_WIN32
  state->flags = flags;
#endif
  state->iochannel = g_io_channel_ref (gnet_tcp_socket_get_io_channel (s));
  state->context = g_main_context_ref (context);
  state->priority = priority;

  state->connect_watch = _gnet_io_watch_add_full (state->context,
      state->priority, state->iochannel, GNET_ANY_IO_CONDITION,
      gnet_tcp_socket_new_async_cb, state, NULL);

  return state;
}

/**
 *  gnet_tcp_socket_new_async_direct
 *  @addr: address
 *  @func: callback function
 *  @data: data to pass to @func on callback
 *
 *  Asynchronously creates a #GTcpSocket and connects to @addr without
 *  using SOCKS.  Most users should use gnet_tcp_socket_new_async()
 *  instead.  The callback is called once the connection is made or an
 *  error occurs.  The callback will not be called during the call to
 *  this function.
 *
 *  Returns: the ID of the connection; NULL on failure.  The ID can be
 *  used with gnet_tcp_socket_new_async_cancel() to cancel the
 *  connection.
 *
 **/
GTcpSocketNewAsyncID
gnet_tcp_socket_new_async_direct (const GInetAddr * addr, 
    GTcpSocketNewAsyncFunc func, gpointer data)
{
  GTcpSocketNewAsyncID async_id;

  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  async_id = gnet_tcp_socket_new_async_direct_full (addr, func, data,
      (GDestroyNotify) NULL, NULL, G_PRIORITY_DEFAULT);

  return async_id;
}


static gboolean 
gnet_tcp_socket_new_async_cb (GIOChannel* iochannel, 
			      GIOCondition condition, 
			      gpointer data)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) data;
  socklen_t len;
  gint error;

  if (!((condition & G_IO_IN) || (condition & G_IO_OUT)))
    goto error;

  len = sizeof(error);

  /* Get the error option */
  if (getsockopt(state->socket->sockfd, SOL_SOCKET, SO_ERROR, (void*) &error, &len) < 0)
    {
      g_warning ("getsockopt() failed");
      goto error;
    }

  /* Check if there is an error */
  if (error)
    goto error;

#ifndef GNET_WIN32
  /* Reset the flags */
  if (fcntl(state->socket->sockfd, F_SETFL, state->flags) != 0)
    {
      g_warning ("fcntl() failed");
      goto error;
    }
#endif

  /* Success */
  (*state->func) (state->socket, state->data);

done:

  state->connect_watch = 0;
  g_io_channel_unref (state->iochannel);
  g_main_context_unref (state->context);
  /* FIXME: is notify really useful in this context? */
  if (state->notify)
    state->notify (state->data);
  memset (state, 0xaa, sizeof (*state));
  g_free (state);
  return FALSE; /* don't call us again */

/* ERROR */
error:
  {
    (*state->func) (NULL, state->data);
    gnet_tcp_socket_delete (state->socket);
    goto done;
  }
}


/**
 *  gnet_tcp_socket_new_async_cancel
 *  @id: ID of the connection
 *
 *  Cancels an asynchronous #GTcpSocket creation that was started with
 *  gnet_tcp_socket_new_async().
 *
 **/
void
gnet_tcp_socket_new_async_cancel (GTcpSocketNewAsyncID id)
{
  GTcpSocketAsyncState* state = (GTcpSocketAsyncState*) id;

  if (state->connect_watch)
    _gnet_source_remove (state->context, state->connect_watch);
  if (state->iochannel)
    g_io_channel_unref (state->iochannel);
  gnet_tcp_socket_delete (state->socket);
  g_main_context_unref (state->context);
  if (state->notify)
    state->notify (state->data);
  g_free (state);
}



/**
 *  gnet_tcp_socket_delete
 *  @socket: a #GTcpSocket
 *
 *  Deletes a #GTcpSocket.
 *
 **/
void
gnet_tcp_socket_delete (GTcpSocket* socket)
{
  if (socket != NULL)
    gnet_tcp_socket_unref (socket);
}



/**
 *  gnet_tcp_socket_ref
 *  @socket: a #GTcpSocket
 *
 *  Adds a reference to a #GTcpSocket.
 *
 **/
void
gnet_tcp_socket_ref (GTcpSocket* socket)
{
  g_return_if_fail (socket != NULL);

  g_atomic_int_inc (&socket->ref_count);
}

/* returns TRUE if the socket has been deleted (useful for io watches) */
static gboolean
gnet_tcp_socket_unref_internal (GTcpSocket * socket)
{
  if (!g_atomic_int_dec_and_test (&socket->ref_count))
    return FALSE;

  if (socket->accept_watch)
    g_source_remove (socket->accept_watch);

  GNET_CLOSE_SOCKET (socket->sockfd); /* Don't care if this fails... */

  if (socket->iochannel)
    g_io_channel_unref (socket->iochannel);

  g_free (socket);
  return TRUE;
}

/**
 *  gnet_tcp_socket_unref
 *  @socket: a #GTcpSocket to unreference
 *
 *  Removes a reference from a #GTcpSocket.  A #GTcpSocket is deleted
 *  when the reference count reaches 0.
 *
 **/
void
gnet_tcp_socket_unref (GTcpSocket* socket)
{
  g_return_if_fail (socket != NULL);

  gnet_tcp_socket_unref_internal (socket);
}



/**
 *  gnet_tcp_socket_get_io_channel
 *  @socket: a #GTcpSocket
 *
 *  Gets the #GIOChannel of a #GTcpSocket.
 *
 *  For a client socket, the #GIOChannel represents the data stream.
 *  Use it like you would any other #GIOChannel.
 *
 *  For a server socket however, the #GIOChannel represents the
 *  listening socket.  When it's readable, there's a connection
 *  waiting to be accepted.  However, using
 *  gnet_tcp_socket_server_accept_async() is more elegant than
 *  watching the #GIOChannel.
 *
 *  Every #GTcpSocket has one and only one #GIOChannel.  If you ref
 *  the channel, then you must unref it eventually.  Do not close the
 *  channel.  The channel is closed by GNet when the socket is
 *  deleted.
 *
 *  Returns: a #GIOChannel.
 *
 **/
GIOChannel* 
gnet_tcp_socket_get_io_channel (GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = _gnet_io_channel_new(socket->sockfd);

  return socket->iochannel;
}


/**
 *  gnet_tcp_socket_get_remote_inetaddr
 *  @socket: a #GTcpSocket
 *
 *  Gets the address of the remote host from a #GTcpSocket.  This
 *  function does not work on server sockets.
 *
 *  Returns: a #GInetAddr.
 *
 **/
GInetAddr* 
gnet_tcp_socket_get_remote_inetaddr (const GTcpSocket* socket)
{
  GInetAddr* ia;

  g_return_val_if_fail (socket != NULL, NULL);

  ia = g_new0(GInetAddr, 1);
  ia->sa = socket->sa;
  ia->ref_count = 1;

  return ia;
}


/**
 *  gnet_tcp_socket_get_local_inetaddr
 *  @socket: a #GTcpSocket
 *
 *  Gets the local host's address from a #GTcpSocket.
 *
 *  Returns: a #GInetAddr, or NULL on error.  Unref with gnet_inetaddr_unref()
 *  when no longer needed.
 *
 **/
GInetAddr*  
gnet_tcp_socket_get_local_inetaddr (const GTcpSocket* socket)
{
  socklen_t socklen;
  struct sockaddr_storage sa;
  GInetAddr* ia;

  g_return_val_if_fail (socket, NULL);

  socklen = sizeof(sa);
  if (getsockname(socket->sockfd, &GNET_SOCKADDR_SA(sa), &socklen) != 0)
    return NULL;

  ia = g_new0(GInetAddr, 1);
  ia->ref_count = 1;
  ia->sa = sa;

  return ia;
}


/**
 *  gnet_tcp_socket_get_port
 *  @socket: a #GTcpSocket
 *
 *  Gets the port a server #GTcpSocket is bound to.
 *
 *  Returns: the port number.
 *
 **/
gint
gnet_tcp_socket_get_port (const GTcpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, 0);

  return g_ntohs(GNET_SOCKADDR_PORT(socket->sa));
}



/* **************************************** */

/**
 *  gnet_tcp_socket_set_tos
 *  @socket: a #GTcpSocket
 *  @tos: type of service
 *
 *  Sets the type-of-service (TOS) of the socket.  TOS theoretically
 *  controls the connection's quality of service, but most routers
 *  ignore it.  Some systems don't even support this function.  The
 *  function does nothing if the operating system does not support it.
 *
 **/
void
gnet_tcp_socket_set_tos (GTcpSocket* socket, GNetTOS tos)
{
  int sotos;

  g_return_if_fail (socket != NULL);

  /* Some systems (e.g. OpenBSD) do not have IPTOS_*.  Other systems
     have some of them, but not others.  And some systems have them,
     but with different names (e.g. FreeBSD has IPTOS_MINCOST).  If a
     system does not have a IPTOS, or any of them, then this function
     does nothing.  */
  switch (tos)
    {
#ifdef IPTOS_LOWDELAY
    case GNET_TOS_LOWDELAY:	sotos = IPTOS_LOWDELAY;		break;
#endif
#ifdef IPTOS_THROUGHPUT
    case GNET_TOS_THROUGHPUT:	sotos = IPTOS_THROUGHPUT;	break;
#endif
#ifdef IPTOS_RELIABILITY
    case GNET_TOS_RELIABILITY:	sotos = IPTOS_RELIABILITY;	break;
#endif
#ifdef IPTOS_LOWCOST
    case GNET_TOS_LOWCOST:	sotos = IPTOS_LOWCOST;		break;
#else
#ifdef IPTOS_MINCOST	/* Called MINCOST in FreeBSD 4.0 */
    case GNET_TOS_LOWCOST:	sotos = IPTOS_MINCOST;		break;
#endif
#endif
    default: return;
    }

#ifdef IP_TOS
  if (setsockopt(socket->sockfd, IPPROTO_IP, IP_TOS, (void*) &sotos, sizeof(sotos)) != 0)
    g_warning ("Can't set TOS on TCP socket\n");
#endif

}


/* **************************************** */
/* Server stuff */


/**
 *  gnet_tcp_socket_server_new
 *
 *  Creates a new #GTcpSocket bound to all interfaces and an arbitrary
 *  port.  SOCKS is used if SOCKS is enabled.
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new (void)
{
  return gnet_tcp_socket_server_new_full (NULL, 0);
}


/**
 *  gnet_tcp_socket_server_new_with_port
 *  @port: port to bind to (0 for an arbitrary port)
 *
 *  Creates a new #GTcpSocket bound to all interfaces and port @port.
 *  If @port is 0, an arbitrary port will be used.  SOCKS is used if
 *  SOCKS is enabled.
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new_with_port (gint port)
{
  return gnet_tcp_socket_server_new_full (NULL, port);
}


/**
 *  gnet_tcp_socket_server_new_full
 *  @iface: Interface to bind to (NULL for all interfaces)
 *  @port: Port to bind to (0 for an arbitrary port)
 *
 *  Creates and new #GTcpSocket bound to interface @iface and port
 *  @port.  If @iface is NULL, the socket is bound to all interfaces.
 *  If @port is 0, the socket is bound to an arbitrary port.  SOCKS is
 *  used if SOCKS is enabled and the interface is NULL.
 *
 *  Returns: a new #GTcpSocket; NULL on error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_new_full (const GInetAddr* iface, gint port)
{
  SOCKET sockfd = 0;
  struct sockaddr_storage sa;
  GTcpSocket* s;
  socklen_t socklen;
  const int on = 1;
#ifndef GNET_WIN32
  gint flags;
#endif


  /* Use SOCKS if enabled */
  if (!iface && gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_server_new (port);

  /* Create sockfd and address */
  sockfd = _gnet_create_listen_socket (SOCK_STREAM, iface, port, &sa);
  if (!GNET_IS_SOCKET_VALID(sockfd))
    return NULL;
  
  /* Set REUSEADDR so we can reuse the port */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	  (void*) &on, sizeof(on)) != 0)
	      g_warning("Can't set reuse on tcp socket\n");

#ifndef GNET_WIN32		/* Unix */
  {
    /* Get the flags (should all be 0?) */
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
      {
	g_warning ("fcntl() failed");
	goto error;
      }
    
    /* Make the socket non-blocking */
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
      {
	g_warning ("fcntl() failed");
	goto error;
      }
  }
#else /* Windows */
  {
    u_long arg;

	/* Make the socket non-blocking */
    arg = 1;
    if (ioctlsocket(sockfd, FIONBIO, &arg))
      goto error;
    }
#endif

  /* Bind */
  if (bind(sockfd, &GNET_SOCKADDR_SA(sa), GNET_SOCKADDR_LEN(sa)) != 0)
    goto error;
  
  /* Get the socket name */
  socklen = GNET_SOCKADDR_LEN(sa);
  if (getsockname(sockfd, &GNET_SOCKADDR_SA(sa), &socklen) != 0)
    goto error;
  
  /* Listen */
  if (listen(sockfd, 10) != 0)
    goto error;
  
  /* Create TcpSocket */
  s = g_new0(GTcpSocket, 1);
  s->sockfd = sockfd;
  s->sa = sa;
  s->ref_count = 1;

  return s;
  
 error:
  if (sockfd)    		
    GNET_CLOSE_SOCKET(sockfd);

  return NULL;
}




#ifndef GNET_WIN32  /*********** Unix code ***********/


/**
 *  gnet_tcp_socket_server_accept
 *  @socket: a #GTcpSocket
 *
 *  Accepts a connection from a #GTcpSocket.  The socket must have
 *  been created using gnet_tcp_socket_server_new() (or equivalent).
 *  Even if the socket's #GIOChannel is readable, the function may
 *  still block.
 *
 *  Returns: a new #GTcpSocket representing a new connection; NULL on
 *  error.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_accept (GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr_storage sa;
  socklen_t n;
  fd_set fdset;
  GTcpSocket* s;

  g_return_val_if_fail (socket != NULL, NULL);

  if (gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_server_accept(socket);

 try_again:
  
  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
    {
      if (errno == EINTR)
	goto try_again;
      
      return NULL;
    }

  n = sizeof(s->sa);
  
  if ((sockfd = accept(socket->sockfd, (struct sockaddr*) &sa, &n)) == -1)
    {
      if (errno == EWOULDBLOCK || 
	  errno == ECONNABORTED ||
#ifdef EPROTO		/* OpenBSD does not have EPROTO */
	  errno == EPROTO || 
#endif
	  errno == EINTR)
	goto try_again;

      return NULL;
    }

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  s->sa = sa;

  return s;
}





/**
 *  gnet_tcp_socket_server_accept_nonblock
 *  @socket: a #GTcpSocket
 *
 *  Accepts a connection from a #GTcpSocket without blocking.  The
 *  socket must have been created using gnet_tcp_socket_server_new()
 *  (or equivalent).
 *
 *  Note that if the socket's #GIOChannel is readable, then there is
 *  PROBABLY a new connection.  It is possible for the connection
 *  to close by the time this function is called, so it may return
 *  NULL.
 *
 *  Returns: a new #GTcpSocket representing a new connection; NULL
 *  otherwise.
 *
 **/
GTcpSocket* 
gnet_tcp_socket_server_accept_nonblock (GTcpSocket* socket)
{
  gint sockfd;
  struct sockaddr_storage sa;
  socklen_t n;
  fd_set fdset;
  GTcpSocket* s;
  struct timeval tv = {0, 0};

  g_return_val_if_fail (socket != NULL, NULL);

  if (gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_server_accept(socket);

 try_again:

  FD_ZERO(&fdset);
  FD_SET(socket->sockfd, &fdset);

  if (select(socket->sockfd + 1, &fdset, NULL, NULL, &tv) == -1)
    {
      if (errno == EINTR)
	goto try_again;

      return NULL;
    }

  n = sizeof(sa);
  if ((sockfd = accept(socket->sockfd, (struct sockaddr*) &sa, &n)) == -1)
    {
      /* If we get an error, return.  We don't want to try again as we
         do in gnet_tcp_socket_server_accept() - it might cause a
         block. */

      return NULL;
    }
  
  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  s->sa = sa;

  return s;
}


#else	/*********** Windows code ***********/


GTcpSocket*
gnet_tcp_socket_server_accept (GTcpSocket* socket)
{
  SOCKET sockfd;
  struct sockaddr_storage sa;
  fd_set fdset;
  GTcpSocket* s;
  socklen_t n;

  g_return_val_if_fail (socket != NULL, NULL);

  if (gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_server_accept(socket);
	
  FD_ZERO(&fdset);
  FD_SET((unsigned)socket->sockfd, &fdset);

  if (select((int)socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
      return NULL;

  /* Don't force the socket into blocking mode */

  n = sizeof(sa);
  sockfd = accept(socket->sockfd, (struct sockaddr*) &sa, &n);
  /* if it fails, looping isn't going to help */

  if (!GNET_IS_SOCKET_VALID(sockfd))
      return NULL;

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  s->sa = sa;

  return s;
}


GTcpSocket*
gnet_tcp_socket_server_accept_nonblock (GTcpSocket* socket)
{
  SOCKET sockfd;
  struct sockaddr_storage sa;

  fd_set fdset;
  GTcpSocket* s;
  u_long arg;
  socklen_t n;

  g_return_val_if_fail (socket != NULL, NULL);

  if (gnet_socks_get_enabled())
    return _gnet_socks_tcp_socket_server_accept(socket);

  FD_ZERO(&fdset);
  FD_SET((unsigned)socket->sockfd, &fdset);

  if (select((int)socket->sockfd + 1, &fdset, NULL, NULL, NULL) == -1)
      return NULL;

  /* make sure the socket is in non-blocking mode */
  arg = 1;
  if (ioctlsocket(socket->sockfd, FIONBIO, &arg))
    return NULL;

  n = sizeof(sa);
  sockfd = accept(socket->sockfd, (struct sockaddr*) &sa, &n);
  /* if it fails, looping isn't going to help */

  if (sockfd == INVALID_SOCKET)
      return NULL;

  s = g_new0(GTcpSocket, 1);
  s->ref_count = 1;
  s->sockfd = sockfd;
  s->sa = sa;

  return s;
}


#endif		/*********** End Windows code ***********/



static gboolean tcp_socket_server_accept_async_cb (GIOChannel* iochannel, 
						   GIOCondition condition, 
						   gpointer data);

/**
 *  gnet_tcp_socket_server_accept_async:
 *  @socket: a #GTcpSocket
 *  @accept_func: callback function.
 *  @user_data: data to pass to @func on callback
 *
 *  Asynchronously accepts a connection from a #GTcpSocket.  The
 *  callback is called when a new client has connected or an error
 *  occurs.  The socket must have been created using
 *  gnet_tcp_socket_server_new() (or equivalent).
 *
 **/
void
gnet_tcp_socket_server_accept_async (GTcpSocket* socket,
				     GTcpSocketAcceptFunc accept_func,
				     gpointer user_data)
{
  GIOChannel* iochannel;

  g_return_if_fail (socket);
  g_return_if_fail (accept_func);
  g_return_if_fail (!socket->accept_func);

  if (gnet_socks_get_enabled())
    {
      _gnet_socks_tcp_socket_server_accept_async (socket, accept_func, user_data);
      return;
    }

  /* Save callback */
  socket->accept_func = accept_func;
  socket->accept_data = user_data;

  /* Add read watch */
  iochannel = gnet_tcp_socket_get_io_channel (socket);
  socket->accept_watch = g_io_add_watch(iochannel, 
					G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
					tcp_socket_server_accept_async_cb, socket);
}



static gboolean
tcp_socket_server_accept_async_cb (GIOChannel* iochannel, GIOCondition condition, 
				   gpointer data)
{
  GTcpSocket* server = (GTcpSocket*) data;
  g_assert (server != NULL);

  if (condition & G_IO_IN)
    {
      GTcpSocket* client;

      client = gnet_tcp_socket_server_accept_nonblock (server);
      if (!client) 
	return TRUE;

      /* Do upcall, protected by a ref */
      gnet_tcp_socket_ref (server);

      (server->accept_func)(server, client, server->accept_data);

      if (gnet_tcp_socket_unref_internal (server) || !server->accept_watch)
	return FALSE;
    }
  else
    {
      gnet_tcp_socket_ref (server);
      (server->accept_func)(server, NULL, server->accept_data);
      server->accept_watch = 0;
      server->accept_func = NULL;
      server->accept_data = NULL;
      gnet_tcp_socket_unref (server);
      return FALSE;
    }

  return TRUE;
}



/**
 *  gnet_tcp_socket_server_accept_async_cancel
 *  @socket: a #GTcpSocket
 *
 *  Stops asynchronously accepting connections for a #GTcpSocket.  The
 *  socket is not closed.
 *
 **/
void
gnet_tcp_socket_server_accept_async_cancel (GTcpSocket* socket)
{
  g_return_if_fail (socket);

  if (!socket->accept_watch)
    return;

  socket->accept_func = NULL;
  socket->accept_data = NULL;

  g_source_remove (socket->accept_watch);
  socket->accept_watch = 0;
}
