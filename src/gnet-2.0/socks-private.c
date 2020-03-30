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

#include "gnet-private.h"
#include "socks.h"
#include "socks-private.h"



/* **************************************** */


static int socks_negotiate_connect (GTcpSocket *s, const GInetAddr *dst);
static int socks4_negotiate_connect (GIOChannel *ioc, const GInetAddr *dst);
static int socks5_negotiate_connect (GIOChannel *ioc, const GInetAddr *dst);


GTcpSocket* 
_gnet_socks_tcp_socket_new (const GInetAddr* addr)
{
  GInetAddr* 		ss_addr = NULL;
  GTcpSocket* 		s;
  int			rv;

  g_return_val_if_fail (addr != NULL, NULL);

  /* Get SOCKS server */
  ss_addr = gnet_socks_get_server();
  if (!ss_addr)
    return NULL;
  
  /* Connect to SOCKS server */
  s = gnet_tcp_socket_new_direct (ss_addr);
  gnet_inetaddr_delete (ss_addr);
  if (!s)
    return NULL;

  /* Negotiate connection */
  rv = socks_negotiate_connect (s, addr);
  if (rv < 0)
    {
      gnet_tcp_socket_delete (s);
      return NULL;
    }

  return s;
}



/* **************************************** */

typedef struct
{
  GInetAddr* addr;
  GTcpSocketNewAsyncFunc func;
  gpointer data;
  GDestroyNotify notify;
} SocksAsyncData;

static void
socks_async_cb (GTcpSocket* socket, gpointer data)
{
  SocksAsyncData *ad = (SocksAsyncData *) data;
  
  if (socket != NULL) {
    int rv;

    rv = socks_negotiate_connect (socket, ad->addr);
    if (rv < 0)
      goto error;

    (ad->func) (socket, ad->data);

    gnet_inetaddr_delete (ad->addr);
    if (ad->notify)
      ad->notify (ad->data);
    g_free (ad);
    return;
  }

error:
  {
    (ad->func) (NULL, ad->data);

    gnet_tcp_socket_delete (socket);
    gnet_inetaddr_delete (ad->addr);
    if (ad->notify)
      ad->notify (ad->data);
    g_free (ad);
  }
}

GTcpSocketNewAsyncID
_gnet_socks_tcp_socket_new_async_full (const GInetAddr * addr, 
    GTcpSocketNewAsyncFunc func, gpointer data, GDestroyNotify notify,
    GMainContext * context, gint priority)
{
  GTcpSocketNewAsyncID async_id;
  SocksAsyncData *ad;
  GInetAddr *ss_addr = NULL;

  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  /* Get SOCKS server */
  ss_addr = gnet_socks_get_server();
  if (!ss_addr)
    return NULL;

  /* Create data */
  ad = g_new0 (SocksAsyncData, 1);
  ad->addr = gnet_inetaddr_clone (addr);
  ad->func = func;
  ad->data = data;
  ad->notify = notify;

  /* Connect to SOCKS server */
  async_id = gnet_tcp_socket_new_async_direct_full (ss_addr, socks_async_cb,
      ad, (GDestroyNotify) NULL, context, priority);

  gnet_inetaddr_delete (ss_addr);

  return async_id;  /* async_id might be NULL */
}

GTcpSocketNewAsyncID
_gnet_socks_tcp_socket_new_async (const GInetAddr * addr,
    GTcpSocketNewAsyncFunc func, gpointer data)
{
  GTcpSocketNewAsyncID async_id;

  g_return_val_if_fail (addr != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  async_id = _gnet_socks_tcp_socket_new_async_full (addr, func, data,
      (GDestroyNotify) NULL, NULL, G_PRIORITY_DEFAULT);

  return async_id;
}

/* **************************************** */

static int
socks_negotiate_connect (GTcpSocket *s, const GInetAddr *dst)
{
  GIOChannel *ioc;
  int ver, ret;

  ioc = gnet_tcp_socket_get_io_channel(s);
  ver = gnet_socks_get_version();
  if (ver == 5)
    ret = socks5_negotiate_connect (ioc, dst);
  else if (ver == 4)
    ret = socks4_negotiate_connect (ioc, dst);
  else
    ret = -1;

  return ret;
}


static int
socks4_negotiate_connect (GIOChannel *ioc, const GInetAddr *dst)
{
  struct socks4_h s4h;
  struct sockaddr_in *sa_in;
  gsize len;

  sa_in = (struct sockaddr_in*)&dst->sa;

  s4h.vn = 4;
  s4h.cd = 1;
  s4h.dport = (short)sa_in->sin_port;
  s4h.dip = (long)sa_in->sin_addr.s_addr;
  s4h.userid = 0;

  if (gnet_io_channel_writen(ioc, &s4h, 9, &len) != G_IO_ERROR_NONE)
    return -1;
  if (gnet_io_channel_readn(ioc, &s4h, 8, &len) != G_IO_ERROR_NONE)
    return -1;

  if ((s4h.cd != 90) || (s4h.vn != 0))
    return -1;

  return 0;
}


static int
socks5_negotiate_connect (GIOChannel *ioc, const GInetAddr *dst)
{
  unsigned char s5r[3];
  struct socks5_h s5h;
  struct sockaddr_in *sa_in;
  gsize len;

  s5r[0] = 5;
  s5r[1] = 1;	/* XXX no authentication yet */
  s5r[2] = 0;	

  if (gnet_io_channel_writen(ioc, s5r, 3, &len) != G_IO_ERROR_NONE)
    return -1;
  if (gnet_io_channel_readn(ioc, s5r, 2, &len) != G_IO_ERROR_NONE)
    return -1;
  if ((s5r[0] != 5) || (s5r[1] != 0))
    return -1;
	
  sa_in = (struct sockaddr_in*)&dst->sa;

  /* fill in SOCKS5 request */
  s5h.vn = 5;
  s5h.cd = 1;
  s5h.rsv = 0;
  s5h.atyp = 1;
  s5h.dip = (long)sa_in->sin_addr.s_addr; 
  s5h.dport = (short)sa_in->sin_port;

  if (gnet_io_channel_writen(ioc, (gchar*)&s5h, 10, &len) != G_IO_ERROR_NONE)
    return -1;
  if (gnet_io_channel_readn(ioc, (gchar*)&s5h, 10, &len) != G_IO_ERROR_NONE)
    return -1;
  if (s5h.cd != 0)
    return -1;

  return 0;
}


/* **************************************** */

static int socks5_negotiate_bind (GTcpSocket* socket, int port);

static gboolean socks_tcp_socket_server_accept_async_cb (GIOChannel* iochannel, 
							 GIOCondition condition, 
							 gpointer data);


GTcpSocket*
_gnet_socks_tcp_socket_server_new (gint port)
{
  GInetAddr* 		ss_addr = NULL;
  GTcpSocket* 		s;
  int			rv;

  /* We only support SOCKS 5 */
  if (gnet_socks_get_version () != 5)
    return NULL;

  /* Get SOCKS server */
  ss_addr = gnet_socks_get_server();
  if (!ss_addr)
    return NULL;

  /* Connect to SOCKS server */
  s = gnet_tcp_socket_new_direct (ss_addr);
  gnet_inetaddr_delete (ss_addr);
  if (!s)
    return NULL;

  /* Negotiate connection */
  rv = socks5_negotiate_bind (s, port);
  if (rv < 0)
    {
      gnet_tcp_socket_delete (s);
      return NULL;
    }

  return s;
}


static int
socks5_negotiate_bind (GTcpSocket* socket, int port)
{
  GIOChannel *ioc;
  unsigned char s5r[3];
  struct socks5_h s5h;
  gsize len;

  ioc = gnet_tcp_socket_get_io_channel(socket);

  s5r[0] = 5;
  s5r[1] = 1;	/* no authentication */
  s5r[2] = 0;	

  if (gnet_io_channel_writen(ioc, s5r, 3, &len) != G_IO_ERROR_NONE)
    goto error;
  if (gnet_io_channel_readn(ioc, s5r, 2, &len) != G_IO_ERROR_NONE)
    goto error;
  if ((s5r[0] != 5) || (s5r[1] != 0))
    goto error;
	
  /* fill in SOCKS5 request */
  s5h.vn = 5;
  s5h.cd = 2;  /* bind */
  s5h.rsv = 0;
  s5h.atyp = 1;
  s5h.dip = 0; /* FIX: this works with nylon; i will check on rfc */
  s5h.dport = g_htons(port);   

  if (gnet_io_channel_writen(ioc, (gchar*)&s5h, 10, &len) != G_IO_ERROR_NONE)
    goto error;
  /* this reply simply confirms */
  if (gnet_io_channel_readn(ioc, (gchar*)&s5h, 10, &len) != G_IO_ERROR_NONE)
    goto error;
  /* make sure we have a connection */
  if (s5h.cd != 0)
    goto error;

  /* Copy the address */
  GNET_SOCKADDR_IN(socket->sa).sin_addr.s_addr = s5h.dip;
  GNET_SOCKADDR_IN(socket->sa).sin_port = s5h.dport;

  return 0;

 error:
  return -1;
}


/* XXX 0 server SOCKS compliant? */
 
GTcpSocket*
_gnet_socks_tcp_socket_server_accept (GTcpSocket* socket)
{
  gint server_port;
  struct socks5_h s5h;
  gsize len;
  GIOChannel* iochannel;
  GIOError error;
  GTcpSocket* s;
  GTcpSocket* new_socket;

  g_return_val_if_fail (socket, NULL);

  /* Save server port */
  server_port = g_ntohs(GNET_SOCKADDR_IN(socket->sa).sin_port);

  /* this reply reveals the connecting hosts ip and port */
  iochannel = gnet_tcp_socket_get_io_channel(socket);
  error = gnet_io_channel_readn(iochannel, (gchar*) &s5h, 10, &len);
  if (error != G_IO_ERROR_NONE)
    return NULL;

  /* The client socket is the server socket */
  /* FIXME: this code should be in tcp.c, just like the struct definition */
  s = g_new0(GTcpSocket, 1);
  s->sockfd = socket->sockfd;
  GNET_SOCKADDR_IN(s->sa).sin_addr.s_addr = s5h.dip;
  GNET_SOCKADDR_IN(s->sa).sin_port = s5h.dport;
  s->ref_count = 1;

  /* Create a new server socket (we just use the sockfd) */
  new_socket = _gnet_socks_tcp_socket_server_new (server_port);
  if (new_socket == NULL)
    {
      g_free (s); /* ok, we copied sockfd */
      return NULL;
    }
  
  /* Copy the fd over and delete the new socket */
  socket->sockfd = new_socket->sockfd;
  g_free (new_socket); /* ok, we copied sockfd */

  /* Hand over IOChannel */
  if (socket->accept_watch)
    {
      g_source_remove (socket->accept_watch);
      socket->accept_watch = 0;
    }
  s->iochannel = socket->iochannel;
  socket->iochannel = NULL;

  /* Reset the async watch if necessary */
  if (socket->accept_func)
    {
      GIOChannel* iochannel;

      /* This will recreate the IOChannel */
      iochannel = gnet_tcp_socket_get_io_channel (socket);

      /* Set the watch on the new IO channel */
      socket->accept_watch = g_io_add_watch(iochannel, 
					    G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
					    socks_tcp_socket_server_accept_async_cb, socket);
    }

  return s;
}


void
_gnet_socks_tcp_socket_server_accept_async (GTcpSocket * socket,
    GTcpSocketAcceptFunc accept_func, gpointer user_data)
{
  GIOChannel* iochannel;

  g_return_if_fail (socket);
  g_return_if_fail (accept_func);
  g_return_if_fail (!socket->accept_func);

  /* Save callback */
  socket->accept_func = accept_func;
  socket->accept_data = user_data;

  /* Add read watch */
  iochannel = gnet_tcp_socket_get_io_channel (socket);
  socket->accept_watch = g_io_add_watch(iochannel, 
					G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, 
					socks_tcp_socket_server_accept_async_cb, socket);
}



static gboolean
socks_tcp_socket_server_accept_async_cb (GIOChannel* iochannel, GIOCondition condition, 
					 gpointer data)
{
  GTcpSocket* server = (GTcpSocket*) data;

  g_assert (server);

  if (condition & G_IO_IN)
    {
      GTcpSocket* client;

      client = _gnet_socks_tcp_socket_server_accept (server);
      if (!client) 
	return TRUE;

      (server->accept_func)(server, client, server->accept_data);

      return FALSE;
    }
  else /* error */
    {
      gnet_tcp_socket_ref (server);

      (server->accept_func)(server, NULL, server->accept_data);

      server->accept_watch = 0;
      server->accept_func = NULL;
      server->accept_data = NULL;

      gnet_tcp_socket_unref (server);

      return FALSE;
    }

  return FALSE;
}
