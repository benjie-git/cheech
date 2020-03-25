/* GNet - Networking library
 * Copyright (C) 2001-2002  Marius Eriksen, David Helder
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

#ifndef _GNET_SOCKS_PRIVATE_H
#define _GNET_SOCKS_PRIVATE_H

#include "gnet-private.h"

struct socks4_h {
  guint8  vn;
  guint8  cd;
  guint16 dport;
  guint32 dip;
  guint8  userid;
};

struct socks5_h {
  guint8  vn;
  guint8  cd;
  guint8  rsv;
  guint8  atyp;	
  guint32 dip;
  guint16 dport;
};

/* SOCKS client */

GTcpSocket          * _gnet_socks_tcp_socket_new (const GInetAddr* addr);

GTcpSocketNewAsyncID  _gnet_socks_tcp_socket_new_async      (const GInetAddr        * addr, 
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data);

GTcpSocketNewAsyncID  _gnet_socks_tcp_socket_new_async_full (const GInetAddr        * addr, 
                                                             GTcpSocketNewAsyncFunc   func,
                                                             gpointer                 data,
                                                             GDestroyNotify           notify,
                                                             GMainContext           * context,
                                                             gint                     priority);

/* SOCKS server */

GTcpSocket * _gnet_socks_tcp_socket_server_new (gint port);

GTcpSocket * _gnet_socks_tcp_socket_server_accept (GTcpSocket * s);

void         _gnet_socks_tcp_socket_server_accept_async (GTcpSocket           * s, 
                                                         GTcpSocketAcceptFunc   accept_func, 
                                                         gpointer               user_data);

#endif /* _GNET_SOCKS_PRIVATE_H */
