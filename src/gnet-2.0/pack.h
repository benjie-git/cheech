/* GNet - Networking library
 * Copyright (C) 2000, 2001  David Helder
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

#ifndef _GNET_PACK_H
#define _GNET_PACK_H

#include <glib.h>

G_BEGIN_DECLS


/* pack */

gint gnet_pack  (const gchar * format, gchar  * buffer, const gint length, ...);

gint gnet_vpack (const gchar * format, gchar  * buffer, const gint length, va_list args);

gint gnet_pack_strdup (const gchar * format, gchar ** bufferp, ...);

/* calculate size */

gint gnet_calcsize  (const gchar * format, ...);

gint gnet_vcalcsize (const gchar * format, va_list args);

/* unpack */

gint gnet_unpack  (const gchar * format, const gchar * buffer, gint length, ...);

gint gnet_vunpack (const gchar * format, const gchar * buffer, gint length, va_list args);


G_END_DECLS

#endif /* _GNET_PACK_H */
