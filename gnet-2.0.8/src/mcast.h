/* GNet - Networking library
 * Copyright (C) 2000, 2002-3  David Helder
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

#ifndef _GNET_MCAST_H
#define _GNET_MCAST_H

#include "inetaddr.h"
#include "udp.h"

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *  GMcastSocket
 *
 *  A #GMcastSocket structure represents a IP Multicast socket.  The
 *  implementation is hidden.  A #GMcastSocket is child of
 *  #GUdpSocket.  Use gnet_mcast_socket_to_udp_socket() to safely cast
 *  a #GMcastSocket to a #GUdpSocket.
 *
 **/
typedef struct _GMcastSocket GMcastSocket;



/* ********** */

GMcastSocket* gnet_mcast_socket_new(void);
GMcastSocket* gnet_mcast_socket_new_with_port (gint port);
GMcastSocket* gnet_mcast_socket_new_full (const GInetAddr* iface, gint port);

void          gnet_mcast_socket_delete (GMcastSocket* socket);

void 	      gnet_mcast_socket_ref (GMcastSocket* socket);
void 	      gnet_mcast_socket_unref (GMcastSocket* socket);

GIOChannel*   gnet_mcast_socket_get_io_channel (GMcastSocket* socket);
GInetAddr*    gnet_mcast_socket_get_local_inetaddr (const GMcastSocket* socket);


/* ********** */

gint 	 gnet_mcast_socket_join_group (GMcastSocket* socket, 
				       const GInetAddr* inetaddr);
gint 	 gnet_mcast_socket_leave_group (GMcastSocket* socket, 
					const GInetAddr* inetaddr);

gint 	 gnet_mcast_socket_get_ttl (const GMcastSocket* socket);
gint 	 gnet_mcast_socket_set_ttl (GMcastSocket* socket, gint ttl);

gint 	 gnet_mcast_socket_is_loopback (const GMcastSocket* socket);
gint 	 gnet_mcast_socket_set_loopback (GMcastSocket* socket, gboolean enable);

/* ********** */

gint     gnet_mcast_socket_send (GMcastSocket* socket, const gchar* buffer, 
				 gint length, const GInetAddr* dst);
gint     gnet_mcast_socket_receive (GMcastSocket* socket, gchar* buffer, 
				    gint length, GInetAddr** src);
gboolean gnet_mcast_socket_has_packet (const GMcastSocket* socket);


/**
 *  gnet_mcast_socket_to_udp_socket
 *  @MS: a #GMcastSocket
 *
 *  Converts a #GMcastSocket to a #GUdpSocket.  A #GMcastSocket is a
 *  child of #GUdpSocket.
 *
 **/
#define gnet_mcast_socket_to_udp_socket(MS) ((GUdpSocket*) (MS))

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_MCAST_H */
