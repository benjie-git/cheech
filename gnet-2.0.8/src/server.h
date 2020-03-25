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


#ifndef _GNET_SERVER_H
#define _GNET_SERVER_H

#include <glib.h>
#include "gnetconfig.h"
#include "tcp.h"
#include "conn.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/**
 *  GServer
 *  @iface: interface address
 *  @port: port number
 *  @socket: TCP server socket
 *  @ref_count: [private]
 *  @func: callback function
 *  @user_data: user data for callback
 *
 *  #GServer is a high-level interface to a TCP server socket.  The
 *  callback is called with a #GConn whenever a new connection is
 *  made.
 *
 **/
typedef struct _GServer GServer;


/**
 *   GServerFunc:
 *   @server: server
 *   @conn: new connection (or NULL if error)
 *   @user_data: user data specified in gnet_server_new()
 *   
 *   Callback for gnet_server_new().  When a client connects, this
 *   callback is called with a new connection.  If the server fails,
 *   this callback is called with @conn set to NULL.  The callback is
 *   not called again.
 *
 *   If @conn is non-NULL, the address (IP, port) of the client that
 *   established the connection can be found in the inetaddr member of
 *   the #GConn structure.
 *
 **/
typedef void (*GServerFunc)(GServer* server,
			    GConn* conn,
			    gpointer user_data);


struct _GServer
{
  GInetAddr* 	iface;
  gint		port;

  GTcpSocket* 	socket;

  guint 	ref_count;

  GServerFunc	func;
  gpointer	user_data;

};


GServer*  gnet_server_new (const GInetAddr* iface, gint port, 
			   GServerFunc func, gpointer user_data);

void      gnet_server_delete (GServer* server);

void	  gnet_server_ref (GServer* server);
void	  gnet_server_unref (GServer* server);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_SERVER_H */
