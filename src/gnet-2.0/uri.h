/* GNet - Networking library
 * Copyright (C) 2000-2001  David Helder, David Bolcsfoldi
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


#ifndef _GNET_URI_H
#define _GNET_URI_H

#include <glib.h>

G_BEGIN_DECLS

/**
 *  GURI:
 *  @scheme: Scheme (or protocol)
 *  @userinfo: User info
 *  @hostname: Host name
 *  @port: Port number
 *  @path: Path
 *  @query: Query
 *  @fragment: Fragment
 *
 *  The #GURI structure represents a URI.  All fields in this
 *  structure are publicly readable.
 *
 **/
typedef struct _GURI GURI;

struct _GURI
{
  gchar* scheme;
  gchar* userinfo;
  gchar* hostname;
  gint   port;
  gchar* path;
  gchar* query;
  gchar* fragment;
};



GURI*     gnet_uri_new (const gchar* uri);
GURI*     gnet_uri_new_fields (const gchar* scheme, const gchar* hostname, 
			       const gint port, const gchar* path);
GURI*	  gnet_uri_new_fields_all (const gchar* scheme, const gchar* userinfo, 
				   const gchar* hostname, const gint port, 
				   const gchar* path, 
				   const gchar* query, const gchar* fragment);
GURI*     gnet_uri_clone (const GURI* uri);
void      gnet_uri_delete (GURI* uri);
	       
gboolean  gnet_uri_equal (gconstpointer p1, gconstpointer p2);
guint     gnet_uri_hash (gconstpointer p);

void	  gnet_uri_escape (GURI* uri);
void	  gnet_uri_unescape (GURI* uri);

gchar* 	  gnet_uri_get_string (const GURI* uri);

void 	  gnet_uri_set_scheme   (GURI* uri, const gchar* scheme);
void 	  gnet_uri_set_userinfo	(GURI* uri, const gchar* userinfo);
void 	  gnet_uri_set_hostname (GURI* uri, const gchar* hostname);
void 	  gnet_uri_set_port     (GURI* uri, gint port);
void 	  gnet_uri_set_path	(GURI* uri, const gchar* path);
void 	  gnet_uri_set_query 	(GURI* uri, const gchar* query);
void 	  gnet_uri_set_fragment (GURI* uri, const gchar* fragment);

gboolean  gnet_uri_parse_inplace (GURI * guri, gchar * uri, gchar * hostname, gsize len);

G_END_DECLS

#endif /* _GNET_URI_H */
