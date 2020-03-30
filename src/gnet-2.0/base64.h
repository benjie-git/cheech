/* GNet Base 64 module
 * Copyright (C) 2003 Free Software Foundation
 * Created by Alfred Reibenschuh <alfredreibenschuh@gmx.net>,
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


#ifndef _GNET_BASE64_H
#define _GNET_BASE64_H

#include <glib.h>

G_BEGIN_DECLS

gchar * gnet_base64_encode (const gchar * src, gint srclen, gint * dstlenp, gboolean strict);

gchar * gnet_base64_decode (const gchar * src, gint srclen, gint * dstlenp);

G_END_DECLS

#endif /* _GNET_BASE64_H */
