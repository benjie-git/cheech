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


#ifndef _GNET_UDP_H
#define _GNET_UDP_H

#include "inetaddr.h"

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *  GUdpSocket
 *
 *  A #GUdpSocket structure represents a UDP socket.  The
 *  implementation is hidden.
 *
 **/
typedef struct _GUdpSocket GUdpSocket;



/* ******************************************** */
/* UDP socket functions				*/

GUdpSocket* gnet_udp_socket_new (void);
GUdpSocket* gnet_udp_socket_new_with_port (gint port);
GUdpSocket* gnet_udp_socket_new_full (const GInetAddr* iface, gint port);

void 	    gnet_udp_socket_delete (GUdpSocket* socket);

void 	    gnet_udp_socket_ref (GUdpSocket* socket);
void 	    gnet_udp_socket_unref (GUdpSocket* socket);

GIOChannel* gnet_udp_socket_get_io_channel (GUdpSocket* socket);
GInetAddr*  gnet_udp_socket_get_local_inetaddr (const GUdpSocket* socket);


/* ********** */

gint 	 gnet_udp_socket_send (GUdpSocket* socket, const gchar* buffer, 
 			       gint length, const GInetAddr* dst);
gint 	 gnet_udp_socket_receive (GUdpSocket* socket, gchar* buffer,
				  gint length, GInetAddr** src);
gboolean gnet_udp_socket_has_packet (const GUdpSocket* socket);


/* ********** */

gint 	 gnet_udp_socket_get_ttl (const GUdpSocket* socket);
gint 	 gnet_udp_socket_set_ttl (GUdpSocket* socket, gint ttl);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_UDP_H */
