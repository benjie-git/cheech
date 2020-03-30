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

#include "gnet-private.h"
#include "mcast.h"

#define GNET_UDP_SOCKET(s)       ((GUdpSocket*)(s))
#define GNET_IS_MCAST_SOCKET(s)  ((s)&&(GNET_UDP_SOCKET(s)->type == GNET_MCAST_SOCKET_TYPE_COOKIE))

/**
 *  gnet_mcast_socket_new
 *  
 *  Creates a #GMcastSocket bound to all interfaces and an arbitrary
 *  port.
 *
 *  Returns: a new #GMcastSocket; NULL on error.
 *
 **/
GMcastSocket*
gnet_mcast_socket_new (void)
{
  return gnet_mcast_socket_new_full (NULL, 0);
}



/**
 *  gnet_mcast_socket_new_with_port
 *  @port: port to bind to
 *
 *  Creates a #GMcastSocket bound to all interfaces and port @port.
 *  If @port is 0, an arbitrary port will be used.  Use this
 *  constructor if you know the port of the group you will join.  Most
 *  applications will use this constructor.
 *
 *  Returns: a new #GMcastSocket; NULL on error.
 *
 **/
GMcastSocket* 
gnet_mcast_socket_new_with_port (gint port)
{
  return gnet_mcast_socket_new_full (NULL, port);
}



/**
 *  gnet_mcast_socket_new_full
 *  @iface: interface to bind to (NULL for all interfaces)
 *  @port: port to bind to (0 for an arbitrary port)
 *
 *  Creates a #GMcastSocket bound to interface @iface and port @port.
 *  If @iface is NULL, all interfaces will be used.  If @port is 0, an
 *  arbitrary port will be used.  To receive packets from this group,
 *  call gnet_mcast_socket_join_group() next.  Loopback is disabled by
 *  default.
 *
 *  Returns: a new #GMcastSocket; NULL on error.
 *
 **/
GMcastSocket* 
gnet_mcast_socket_new_full (const GInetAddr* iface, gint port)
{
  struct sockaddr_storage sa;
  GMcastSocket*           ms;
  GUdpSocket*             us;
  const int               on = 1;
  int                     sockfd;

  /* Create sockfd and address */
  sockfd = _gnet_create_listen_socket (SOCK_DGRAM, iface, port, &sa);
  if (!GNET_IS_SOCKET_VALID(sockfd))
    {
      g_warning ("socket() failed");
      return NULL;
    }

  /* Set socket option to share the UDP port */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
		 (void*) &on, sizeof(on)) != 0)
    g_warning("Can't reuse mcast socket\n");

  /* Bind to the socket to some local address and port */
  if (bind(sockfd, &GNET_SOCKADDR_SA(sa), GNET_SOCKADDR_LEN(sa)) != 0)
    {
      GNET_CLOSE_SOCKET (sockfd);
      return NULL;
    }

  /* Create socket */
  ms = g_new0(GMcastSocket, 1);
  us = (GUdpSocket *) ms;
  us->type = GNET_MCAST_SOCKET_TYPE_COOKIE;
  us->sockfd = sockfd;
  us->sa = sa;
  us->ref_count = 1;

  gnet_mcast_socket_set_loopback (ms, FALSE);

  return ms;
}



/**
 *  gnet_mcast_socket_delete:
 *  @socket: a #GMcastSocket, or NULL
 *
 *  Deletes a #GMcastSocket. Does nothing if @socket is NULL.
 *
 **/
void
gnet_mcast_socket_delete (GMcastSocket* socket)
{
  g_return_if_fail (socket == NULL || GNET_IS_MCAST_SOCKET (socket));

  gnet_udp_socket_unref ((GUdpSocket *) socket);
}



/**
 *  gnet_mcast_socket_ref
 *  @socket: a #GMcastSocket
 *
 *  Adds a reference to a #GMcastSocket.
 *
 **/
void
gnet_mcast_socket_ref (GMcastSocket* socket)
{
  g_return_if_fail (socket != NULL);
  g_return_if_fail (GNET_IS_MCAST_SOCKET (socket));

  gnet_udp_socket_ref ((GUdpSocket *) socket);
}


/**
 *  gnet_mcast_socket_unref
 *  @socket: a #GMcastSocket
 *
 *  Removes a reference from a #GMcastSocket.  A #GMcastSocket is
 *  deleted when the reference count reaches 0.
 *
 **/
void
gnet_mcast_socket_unref (GMcastSocket* socket)
{
  g_return_if_fail (socket != NULL);
  g_return_if_fail (GNET_IS_MCAST_SOCKET (socket));

  gnet_udp_socket_unref ((GUdpSocket *) socket);
}



/**
 *  gnet_mcast_socket_get_io_channel:
 *  @socket: a #GMcastSocket
 *
 *  Gets the #GIOChannel of a #GMcastSocket.
 *
 *  Use the channel with g_io_add_watch() to check if the socket is
 *  readable or writable.  If the channel is readable, call
 *  gnet_mcast_socket_receive() to receive a packet.  If the channel
 *  is writable, call gnet_mcast_socket_send() to send a packet.  This
 *  is not a normal giochannel - do not read from or write to it.
 *
 *  Every #GMcastSocket has one and only one #GIOChannel.  If you ref
 *  the channel, then you must unref it eventually.  Do not close the
 *  channel.  The channel is closed by GNet when the socket is
 *  deleted.
 *
 *  Returns: a #GIOChannel.
 *
 **/
GIOChannel*   
gnet_mcast_socket_get_io_channel (GMcastSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), NULL);

  return gnet_udp_socket_get_io_channel((GUdpSocket*) socket);
}


/**
 *  gnet_mcast_socket_get_local_inetaddr
 *  @socket: a #GMcastSocket
 *
 *  Gets the local host's address from a #GMcastSocket.
 *
 *  Returns: a #GInetAddr.
 *
 **/
GInetAddr*  
gnet_mcast_socket_get_local_inetaddr (const GMcastSocket* socket)
{
  g_return_val_if_fail (socket != NULL, NULL);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), NULL);

  return gnet_udp_socket_get_local_inetaddr((GUdpSocket*) socket);
}



/**
 *  gnet_mcast_socket_join_group
 *  @socket: a #GMcastSocket
 *  @inetaddr: address of the group
 *
 *  Joins a multicast group.  Join only one group per socket.
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_join_group (GMcastSocket* socket, 
			      const GInetAddr* inetaddr)
{
  GUdpSocket *udpsocket;
  gint rv = -1;

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -1);

  udpsocket = GNET_UDP_SOCKET (socket);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      struct ip_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.imr_multiaddr, GNET_INETADDR_ADDRP(inetaddr), 
	     sizeof(mreq.imr_multiaddr));
      mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

      /* Join the group */
      rv = setsockopt (udpsocket->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
          (void*) &mreq, sizeof(mreq));
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      struct ipv6_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.ipv6mr_multiaddr, GNET_INETADDR_ADDRP(inetaddr), 
	     sizeof(mreq.ipv6mr_multiaddr));
      mreq.ipv6mr_interface = 0;

      /* Join the group */
      rv = setsockopt (udpsocket->sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
          (void*) &mreq, sizeof(mreq));
    }
#endif
  else
    g_assert_not_reached ();

  return rv;
}


/**
 *  gnet_mcast_socket_leave_group
 *  @socket: a #GMcastSocket
 *  @inetaddr: address of the group
 *
 *  Leaves a mulitcast group.
 *
 *  Returns: 0 on success.
 *
 **/
gint
gnet_mcast_socket_leave_group (GMcastSocket* socket, 
			       const GInetAddr* inetaddr)
{
  GUdpSocket *udpsocket;

  gint rv = -1;

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -1);

  udpsocket = GNET_UDP_SOCKET (socket);

  if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET)
    {
      struct ip_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.imr_multiaddr, GNET_INETADDR_ADDRP(inetaddr), 
	     sizeof(mreq.imr_multiaddr));
      mreq.imr_interface.s_addr = g_htonl(INADDR_ANY);

      /* Join the group */
      rv = setsockopt (udpsocket->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
          (void*) &mreq, sizeof(mreq));
    }
#ifdef HAVE_IPV6
  else if (GNET_INETADDR_FAMILY(inetaddr) == AF_INET6)
    {
      struct ipv6_mreq mreq;

      /* Create the multicast request structure */
      memcpy(&mreq.ipv6mr_multiaddr, GNET_INETADDR_ADDRP(inetaddr), 
	     sizeof(mreq.ipv6mr_multiaddr));
      mreq.ipv6mr_interface = 0;

      /* Join the group */
      rv = setsockopt (udpsocket->sockfd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
          (void*) &mreq, sizeof(mreq));
    }
#endif
  else
    g_assert_not_reached ();

  return rv;
}



/**
 *  gnet_mcast_socket_get_ttl
 *  @socket: a #GMcastSocket
 *
 *  Gets the multicast time-to-live (TTL) of a #GMcastSocket.  All IP
 *  multicast packets have a TTL field.  This field is decremented by
 *  a router before it forwards the packet.  If the TTL reaches zero,
 *  the packet is discarded.  The default value is sufficient for most
 *  applications.
 *
 *  The table below shows the scope for a given TTL.  The scope is
 *  only an estimate.
 *
 *  <table>
 *    <title>TTL and scope</title>
 *    <tgroup cols="2" align="left">
 *    <thead>
 *      <row>
 *        <entry>TTL</entry>
 *        <entry>Scope</entry>
 *      </row>
 *    </thead>
 *    <tbody>
 *      <row>
 *        <entry>0</entry>
 *        <entry>node local</entry>
 *      </row>
 *      <row>
 *        <entry>1</entry>
 *        <entry>link local</entry>
 *      </row>
 *      <row>
 *        <entry>2-32</entry>
 *        <entry>site local</entry>
 *      </row>
 *      <row>
 *        <entry>33-64</entry>
 *        <entry>region local</entry>
 *      </row>
 *      <row>
 *        <entry>65-128</entry>
 *        <entry>continent local</entry>
 *      </row>
 *      <row>
 *        <entry>129-255</entry>
 *        <entry>unrestricted (global)</entry>
 *      </row>
 *    </tbody>
 *    </tgroup>
 *  </table>
 *
 *  Returns: the TTL (an integer between 0 and 255), -1 if the kernel
 *  default is being used, or an integer less than -1 on error.
 *
 **/
gint
gnet_mcast_socket_get_ttl (const GMcastSocket* socket)
{
  GUdpSocket *udpsocket;
  guchar ttl;
  socklen_t ttl_size;
  int rv = -1;

  g_return_val_if_fail (socket != NULL, -2);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -2);

  udpsocket = GNET_UDP_SOCKET (socket);

  ttl_size = sizeof(ttl);

  /* Get the IPv4 TTL if the socket is bound to an IPv4 address */
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET)
    {
      rv = getsockopt (udpsocket->sockfd, IPPROTO_IP,
          IP_MULTICAST_TTL, (void*) &ttl, &ttl_size);
    }

  /* Get the IPv6 TTL if the socket is bound to an IPv6 address */
#ifdef HAVE_IPV6
  else if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6)
    {
      rv = getsockopt (udpsocket->sockfd, IPPROTO_IPV6,
          IPV6_MULTICAST_HOPS, (void*) &ttl, &ttl_size);
    }
#endif
  else
    g_assert_not_reached ();

  if (rv == -1)
    return -2;

  return ttl;
}


/**
 *  gnet_mcast_socket_set_ttl
 *  @socket: a #GMcastSocket
 *  @ttl: value to set TTL to
 *
 *  Sets the time-to-live (TTL) default of a #GMcastSocket.  Set the TTL
 *  to -1 to use the kernel default.  The default value is sufficient
 *  for most applications.
 *
 *  Returns: 0 if successful.  
 *
 **/
gint
gnet_mcast_socket_set_ttl (GMcastSocket* socket, gint ttl)
{
  GUdpSocket *udpsocket;
  guchar ttlb;
  int rv1, rv2;
#ifdef HAVE_IPV6
  GIPv6Policy policy;
#endif

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -1);

  udpsocket = GNET_UDP_SOCKET (socket);

  rv1 = -1;
  rv2 = -1;

  /* If the bind address is anything IPv4 *or* the bind address is
     0::0 IPv6 and IPv6 policy allows IPv4, set IP_TTL.  In the latter case,
     if we bind to 0::0 and the host is dual-stacked, then all IPv4
     interfaces will be bound to also. */
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET 
#ifdef HAVE_IPV6
      || (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6(udpsocket->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4))
#endif
      )
    {
      ttlb = ttl;
      rv1 = setsockopt (udpsocket->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
		       (void*) &ttlb, sizeof(ttlb));
    }


  /* If the bind address is IPv6, set IPV6_UNICAST_HOPS */
#ifdef HAVE_IPV6
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6)
    {
      ttlb = ttl;
      rv2 = setsockopt (udpsocket->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, 
		       (void*) &ttlb, sizeof(ttlb));
    }
#endif

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}



/**
 *  gnet_mcast_socket_is_loopback
 *  @socket: a #GMcastSocket
 *
 *  Checks if a #GMcastSocket has loopback enabled.  If loopback is
 *  enabled, all packets sent by the host will also be received by the
 *  host.  Loopback is disabled by default.
 *
 *  Returns: 0 if loopback is disabled, 1 if enabled, and -1 on error.
 *
 **/
gint
gnet_mcast_socket_is_loopback (const GMcastSocket* socket)
{
  GUdpSocket *udpsocket;
  socklen_t flag_size;
  int rv = -1;
  gint is_loopback = 0;

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -1);

  udpsocket = GNET_UDP_SOCKET (socket);

  /* Get IPv4 loopback if it's bound to a IPv4 address */
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET)
    {
      guchar flag;

      flag_size = sizeof (flag);
      rv = getsockopt (udpsocket->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, 
		      (char *) &flag, &flag_size);
      if (flag)
	is_loopback = 1;
    }

  /* Otherwise, get IPv6 loopback */
#ifdef HAVE_IPV6
  else if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6)
    {
      guint flag;

      flag_size = sizeof (flag);
      rv = getsockopt (udpsocket->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, 
		      (void *) &flag, &flag_size);
      if (flag)
	is_loopback = 1;
    }
#endif
  else
    g_assert_not_reached();

  if (rv == -1)
    return -1;

  return is_loopback;
}



/**
 *  gnet_mcast_socket_set_loopback
 *  @socket: a #GMcastSocket
 *  @enable: should loopback be enabled?
 *
 *  Enables (or disables) loopback on a #GMcastSocket.  Loopback is
 *  disabled by default.
 *
 *  Returns: 0 if successful.
 *
 **/
gint
gnet_mcast_socket_set_loopback (GMcastSocket* socket, gboolean enable)
{
  GUdpSocket *udpsocket;
  int rv1, rv2;
#ifdef HAVE_IPV6
  GIPv6Policy policy;
#endif

  g_return_val_if_fail (socket != NULL, -1);
  g_return_val_if_fail (GNET_IS_MCAST_SOCKET (socket), -1);

  udpsocket = GNET_UDP_SOCKET (socket);

  rv1 = -1;
  rv2 = -1;

  /* Set IPv4 loopback.  (As in set_ttl().) */
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET 
#ifdef HAVE_IPV6
      || (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&GNET_SOCKADDR_SA6 (udpsocket->sa).sin6_addr) &&
       ((policy = gnet_ipv6_get_policy()) == GIPV6_POLICY_IPV4_THEN_IPV6 ||
	policy == GIPV6_POLICY_IPV6_THEN_IPV4))
#endif
      )
    {
      guchar flag;

      flag = (guchar) enable;

      rv1 = setsockopt (udpsocket->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
		       (char *) &flag, sizeof(flag));
    }

  /* Set IPv6 loopback */
#ifdef HAVE_IPV6
  if (GNET_SOCKADDR_FAMILY (udpsocket->sa) == AF_INET6)
    {
      guint flag;

      flag = (guint) enable;

      rv2 = setsockopt (udpsocket->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       (void *) &flag, sizeof(flag));
    }
#endif

  if (rv1 == -1 && rv2 == -1)
    return -1;

  return 0;
}



/**
 *  gnet_mcast_socket_send
 *  @socket: a #GMcastSocket
 *  @buffer: buffer to send
 *  @length: length of buffer
 *  @dst: destination address
 *
 *  Sends data to a host using a #GMcastSocket.
 *
 *  Returns: 0 if successful.
 *
 **/
gint 
gnet_mcast_socket_send (GMcastSocket* socket, const gchar* buffer, gint length, 
			const GInetAddr* dst)
{
  return gnet_udp_socket_send((GUdpSocket*) socket, buffer, length, dst);
}


/**
 *  gnet_mcast_socket_receive
 *  @socket: a #GMcastSocket
 *  @buffer: buffer to write to
 *  @length: length of @buffer
 *  @src: pointer to source address (optional)
 *
 *  Receives data using a #GMcastSocket.  If @src is set, the source
 *  address is stored in the location @src points to.  The address is
 *  caller owned.
 *
 *  Returns: the number of bytes received, -1 if unsuccessful.
 *
 **/
gint 
gnet_mcast_socket_receive (GMcastSocket* socket, gchar* buffer, gint length,
			   GInetAddr** src)
{
  return gnet_udp_socket_receive((GUdpSocket*) socket, buffer, length, src);
}


/**
 *  gnet_mcast_socket_has_packet:
 *  @socket: a #GMcastSocket
 *
 *  Tests if a #GMcastSocket has a packet waiting to be received.
 *
 *  Returns: TRUE if there is packet waiting, FALSE otherwise.
 *
 **/
gboolean
gnet_mcast_socket_has_packet (const GMcastSocket* socket)
{
  return gnet_udp_socket_has_packet((const GUdpSocket*) socket);
}

