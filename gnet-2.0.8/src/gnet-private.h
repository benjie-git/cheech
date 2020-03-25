/* GNet - Networking library
 * Copyright (C) 2000  David Helder
 * Copyright (C) 2000-2003  Andrew Lanoix
 * Copyright (C) 2007 Tim-Philipp Müller <tim centricular net>
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

#ifndef _GNET_PRIVATE_H
#define _GNET_PRIVATE_H

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <glib.h>
#include "gnet.h"
#include "gnetconfig.h"


#ifndef GNET_WIN32  /*********** Unix specific ***********/

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <netinet/in_systm.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>		/* Need for TOS */
#include <arpa/inet.h>

#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#ifndef HAVE_SOCKADDR_STORAGE
struct sockaddr_storage {
#ifdef HAVE_SOCKADDR_LEN
		unsigned char ss_len;
		unsigned char ss_family;
#else
        unsigned short ss_family;
#endif
        char info[126];
};
#endif

#ifndef SOCKET
#define SOCKET gint
#endif

#define GNET_CLOSE_SOCKET(SOCKFD) close(SOCKFD)

/* Use _gnet_io_channel_new() to create iochannels */
#define GNET_SOCKET_IO_CHANNEL_NEW(SOCKFD) g_io_channel_unix_new(SOCKFD)

#define GNET_IS_SOCKET_VALID(S) ((S) >= 0)
#define GNET_INVALID_SOCKET (-1)

#define STATUS_IS_SOCKET_ERROR(status) ((status) < 0)

#define ERROR_IS_CONNECT_IN_PROGRESS() (errno == EINPROGRESS)

#else	/*********** Windows specific ***********/

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef socklen_t
#  define socklen_t gint32
#endif
#define in_addr_t guint32

#define STATUS_IS_SOCKET_ERROR(status) ((status) == SOCKET_ERROR)

#define ERROR_IS_CONNECT_IN_PROGRESS() (WSAGetLastError () == WSAEWOULDBLOCK)

#define GNET_IS_SOCKET_VALID(S) ((S) != INVALID_SOCKET)
#define GNET_INVALID_SOCKET (INVALID_SOCKET)

#define GNET_CLOSE_SOCKET(SOCKFD) closesocket(SOCKFD)

/* Use _gnet_io_channel_new() to create iochannels */
#define GNET_SOCKET_IO_CHANNEL_NEW(SOCKFD) g_io_channel_win32_new_socket(SOCKFD)

#endif	/*********** End Windows specific ***********/

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#ifndef IN_LOOPBACKNET /* not defined in Win32 */
#define IN_LOOPBACKNET 127
#endif

#ifndef GNET_SOCKADDR_FAMILY_FIELD_NAME
# define GNET_SOCKADDR_FAMILY_FIELD_NAME ss_family
#endif

#define GNET_SOCKADDR_IN(s)     (*((struct sockaddr_in*) &s))
#define GNET_SOCKADDR_SA(s)     (*((struct sockaddr*) &s))
#define GNET_SOCKADDR_SA4(s)    (*((struct sockaddr_in*) &s))
#define GNET_SOCKADDR_FAMILY(s) ((s).GNET_SOCKADDR_FAMILY_FIELD_NAME)

#ifdef HAVE_IPV6

#ifndef IN6_ARE_ADDR_EQUAL
#define IN6_ARE_ADDR_EQUAL(a,b) \
        ((((unsigned int *) (a))[0] == ((unsigned int *) (b))[0]) && \
         (((unsigned int *) (a))[1] == ((unsigned int *) (b))[1]) && \
         (((unsigned int *) (a))[2] == ((unsigned int *) (b))[2]) && \
         (((unsigned int *) (a))[3] == ((unsigned int *) (b))[3]))
#endif

#define GNET_SOCKADDR_SA6(s)	(*((struct sockaddr_in6*) &s))
#define GNET_SOCKADDR_ADDRP(s)	((GNET_SOCKADDR_FAMILY((s)) == AF_INET)?\
                                  (void*)&((struct sockaddr_in*)&s)->sin_addr:\
                                  (void*)&((struct sockaddr_in6*)&s)->sin6_addr)
#define GNET_SOCKADDR_ADDR32(s,n)((GNET_SOCKADDR_FAMILY((s)) == AF_INET)?\
                                  ((struct sockaddr_in*)&s)->sin_addr.s_addr:\
                                  *(guint32*)&((struct sockaddr_in6*)&s)->sin6_addr.s6_addr[(n)*4])
#define GNET_SOCKADDR_ADDR32_SET(s,n,a) if (GNET_SOCKADDR_FAMILY((s)) == AF_INET) \
                                          ((struct sockaddr_in*)&s)->sin_addr.s_addr = a; \
                                        else \
                                          *(guint32*)&((struct sockaddr_in6*)&s)->sin6_addr.s6_addr[(n)*4] = a;
#define GNET_SOCKADDR_ADDRLEN(s) ((GNET_SOCKADDR_FAMILY((s)) == AF_INET)?\
				 sizeof(struct in_addr):\
				 sizeof(struct in6_addr))
#define GNET_SOCKADDR_PORT(s)	((GNET_SOCKADDR_FAMILY((s)) == AF_INET)?\
                                  ((struct sockaddr_in*)&s)->sin_port:\
                                  ((struct sockaddr_in6*)&s)->sin6_port)
#define GNET_SOCKADDR_PORT_SET(s, p)	if (GNET_SOCKADDR_FAMILY((s)) == AF_INET)\
                                          ((struct sockaddr_in*)&(s))->sin_port = p;\
                                        else \
                                          ((struct sockaddr_in6*)&(s))->sin6_port = p;
#define GNET_SOCKADDR_LEN(s)	((GNET_SOCKADDR_FAMILY((s)) == AF_INET)?\
                                  sizeof(struct sockaddr_in):\
                                  sizeof(struct sockaddr_in6))
#else /* NO IPV6 */

#define GNET_SOCKADDR_ADDRP(s)	(void*)&((struct sockaddr_in*)&s)->sin_addr
#define GNET_SOCKADDR_ADDR32(s,n)((struct sockaddr_in*)&s)->sin_addr.s_addr
#define GNET_SOCKADDR_ADDRLEN(s) sizeof(struct in_addr)
#define GNET_SOCKADDR_PORT(s)	((struct sockaddr_in*)&s)->sin_port
#define GNET_SOCKADDR_LEN(s)	sizeof(struct sockaddr_in)
#define GNET_SOCKADDR_PORT_SET(s, p) ((struct sockaddr_in*)&(s))->sin_port = p;

#endif


#ifdef HAVE_SOCKADDR_SA_LEN
#define GNET_SOCKADDR_SET_SS_LEN(s) do{(s).ss_len = GNET_SOCKADDR_LEN(s);}while(0)
#else
#define GNET_SOCKADDR_SET_SS_LEN(s) while(0){} /* do nothing */
#endif


#define GNET_INETADDR_SA(i)         GNET_SOCKADDR_SA((i)->sa)
#define GNET_INETADDR_SA4(i)        GNET_SOCKADDR_SA4((i)->sa)
#define GNET_INETADDR_SA6(i)        GNET_SOCKADDR_SA6((i)->sa) 
#define GNET_INETADDR_FAMILY(i)     GNET_SOCKADDR_FAMILY((i)->sa)
#define GNET_INETADDR_ADDRP(i)      GNET_SOCKADDR_ADDRP((i)->sa)
#define GNET_INETADDR_ADDR32(i,n)   GNET_SOCKADDR_ADDR32((i)->sa,(n))
#define GNET_INETADDR_ADDRLEN(i)    GNET_SOCKADDR_ADDRLEN((i)->sa)
#define GNET_INETADDR_PORT(i)       GNET_SOCKADDR_PORT((i)->sa)
#define GNET_INETADDR_PORT_SET(i, p)	GNET_SOCKADDR_PORT_SET((i)->sa, p)
#define GNET_INETADDR_LEN(i)        GNET_SOCKADDR_LEN((i)->sa)
#define GNET_INETADDR_SET_SS_LEN(i) GNET_SOCKADDR_SET_SS_LEN((i)->sa)


#define GNET_ANY_IO_CONDITION   (G_IO_IN|G_IO_OUT|G_IO_PRI|G_IO_ERR|G_IO_HUP|G_IO_NVAL)

G_BEGIN_DECLS


/*

   This is the private declaration and definition file.  The things in
   here are to be used by GNet ONLY.  Use at your own risk.  If this
   file includes something that you absolutely must have, write the
   GNet developers.

*/

#define GNET_UDP_SOCKET_TYPE_COOKIE    71254329
#define GNET_MCAST_SOCKET_TYPE_COOKIE  49712423

struct _GUdpSocket
{
  guint type;
  SOCKET sockfd;
  gint ref_count;
  GIOChannel* iochannel;
  struct sockaddr_storage sa;
};

struct _GMcastSocket
{
  GUdpSocket udpsocket;
};

struct _GTcpSocket
{
  SOCKET sockfd;
  gint ref_count; /* ATOMIC */
  GIOChannel* iochannel;
  struct sockaddr_storage sa;
  /* sa is remote host for clients, local host for servers */

  GTcpSocketAcceptFunc accept_func;
  gpointer accept_data;
  guint	accept_watch;
};

struct _GInetAddr
{
  gchar* name;
  gint ref_count; /* ATOMIC */
  struct sockaddr_storage sa;
};

/* **************************************** 	*/
/* More Windows specific stuff 			*/

#ifdef GNET_WIN32

/* Name-mangled IPv6 structures for primarily Mingw and a few VC .NET 2002 */
#ifndef MAX_ADAPTER_ADDRESS_LENGTH
#define MAX_ADAPTER_ADDRESS_LENGTH 8
#endif 

#ifndef GAA_FLAG_SKIP_UNICAST
#define GAA_FLAG_SKIP_UNICAST      0x0001
#endif

#ifndef GAA_FLAG_SKIP_ANYCAST
#define GAA_FLAG_SKIP_ANYCAST      0x0002
#endif

#ifndef GAA_FLAG_SKIP_MULTICAST
#define GAA_FLAG_SKIP_MULTICAST    0x0004
#endif

#ifndef GAA_FLAG_SKIP_DNS_SERVER
#define GAA_FLAG_SKIP_DNS_SERVER   0x0008
#endif

/* VC .NET 2002 is missing this too. */
#ifndef GAA_FLAG_INCLUDE_PREFIX
#define GAA_FLAG_INCLUDE_PREFIX     0x0010
#endif

/* VC .NET 2002 is missing this too. */
#ifndef GAA_FLAG_SKIP_FRIENDLY_NAME
#define GAA_FLAG_SKIP_FRIENDLY_NAME 0x0020
#endif

#ifndef IPV6_UNICAST_HOPS
#define IPV6_UNICAST_HOPS			4
#endif

#ifndef IPV6_MULTICAST_LOOP
#define IPV6_MULTICAST_LOOP			11
#endif

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP			12
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP		13
#endif

#ifndef IPV6_JOIN_GROUP
#define IPV6_JOIN_GROUP				IPV6_ADD_MEMBERSHIP
#endif

#ifndef IPV6_LEAVE_GROUP
#define IPV6_LEAVE_GROUP			IPV6_DROP_MEMBERSHIP
#endif

#ifndef IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_HOPS			10 
#endif

typedef enum 
{
  IpDadStateInvalid_GNET = 0, 
  IpDadStateTentative_GNET, 
  IpDadStateDuplicate_GNET, 
  IpDadStateDeprecated_GNET, 
  IpDadStatePreferred_GNET
} IP_DAD_STATE_GNET;

typedef enum 
{
  IpPrefixOriginOther_GNET = 0, 
  IpPrefixOriginManual_GNET, 
  IpPrefixOriginWellKnown_GNET, 
  IpPrefixOriginDhcp_GNET, 
  IpPrefixOriginRouterAdvertisement_GNET
} IP_PREFIX_ORIGIN_GNET;

typedef enum 
{
  IpSuffixOriginOther_GNET = 0, 
  IpSuffixOriginManual_GNET, 
  IpSuffixOriginWellKnown_GNET, 
  IpSuffixOriginDhcp_GNET, 
  IpSuffixOriginLinkLayerAddress_GNET, 
  IpSuffixOriginRandom_GNET
} IP_SUFFIX_ORIGIN_GNET;

typedef enum 
{
  IfOperStatusUp_GNET = 1, 
  IfOperStatusDown_GNET, 
  IfOperStatusTesting_GNET, 
  IfOperStatusUnknown_GNET, 
  IfOperStatusDormant_GNET, 
  IfOperStatusNotPresent_GNET, 
  IfOperStatusLowerLayerDown_GNET
} IF_OPER_STATUS_GNET;

typedef struct _IP_ADAPTER_UNICAST_ADDRESS_GNET 
{  
	union 
	{    
		struct 
		{      
			ULONG Length;      
			DWORD Flags;    
		};  
	};  
	struct _IP_ADAPTER_UNICAST_ADDRESS_GNET* Next;  
	SOCKET_ADDRESS Address;  
	IP_PREFIX_ORIGIN_GNET PrefixOrigin;  
	IP_SUFFIX_ORIGIN_GNET SuffixOrigin;  
	IP_DAD_STATE_GNET DadState;  
	ULONG ValidLifetime;  
	ULONG PreferredLifetime;  
	ULONG LeaseLifetime;
} IP_ADAPTER_UNICAST_ADDRESS_GNET, *PIP_ADAPTER_UNICAST_ADDRESS_GNET;

typedef struct _IP_ADAPTER_ANYCAST_ADDRESS_GNET 
{  
	union 
	{    
		ULONGLONG Alignment;    
		struct 
		{      
			ULONG Length;      
			DWORD Flags;    
		};  
	};  
	struct _IP_ADAPTER_ANYCAST_ADDRESS_GNET* Next;  
	SOCKET_ADDRESS Address;
} IP_ADAPTER_ANYCAST_ADDRESS_GNET, *PIP_ADAPTER_ANYCAST_ADDRESS_GNET;

typedef struct _IP_ADAPTER_DNS_SERVER_ADDRESS_GNET
{  
	union 
	{    
		ULONGLONG Alignment;    
		struct 
		{      
			ULONG Length;      
			DWORD Reserved;    
		};  
	};  
	struct _IP_ADAPTER_DNS_SERVER_ADDRESS_GNET* Next;  
	SOCKET_ADDRESS Address;
} IP_ADAPTER_DNS_SERVER_ADDRESS_GNET, *PIP_ADAPTER_DNS_SERVER_ADDRESS_GNET;

typedef struct _IP_ADAPTER_PREFIX_GNET 
{  
	union 
	{    
		ULONGLONG Alignment;    
		struct 
		{      
			ULONG Length;      
			DWORD Flags;    
		};  
	};  
	struct _IP_ADAPTER_PREFIX_GNET* Next;  
	SOCKET_ADDRESS Address;  ULONG PrefixLength;
} IP_ADAPTER_PREFIX_GNET, *PIP_ADAPTER_PREFIX_GNET;

typedef struct _IP_ADAPTER_MULTICAST_ADDRESS_GNET {
  union {
    ULONGLONG  _temp;
    struct {
      ULONG Length;
      DWORD Flags;
    };
  };
  struct _IP_ADAPTER_MULTICAST_ADDRESS_GNET* Next;
  SOCKET_ADDRESS Address;
} IP_ADAPTER_MULTICAST_ADDRESS_GNET, *PIP_ADAPTER_MULTICAST_ADDRESS_GNET;

typedef struct _IP_ADAPTER_ADDRESSES_GNET 
{  
	union 
	{    
		ULONGLONG Alignment;    
		struct 
		{      
			ULONG Length;      
			DWORD IfIndex;    
		};  
	};  
	struct _IP_ADAPTER_ADDRESSES_GNET* Next;  
	PCHAR AdapterName;  
	PIP_ADAPTER_UNICAST_ADDRESS_GNET FirstUnicastAddress;  
	PIP_ADAPTER_ANYCAST_ADDRESS_GNET FirstAnycastAddress;  
	PIP_ADAPTER_MULTICAST_ADDRESS_GNET FirstMulticastAddress;  
	PIP_ADAPTER_DNS_SERVER_ADDRESS_GNET FirstDnsServerAddress;  
	PWCHAR DnsSuffix;  
	PWCHAR Description;  
	PWCHAR FriendlyName;  
	BYTE PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];  
	DWORD PhysicalAddressLength;  
	DWORD Flags;  
	DWORD Mtu;  
	DWORD IfType;  
	IF_OPER_STATUS_GNET OperStatus;  
	DWORD Ipv6IfIndex;  
	DWORD ZoneIndices[16];  
	PIP_ADAPTER_PREFIX_GNET FirstPrefix;
} IP_ADAPTER_ADDRESSES_GNET, *PIP_ADAPTER_ADDRESSES_GNET;
/* Name-mangled IPv6 structures */

/* Function Pointers to the new IPv6 functions */
HANDLE hLibrary_ws2_32; /* WinSock 2 dll */
HANDLE hLibrary_Iphlpapi; /* for GetAdaptersAddresses() */

typedef int (WINAPI * PFN_GETADDRINFO) (const char*, const char*, 
										const struct addrinfo FAR *, 
										struct addrinfo FAR *FAR *);
PFN_GETADDRINFO pfn_getaddrinfo;

typedef int (WINAPI * PFN_GETNAMEINFO) (const struct sockaddr FAR *, 
										socklen_t, char FAR *, DWORD, 
										char FAR *, DWORD, int);
PFN_GETNAMEINFO pfn_getnameinfo;

typedef void (WINAPI * PFN_FREEADDRINFO) (struct addrinfo FAR *);
PFN_FREEADDRINFO pfn_freeaddrinfo;

typedef DWORD (WINAPI * PFN_GETADAPTERSADDRESSES) (ULONG, DWORD, PVOID,
											   PIP_ADAPTER_ADDRESSES_GNET,
											   PULONG);
PFN_GETADAPTERSADDRESSES pfn_getaddaptersaddresses;


#endif



/* ************************************************************ */

/* Private/Experimental functions */

GIOChannel* _gnet_io_channel_new (SOCKET sockfd);

SOCKET _gnet_create_listen_socket (int type, const GInetAddr* iface, int port, struct sockaddr_storage* sa);

int gnet_initialize_windows_sockets(void);
void gnet_uninitialize_windows_sockets(void);

/* Private utility functions */

guint   _gnet_idle_add_full     (GMainContext  * context,
                                 gint            priority,
                                 GSourceFunc     function,
                                 gpointer        data,
                                 GDestroyNotify  notify);

guint   _gnet_timeout_add_full  (GMainContext  * context,
                                 gint            priority,
                                 guint           interval,
                                 GSourceFunc     function,
                                 gpointer        data,
                                 GDestroyNotify  notify);

guint   _gnet_io_watch_add_full (GMainContext  * context,
                                 gint            priority,
                                 GIOChannel    * channel,
                                 GIOCondition    condition,
                                 GIOFunc         function,
                                 gpointer        data,
                                 GDestroyNotify  notify);

/* will do nothing if source_id is 0 (unlike g_source_remove()) */
void    _gnet_source_remove     (GMainContext * context, guint source_id);

G_END_DECLS

#endif /* _GNET_PRIVATE_H */
