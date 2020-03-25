/* GNet - Networking library
 * Copyright (C) 2000-2002  David Helder
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

#include <memory.h>
#include "gnet-private.h"
#include "server.h"



static void server_accept_cb (GTcpSocket* server_socket, GTcpSocket* client, gpointer data);


/**
 *  gnet_server_new:
 *  @iface: interface to bind to (NULL for all interfaces)
 *  @port: port to bind to (0 for an arbitrary port)
 *  @func: callback to call when a connection is accepted
 *  @user_data: data to pass to callback
 *
 *  Creates a new #GServer object representing a server.  Usually,
 *  @iface is set to NULL to bind to all interfaces and @port is a
 *  specific number.  The callback is called whenever a new connection
 *  arrives or if there is a server error.  The callback is not called
 *  again after a server error.
 *
 *  Returns: a new #GServer.
 *
 **/
GServer*
gnet_server_new (const GInetAddr* iface, gint port, 
		 GServerFunc func, gpointer user_data)
{
  GTcpSocket* socket;
  GServer* server = NULL;

  g_return_val_if_fail (func, NULL);

  socket = gnet_tcp_socket_server_new_full (iface, port);
  if (!socket)
    return NULL;

  server = g_new0 (GServer, 1);
  server->ref_count = 1;
  server->func = func;
  server->user_data = user_data;
  server->socket = socket;
  server->iface = gnet_tcp_socket_get_local_inetaddr (server->socket);
  server->port  = gnet_tcp_socket_get_port (server->socket);

  /* Wait for new connections */
  gnet_tcp_socket_server_accept_async (server->socket, 
				       server_accept_cb, server);

  return server;
}



/**
 *  gnet_server_delete:
 *  @server: #GServer to delete.
 *
 *  Closes and deletes a #GServer.
 *
 **/
void
gnet_server_delete (GServer* server)
{
  if (server != NULL)
    gnet_server_unref (server);
}


/**
 *  gnet_server_ref
 *  @server: a #GServer
 *
 *  Adds a reference to a #GServer.
 *
 **/
void
gnet_server_ref (GServer* server)
{
  g_return_if_fail (server);

  server->ref_count++;
}


/**
 *  gnet_server_unref
 *  @server: a #GServer
 *
 *  Removes a reference from a #GServer.  A #GServer is deleted when
 *  the reference count reaches 0.
 *
 **/
void
gnet_server_unref (GServer* server)
{
  server->ref_count--;
  if (server->ref_count > 0)
    return;

  if (server->socket)	 
    gnet_tcp_socket_delete (server->socket);
  if (server->iface)     	 
    gnet_inetaddr_delete (server->iface);
  g_free (server);
}



static void
server_accept_cb (GTcpSocket* server_socket, GTcpSocket* client, gpointer data)
{
  GServer* server = (GServer*) data;

  g_return_if_fail (server);

  if (client)
    {
      GConn* conn; 

      conn = gnet_conn_new_socket (client, NULL, NULL);

      (server->func)(server, conn, server->user_data);
    }
  else
    {
      gnet_tcp_socket_server_accept_async_cancel (server_socket);
      (server->func)(server, NULL, server->user_data);
    }
}

