/* GNet - Networking library
 * Copyright (C) 2000-3  David Helder
 * Copyright (C) 2000  Andrew Lanoix
 * Copyright (C) 2007  Tim-Philipp Müller <tim at centricular dot net>
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


#ifndef _GNET_TCP_H
#define _GNET_TCP_H

#include "inetaddr.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 *  GTcpSocket
 *
 *  A #GTcpSocket structure represents a TCP socket.  The
 *  implementation is hidden.
 *
 **/
typedef struct _GTcpSocket GTcpSocket;


/* **************************************** */

GTcpSocket* gnet_tcp_socket_connect (const gchar* hostname, gint port);
GTcpSocket* gnet_tcp_socket_new (const GInetAddr* addr);
GTcpSocket* gnet_tcp_socket_new_direct (const GInetAddr* addr);

void 	    gnet_tcp_socket_delete (GTcpSocket* socket);

void 	    gnet_tcp_socket_ref (GTcpSocket* socket);
void 	    gnet_tcp_socket_unref (GTcpSocket* socket);


GIOChannel* gnet_tcp_socket_get_io_channel (GTcpSocket* socket);
GInetAddr*  gnet_tcp_socket_get_remote_inetaddr (const GTcpSocket* socket);
GInetAddr*  gnet_tcp_socket_get_local_inetaddr (const GTcpSocket* socket);


GTcpSocket* gnet_tcp_socket_server_new (void);
GTcpSocket* gnet_tcp_socket_server_new_with_port (gint port);
GTcpSocket* gnet_tcp_socket_server_new_full (const GInetAddr* iface, gint port);

GTcpSocket* gnet_tcp_socket_server_accept (GTcpSocket* socket);
GTcpSocket* gnet_tcp_socket_server_accept_nonblock (GTcpSocket* socket);

gint        gnet_tcp_socket_get_port (const GTcpSocket* socket);


/* ********** */

/**
 *  GNetTOS
 *  @GNET_TOS_NONE: Unspecified
 *  @GNET_TOS_LOWDELAY: Low delay
 *  @GNET_TOS_THROUGHPUT: High throughput
 *  @GNET_TOS_RELIABILITY: High reliability
 *  @GNET_TOS_LOWCOST: Low cost
 *
 *  Type-of-service.
 *
 **/
typedef enum
{
  GNET_TOS_NONE,
  GNET_TOS_LOWDELAY,
  GNET_TOS_THROUGHPUT,
  GNET_TOS_RELIABILITY,
  GNET_TOS_LOWCOST

} GNetTOS;

void 	    gnet_tcp_socket_set_tos (GTcpSocket* socket, GNetTOS tos);



/* **************************************** */
/* Asynchronous functions		    */


/**
 *  GTcpSocketConnectAsyncID
 *  
 *  ID of an asynchronous connection started with
 *  gnet_tcp_socket_connect_async().  The connection can be canceled
 *  by calling gnet_tcp_socket_connect_async_cancel() with the ID.
 *
 **/
typedef struct _GTcpSocketConnectState * GTcpSocketConnectAsyncID;



/**
 *  GTcpSocketConnectAsyncStatus
 *  @GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK: Connection succeeded
 *  @GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR: Error, address lookup failed
 *  @GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR: Error, could not connect
 *  
 *  Status for connecting via gnet_tcp_socket_connect_async(), passed
 *  by GTcpSocketConnectAsyncFunc.  More errors may be added in the
 *  future, so it's best to compare against
 *  %GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK.
 *
 **/
typedef enum {
  GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK,
  GTCP_SOCKET_CONNECT_ASYNC_STATUS_INETADDR_ERROR,
  GTCP_SOCKET_CONNECT_ASYNC_STATUS_TCP_ERROR
} GTcpSocketConnectAsyncStatus;



/**
 *  GTcpSocketConnectAsyncFunc:
 *  @socket: TcpSocket that was connecting (callee owned)
 *  @status: Status of the connection
 *  @data: User data
 *  
 *  Callback for gnet_tcp_socket_connect_async().
 *
 **/
typedef void (*GTcpSocketConnectAsyncFunc) (GTcpSocket                   * socket, 
                                            GTcpSocketConnectAsyncStatus   status, 
                                            gpointer                       data);


GTcpSocketConnectAsyncID  gnet_tcp_socket_connect_async      (const gchar              * hostname,
                                                              gint                       port, 
                                                              GTcpSocketConnectAsyncFunc func, 
                                                              gpointer                   data);

GTcpSocketConnectAsyncID  gnet_tcp_socket_connect_async_full (const gchar              * hostname,
                                                              gint                       port, 
                                                              GTcpSocketConnectAsyncFunc func,
                                                              gpointer                   data,
                                                              GDestroyNotify             notify,
                                                              GMainContext             * context,
                                                              gint                       priority);

void                      gnet_tcp_socket_connect_async_cancel (GTcpSocketConnectAsyncID id);

/* ********** */


/**
 *  GTcpSocketNewAsyncID:
 *  
 *  ID of an asynchronous tcp socket creation started with
 *  gnet_tcp_socket_new_async().  The creation can be canceled by
 *  calling gnet_tcp_socket_new_async_cancel() with the ID.
 *
 **/
typedef struct _GTcpSocketAsyncState * GTcpSocketNewAsyncID;



/**
 *  GTcpSocketNewAsyncFunc:
 *  @socket: Socket that was connecting
 *  @data: User data
 *  
 *  Callback for gnet_tcp_socket_new_async().  The socket will be
 *  NULL if the connection failed.
 *
 **/
typedef void (*GTcpSocketNewAsyncFunc) (GTcpSocket * socket, gpointer data);

GTcpSocketNewAsyncID  gnet_tcp_socket_new_async             (const GInetAddr        * addr, 
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data);

GTcpSocketNewAsyncID  gnet_tcp_socket_new_async_full        (const GInetAddr        * addr, 
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data,
                                                             GDestroyNotify           notify,
                                                             GMainContext           * context,
                                                             gint                     priority);

void                  gnet_tcp_socket_new_async_cancel      (GTcpSocketNewAsyncID id);


GTcpSocketNewAsyncID  gnet_tcp_socket_new_async_direct      (const GInetAddr        * addr,
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data);

GTcpSocketNewAsyncID  gnet_tcp_socket_new_async_direct_full (const GInetAddr        * addr, 
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data,
                                                             GDestroyNotify           notify,
                                                             GMainContext           * context,
                                                             gint                     priority);


/* ********** */


/**
 *   GTcpSocketAcceptFunc:
 *   @server: Server socket
 *   @client: Client socket
 *   @data: User data
 *   
 *   Callback for gnet_tcp_socket_server_accept_async().  The socket
 *   had an irrecoverable error if client_socket is NULL.
 *
 **/
typedef void (*GTcpSocketAcceptFunc)(GTcpSocket* server, 
				     GTcpSocket* client,
				     gpointer data);

void gnet_tcp_socket_server_accept_async (GTcpSocket* socket,
					  GTcpSocketAcceptFunc accept_func,
					  gpointer user_data);
void gnet_tcp_socket_server_accept_async_cancel (GTcpSocket* socket);


G_END_DECLS

#endif /* _GNET_TCP_H */
