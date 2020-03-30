/* GNet - Networking library
 * Copyright (C) 2000, 2002  David Helder
 * Copyright (C) 2000  Andrew Lanoix
 * Copyright (C) 2007  Tim-Philipp Müller <tim centricular net>
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


#ifndef _GNET_INETADDR_H
#define _GNET_INETADDR_H

#include <glib.h>

G_BEGIN_DECLS

/**
 *  GInetAddr
 *
 *  GInetAddr is an internet address.
 *
 **/
typedef struct _GInetAddr GInetAddr;


/* ********** */

GInetAddr* gnet_inetaddr_new (const gchar* hostname, gint port);
GInetAddr* gnet_inetaddr_new_nonblock (const gchar* hostname, gint port);
GInetAddr* gnet_inetaddr_new_bytes (const gchar* bytes, const guint length);

GList*     gnet_inetaddr_new_list (const gchar* hostname, gint port);
void	   gnet_inetaddr_delete_list (GList* list);

GInetAddr* gnet_inetaddr_clone (const GInetAddr* inetaddr);

void       gnet_inetaddr_delete (GInetAddr* inetaddr);
void 	   gnet_inetaddr_ref (GInetAddr* inetaddr);
void 	   gnet_inetaddr_unref (GInetAddr* inetaddr);

gchar*     gnet_inetaddr_get_name (/* const */ GInetAddr* inetaddr);
gchar*     gnet_inetaddr_get_name_nonblock (GInetAddr* inetaddr);

gint	   gnet_inetaddr_get_length (const GInetAddr* inetaddr);
void	   gnet_inetaddr_get_bytes (const GInetAddr* inetaddr, gchar* buffer);
void	   gnet_inetaddr_set_bytes (GInetAddr* inetaddr, const gchar* bytes, gint length);

gchar*     gnet_inetaddr_get_canonical_name (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_canonical (const gchar* hostname);

gint 	   gnet_inetaddr_get_port (const GInetAddr* inetaddr);
void 	   gnet_inetaddr_set_port (const GInetAddr* inetaddr, gint port);
gboolean   gnet_inetaddr_is_internet  (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_private   (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_reserved  (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_loopback  (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_multicast (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_broadcast (const GInetAddr* inetaddr);

gboolean   gnet_inetaddr_is_ipv4      (const GInetAddr* inetaddr);
gboolean   gnet_inetaddr_is_ipv6      (const GInetAddr* inetaddr);


guint 	   gnet_inetaddr_hash (gconstpointer p);
gboolean   gnet_inetaddr_equal (gconstpointer p1, gconstpointer p2);
gboolean   gnet_inetaddr_noport_equal (gconstpointer p1, gconstpointer p2);

gchar*     gnet_inetaddr_get_host_name (void);
GInetAddr* gnet_inetaddr_get_host_addr (void);


GInetAddr* gnet_inetaddr_autodetect_internet_interface (void);
GInetAddr* gnet_inetaddr_get_interface_to (const GInetAddr* inetaddr);
GInetAddr* gnet_inetaddr_get_internet_interface (void);

gboolean   gnet_inetaddr_is_internet_domainname (const gchar* name);

GList*     gnet_inetaddr_list_interfaces (void);


/**
 *  GNET_INETADDR_MAX_LEN
 *
 *  Maximum length of a #GInetAddr's address in bytes.  This can be
 *  used to allocate a buffer large enough for
 *  gnet_inetaddr_get_bytes().
 *
 **/
#define GNET_INETADDR_MAX_LEN 16



/* **************************************** */
/* Asynchronous functions */


/**
 *   GInetAddrNewAsyncID:
 * 
 *   ID of an asynchronous GInetAddr creation/lookup started with
 *   gnet_inetaddr_new_async().  The creation can be canceled by
 *   calling gnet_inetaddr_new_async_cancel() with the ID.
 *
 **/
typedef struct _GInetAddrNewState * GInetAddrNewAsyncID;



/**
 *   GInetAddrNewAsyncFunc:
 *   @inetaddr: InetAddr that was looked up (callee owned)
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_async().  Callee owns the address.
 *   The address will be NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrNewAsyncFunc) (GInetAddr * inetaddr, gpointer data);


GInetAddrNewAsyncID  gnet_inetaddr_new_async      (const gchar          * hostname,
                                                   gint                   port,
                                                   GInetAddrNewAsyncFunc  func, 
                                                   gpointer               data);

GInetAddrNewAsyncID  gnet_inetaddr_new_async_full (const gchar          * hostname,
                                                   gint                   port,
                                                   GInetAddrNewAsyncFunc  func, 
                                                   gpointer               data,
                                                   GDestroyNotify         notify,
                                                   GMainContext         * context,
                                                   gint                   priority);

void                 gnet_inetaddr_new_async_cancel (GInetAddrNewAsyncID id);



/**
 *   GInetAddrNewListAsyncID:
 * 
 *   ID of an asynchronous GInetAddr list creation/lookup started with
 *   gnet_inetaddr_new_list_async().  The creation can be canceled by
 *   calling gnet_inetaddr_new_list_async_cancel() with the ID.
 *
 **/
typedef struct _GInetAddrNewListState * GInetAddrNewListAsyncID;



/**
 *   GInetAddrNewListAsyncFunc:
 *   @list: List of GInetAddr's (callee owned)
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_new_list_async().  Callee owns the
 *   list of GInetAddrs.  The list is NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrNewListAsyncFunc) (GList * list, gpointer data);

GInetAddrNewListAsyncID  gnet_inetaddr_new_list_async      (const gchar              * hostname,
                                                            gint                       port, 
                                                            GInetAddrNewListAsyncFunc  func, 
                                                            gpointer                   data);

GInetAddrNewListAsyncID  gnet_inetaddr_new_list_async_full (const gchar              * hostname,
                                                            gint                       port, 
                                                            GInetAddrNewListAsyncFunc  func, 
                                                            gpointer                   data,
                                                            GDestroyNotify             notify,
                                                            GMainContext             * context,
                                                            gint                       priority);

void                     gnet_inetaddr_new_list_async_cancel (GInetAddrNewListAsyncID id);



/**
 *   GInetAddrGetNameAsyncID:
 * 
 *   ID of an asynchronous InetAddr name lookup started with
 *   gnet_inetaddr_get_name_async().  The lookup can be canceled by
 *   calling gnet_inetaddr_get_name_async_cancel() with the ID.
 *
 **/
typedef struct _GInetAddrReverseAsyncState * GInetAddrGetNameAsyncID;



/**
 *   GInetAddrGetNameAsyncFunc:
 *   @hostname: Canonical name of the address (callee owned), NULL on failure
 *   @data: User data
 *   
 *   Callback for gnet_inetaddr_get_name_async().  Callee (that is: you) owns
 *   the name.  Free it with g_free() when no longer needed.  The name will be
 *   NULL if the lookup failed.
 *
 **/
typedef void (*GInetAddrGetNameAsyncFunc) (gchar * hostname, gpointer data);


GInetAddrGetNameAsyncID  gnet_inetaddr_get_name_async      (GInetAddr                 * inetaddr,
                                                            GInetAddrGetNameAsyncFunc   func,
                                                            gpointer                    data);

GInetAddrGetNameAsyncID  gnet_inetaddr_get_name_async_full (GInetAddr                 * inetaddr,
                                                            GInetAddrGetNameAsyncFunc   func,
                                                            gpointer                    data,
                                                            GDestroyNotify              notify,
                                                            GMainContext              * context,
                                                            gint                        priority);

void                     gnet_inetaddr_get_name_async_cancel (GInetAddrGetNameAsyncID id);

G_END_DECLS

#endif /* _GNET_INETADDR_H */
