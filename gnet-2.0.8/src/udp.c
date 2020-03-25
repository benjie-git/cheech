/* GNet - Networking library
 * Copyright (C) 2000-2003  David Helder
 * Copyright (C) 2000-2003  Andrew Lanoix
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
#include "udp.h"

#define GNET_UDP_SOCKET_TYPE(s) (((GUdpSocket*)(s))->type)
#define GNET_IS_UDP_SOCKET(s)   ((s) && (GNET_UDP_SOCKET_TYPE(s) == GNET_UDP_SOCKET_TYPE_COOKIE \
                                      || GNET_UDP_SOCKET_TYPE(s) == GNET_MCAST_SOCKET_TYPE_COOKIE))

/**
 *  gnet_udp_socket_new
 *  
 *  Creates a #GUdpSocket bound to all interfaces and an arbitrary
 *  port.
 *
 *  Returns: a new #GUdpSocket; NULL on error.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new (void)
{
  return gnet_udp_socket_new_full (NULL, 0);
}


/**
 *  gnet_udp_socket_new_with_port:
 *  @port: port to bind to (0 for an arbitrary port)
 * 
 *  Creates a #GUdpSocket bound to all interfaces and port @port.  If
 *  @port is 0, an arbitrary port will be used.
 *
 *  Returns: a new #GUdpSocket; NULL on error.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new_with_port (gint port)
{
  return gnet_udp_socket_new_full (NULL, port);
}


/**
 *  gnet_udp_socket_new_full
 *  @iface: interface to bind to (NULL for all interfaces)
 *  @port: port to bind to (0 for an arbitrary port)
 * 
 *  Creates a #GUdpSocket bound to interface @iface and port @port.
 *  If @iface is NULL, all interfaces will be used.  If @port is 0, an
 *  arbitrary port will be used.
 *
 *  Returns: a new #GUdpSocket; NULL on error.
 *
 **/
GUdpSocket* 
gnet_udp_socket_new_full (const GInetAddr* iface, gint port)
{
  SOCKET 			  sockfd;
  struct sockaddr_storage sa;
  GUdpSocket* 		  s;
  const int 		  on = 1;

  /* Create sockfd and address */
  sockfd = _gnet_create_listen_socket (SOCK_DGRAM, iface, port, &sa);
  if (!GNET_IS_SOCKET_VALID(sockfd))
    {
      g_warning ("socket() failed");
      return NULL;
    }

  /* Set broadcast option.  This allows the user to broadcast packets.
     It has no effect otherwise. */
  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
		 (void*) &on, sizeof(on)) != 0)
    {
      g_warning ("setsockopt() failed");
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  /* Bind to the socket to some local address and port */
  if (bind(sockfd, &GNET_SOCKADDR_SA(sa), GNET_SOCKADDR_LEN(sa)) != 0)
    {
      GNET_CLOSE_SOCKET(sockfd);
      return NULL;
    }

  /* Create UDP socket */
  s = g_new0 (GUdpSocket, 1);
  s->type = GNET_UDP_SOCKET_TYPE_COOKIE;
  s->sockfd = sockfd;
  s->sa = sa;
  s->ref_count = 1;

  return s;
}



/**
 *  gnet_udp_socket_delete:
 *  @socket: a #GUdpSocket, or NULL
 *
 *  Deletes a #GUdpSocket. Does nothing if @socket is NULL.
 *
 **/
void
gnet_udp_socket_delete (GUdpSocket* socket)
{
  if (socket != NULL) {
    g_return_if_fail (GNET_IS_UDP_SOCKET (socket));

    gnet_udp_socket_unref (socket);
  }
}



/**
 *  gnet_udp_socket_ref
 *  @socket: #GUdpSocket to reference
 *
 *  Adds a reference to a #GUdpSocket.
 *
 **/
void
gnet_udp_socket_ref (GUdpSocket* socket)
{
  g_return_if_fail (socket != NULL);
  g_return_if_fail (GNET_IS_UDP_SOCKET (socket));

  g_atomic_int_inc (&socket->ref_count);
}


/**
 *  gnet_udp_socket_unref
 *  @socket: a #GUdpSocket
 *
 *  Removes a reference from a #GUdpScoket.  A #GUdpSocket is deleted
 *  when the reference count reaches 0.
 *
 **/
void
gnet_udp_socket_unref (GUdpSocket* socket)
{
  g_return_if_fail (socket != NULL);
  g_return_if_fail (GNET_IS_UDP_SOCKET (socket));

  if (g_atomic_int_dec_and_test (&socket->ref_count)) {
    GNET_CLOSE_SOCKET (socket->sockfd);  /* Don't care if this fails... */

    if (socket->iochannel)
      g_io_channel_unref (socket->iochannel);

    socket->type = 0;
    g_free (socket);
  }
}



/**
 *  gnet_udp_socket_get_io_channel
 *  @socket: a #GUdpSocket
 *
 *  Gets the #GIOChannel of a #GUdpSocket.
 *
 *  Use the channel with g_io_add_watch() to check if the socket is
 *  readable or writable.  If the channel is readable, call
 *  gnet_udp_socket_receive() to receive a packet.  If the channel is
 *  writable, call gnet_udp_socket_send() to send a packet.  This is
 *  not a normal giochannel - do not read from or write to it.
 *
 *  Every #GUdpSocket has one and only one #GIOChannel.  If you ref
 *  the channel, then you must unref it eventually.  Do not close the
 *  channel.  The channel is closed by GNet when the socket is
 *  deleted.
 *
 *  Before deleting the UDP socket, make sure to remove any watches you have
 *  added with g_io_add_watch() again with g_source_remove() using the integer
 *  id returned by g_io_add_watch(). You may find your program stuck in a busy
 *  loop at 100% CPU utilisation if you forget to do this.
 *
 *  Returns: a #GIOChannel.
 *
 **/
GIOChannel* 
gnet_udp_socket_get_io_channel (GUdpSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), NULL);

  if (socket->iochannel == NULL)
    socket->iochannel = _gnet_io_channel_new (socket->sockfd);
  
  return socket->iochannel;
}



/**
 *  gnet_udp_socket_get_local_inetaddr
 *  @socket: a #GUdpSocket
 *
 *  Gets the local host's address from a #GUdpSocket.
 *
 *  Returns: a #GInetAddr.
 *
 **/
GInetAddr*  
gnet_udp_socket_get_local_inetaddr (const GUdpSocket* socket)
{
  socklen_t socklen;
  struct sockaddr_storage sa;
  GInetAddr* ia;

  g_return_val_if_fail (socket, NULL);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), NULL);

  socklen = sizeof(sa);
  if (getsockname(socket->sockfd, &GNET_SOCKADDR_SA(sa), &socklen) != 0)
    return NULL;

  ia = g_new0(GInetAddr, 1);
  ia->ref_count = 1;
  ia->sa = sa;

  return ia;
}


/**
 *  gnet_udp_socket_send
 *  @socket: a #GUdpSocket
 *  @buffer: buffer to send
 *  @length: length of @buffer
 *  @dst: destination address
 *
 *  Sends data to a host using a #GUdpSocket.
 *
 *  Returns: 0 if successful; something else on error.
 *
 **/
gint 
gnet_udp_socket_send (GUdpSocket* socket, 
		      const gchar* buffer, gint length, 
		      const GInetAddr* dst)
{
  gint bytes_sent;
  struct sockaddr_storage sa;

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), -1);
  g_return_val_if_fail (dst != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);

  /* Address of dst must be address of socket */
#ifdef HAVE_IPV6
  if (GNET_INETADDR_FAMILY(dst) != GNET_SOCKADDR_FAMILY(socket->sa))
    {
      /* If dst is IPv4, map to IPv6 */
      if (GNET_INETADDR_FAMILY(dst) == AF_INET && 
	  GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET6)
	{
          GNET_SOCKADDR_FAMILY(sa) = AF_INET6;
	  GNET_SOCKADDR_SET_SS_LEN(sa);
          GNET_SOCKADDR_PORT_SET(sa, GNET_INETADDR_PORT(dst));
          GNET_SOCKADDR_ADDR32_SET(sa, 0, 0);
          GNET_SOCKADDR_ADDR32_SET(sa, 1, 0);
          GNET_SOCKADDR_ADDR32_SET(sa, 2, g_htonl(0xffff));
          GNET_SOCKADDR_ADDR32_SET(sa, 3, GNET_INETADDR_ADDR32(dst, 0));
	}

      /* If dst is IPv6, map to IPv4 if possible */
      else if (GNET_INETADDR_FAMILY(dst) == AF_INET6 &&
               GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET &&
               IN6_IS_ADDR_V4MAPPED(&GNET_INETADDR_SA6(dst).sin6_addr))
	{
          GNET_SOCKADDR_FAMILY(sa) = AF_INET;
	  GNET_SOCKADDR_SET_SS_LEN(sa);
          GNET_SOCKADDR_PORT_SET(sa, GNET_INETADDR_PORT(dst));
          GNET_SOCKADDR_ADDR32_SET(sa, 0, GNET_INETADDR_ADDR32(dst, 3));
	}
      else
        return -1;

    }
    /* Addresses match - just copy the address */
    else
#endif
      {
	sa = dst->sa;
      }


  bytes_sent = sendto (socket->sockfd,
                       (void*) buffer, length, 0,
                       (struct sockaddr*) &sa, GNET_SOCKADDR_LEN(sa));

  if (bytes_sent != length)
    return -1;

  return 0;
}



/**
 *  gnet_udp_socket_receive
 *  @socket: a #GUdpSocket
 *  @buffer: buffer to write to
 *  @length: length of @buffer
 *  @src: pointer to source address (optional)
 *
 *  Receives data using a #GUdpSocket.  If @src is set, the source
 *  address is stored in the location @src points to.  The address is
 *  caller owned.
 *
 *  Returns: the number of bytes received, -1 on error.
 *
 **/
gint 
gnet_udp_socket_receive (GUdpSocket* socket, 
			 gchar* buffer, gint length,
			 GInetAddr** src)
{
  gint bytes_received;
  struct sockaddr_storage sa;
  socklen_t sa_len = sizeof (struct sockaddr_storage);

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (buffer != NULL, -1);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), -1);

  bytes_received = recvfrom (socket->sockfd, 
			     (void*) buffer, length, 
			     0, (struct sockaddr*) &sa, &sa_len);

  if (bytes_received == -1)
    return -1;

  if (src)
    {
      (*src) = g_new0 (GInetAddr, 1);
      (*src)->sa = sa;
      (*src)->ref_count = 1;
    }

  return bytes_received;
}


#ifndef GNET_WIN32  /*********** Unix code ***********/


/**
 *  gnet_udp_socket_has_packet
 *  @socket: a #GUdpSocket
 *
 *  Tests if a #GUdpSocket has a packet waiting to be received.
 *
 *  Returns: TRUE if there is packet waiting, FALSE otherwise.
 *
 **/
gboolean
gnet_udp_socket_has_packet (const GUdpSocket* socket)
{
  fd_set readfds;
  struct timeval timeout = {0, 0};

  g_return_val_if_fail (socket != NULL, FALSE);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), FALSE);

  FD_ZERO (&readfds);
  FD_SET (socket->sockfd, &readfds);
  if ((select(socket->sockfd + 1, &readfds, NULL, NULL, &timeout)) == 1)
    {
      return TRUE;
    }

  return FALSE;
}


#else	/*********** Windows code ***********/


gboolean
gnet_udp_socket_has_packet (const GUdpSocket* socket)
{
  gint bytes_received;
  gchar data[1];
  guint packetlength;
  u_long arg;
  gint error;

  g_return_val_if_fail (socket != NULL, FALSE);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), FALSE);

  arg = 1;
  ioctlsocket(socket->sockfd, FIONBIO, &arg); /* set to non-blocking mode */

  packetlength = 1;
  bytes_received = recvfrom(socket->sockfd, (void*) data, packetlength, 
			    MSG_PEEK, NULL, NULL);

  error = WSAGetLastError();

  arg = 0;
  ioctlsocket(socket->sockfd, FIONBIO, &arg); /* set blocking mode */

  if (bytes_received == SOCKET_ERROR)
    {
      if (WSAEMSGSIZE != error)
	{
	  return FALSE;
	}
      /* else, the buffer was not big enough, which is fine since we
	 just want to see if a packet is there..*/
    }

  if (bytes_received)
    return TRUE;

  return FALSE;
}
	
#endif		/*********** End Windows code ***********/



/**
 *  gnet_udp_socket_get_ttl
 *  @socket: a #GUdpSocket
 *
 *  Gets the time-to-live (TTL) default of a #GUdpSocket.  All UDP
 *  packets have a TTL field.  This field is decremented by a router
 *  before it forwards the packet.  If the TTL reaches zero, the
 *  packet is discarded.  The default value is sufficient for most
 *  applications.
 *
 *  Returns: the TTL (an integer between 0 and 255), -1 if the kernel
 *  default is being used, or an integer less than -1 on error.
 *
 **/
gint
gnet_udp_socket_get_ttl (const GUdpSocket* socket)
{
  int ttl;
  socklen_t ttl_size;
  int rv = -2;

  g_return_val_if_fail (socket != NULL, FALSE);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), FALSE);

  ttl_size = sizeof(ttl);

  /* Get the IPv4 TTL if it's bound to an IPv4 address */
  if (GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET)
    {
      rv = getsockopt(socket->sockfd, IPPROTO_IP, IP_TTL, 
		      (void*) &ttl, &ttl_size);
    }

  /* Or, get the IPv6 TTL if it's bound to an IPv6 address */
#ifdef HAVE_IPV6
  else if (GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET6)
    {
      rv = getsockopt(socket->sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, 
		      (void*) &ttl, &ttl_size);
    }
#endif
  else
    g_assert_not_reached();

  
  if (rv == -1)
    return -2;

  return ttl;
}


/**
 *  gnet_udp_socket_set_ttl
 *  @socket: a #GUdpSocket
 *  @ttl: value to set TTL to
 *
 *  Sets the time-to-live (TTL) default of a #GUdpSocket.  Set the TTL
 *  to -1 to use the kernel default.  The default value is sufficient
 *  for most applications.
 *
 *  Returns: 0 if successful.
 *
 **/
gint
gnet_udp_socket_set_ttl (GUdpSocket* socket, gint ttl)
{
  int rv1, rv2;
#ifdef HAVE_IPV6
  GIPv6Policy policy;
#endif

  g_return_val_if_fail (socket != NULL, FALSE);
  g_return_val_if_fail (GNET_IS_UDP_SOCKET (socket), FALSE);

  rv1 = -1;
  rv2 = -1;

  /* If the bind address is anything IPv4 *or* the bind address is
     0::0 IPv6 and IPv6 policy allows IPv4, set IP_TTL.  In the latter case,
     if we bind to 0::0 and the host is dual-stacked, then all IPv4
     interfaces will be bound to also. */
  if (GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET 
#ifdef HAVE_IPV6
      || (GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6(socket->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4))
#endif
      )
    {
      rv1 = setsockopt(socket->sockfd, IPPROTO_IP, IP_TTL, 
		       (void*) &ttl, sizeof(ttl));
    }


  /* If the bind address is IPv6, set IPV6_UNICAST_HOPS */
#ifdef HAVE_IPV6
  if (GNET_SOCKADDR_FAMILY(socket->sa) == AF_INET6)
    {
      rv2 = setsockopt(socket->sockfd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, 
		       (void*) &ttl, sizeof(ttl));
    }
#endif

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}
