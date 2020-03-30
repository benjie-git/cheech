/* GNet - Networking library
 * Copyright (C) 2000  David Helder
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

#ifndef _GNET_IOCHANNEL_H
#define _GNET_IOCHANNEL_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


GIOError gnet_io_channel_writen (GIOChannel*   channel, 
				 gpointer      buffer, 
				 gsize         length,
				 gsize*        bytes_writtenp);

GIOError gnet_io_channel_readn (GIOChannel*    channel, 
				 gpointer      buffer, 
				 gsize         length,
				 gsize*        bytes_readp);

GIOError gnet_io_channel_readline (GIOChannel* channel, 
				   gchar*      buffer, 
				   gsize       length,
				   gsize*      bytes_readp);

GIOError gnet_io_channel_readline_strdup (GIOChannel*   channel, 
					  gchar**       bufferp, 
					  gsize*        bytes_readp);


#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_IOCHANNEL_H */
