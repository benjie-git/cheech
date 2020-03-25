/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2001  Mark Ferlatte
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

#ifndef _GNET_UNIX_H
#define _GNET_UNIX_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *  GUnixSocket
 *
 *  A #GUnixSocket structure represents a Unix socket.  The
 *  implementation is hidden.
 *
 **/
typedef struct _GUnixSocket GUnixSocket;

GUnixSocket* gnet_unix_socket_new (const gchar* path);
GUnixSocket* gnet_unix_socket_new_abstract (const gchar* path);

void 	     gnet_unix_socket_delete (GUnixSocket* socket);

void 	     gnet_unix_socket_ref (GUnixSocket* socket);
void 	     gnet_unix_socket_unref (GUnixSocket* socket);

GIOChannel*  gnet_unix_socket_get_io_channel (GUnixSocket* socket);

gchar*       gnet_unix_socket_get_path (const GUnixSocket* socket);

GUnixSocket* gnet_unix_socket_server_new (const gchar* path);
GUnixSocket* gnet_unix_socket_server_new_abstract (const gchar* path);

GUnixSocket* gnet_unix_socket_server_accept (const GUnixSocket* socket);
GUnixSocket* gnet_unix_socket_server_accept_nonblock (const GUnixSocket* socket);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNET_UNIX_H */
