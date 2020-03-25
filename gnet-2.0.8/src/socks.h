/* GNet - Networking library
 * Copyright (C) 2001-2002  David Helder
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

#ifndef _GNET_SOCKS_H
#define _GNET_SOCKS_H

/* 
   This module is experimental, buggy, and unstable.  Use at your own
   risk.  To use this module, define GNET_EXPERIMENTAL before
   including gnet.h.
*/
#ifdef GNET_EXPERIMENTAL 

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *  GNET_SOCKS_PORT
 *
 *  Default port for the SOCKS protocol.
 **/
#define GNET_SOCKS_PORT 1080


/**
 *  GNET_SOCKS_VERSION
 *  
 *  Default version of the SOCKS protocol.
 **/
#define GNET_SOCKS_VERSION 5



gboolean   gnet_socks_get_enabled (void);
void	   gnet_socks_set_enabled (gboolean enabled);

GInetAddr* gnet_socks_get_server (void);
void       gnet_socks_set_server (const GInetAddr* inetaddr);

gint	   gnet_socks_get_version (void);
void       gnet_socks_set_version (gint version);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* GNET_EXPERIMENTAL */

#endif /* _GNET_SOCKS_H */
