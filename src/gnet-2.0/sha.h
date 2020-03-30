/*

   This code is free (LGPL compatible).  See the top of sha.c for
   details.

   Copyright (C) 1995  A.M. Kuchling [original author]
   Copyright (C) 2000  David Helder [GNet API]

 */


#ifndef _GNET_SHA_H
#define _GNET_SHA_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 *  GSHA
 *
 *  GSHA is a SHA hash.
 *
 **/
typedef struct _GSHA GSHA;

/**
 *  GNET_SHA_HASH_LENGTH
 *
 *  Length of the SHA hash in bytes.
 **/
#define GNET_SHA_HASH_LENGTH	20


GSHA*    gnet_sha_new (const gchar* buffer, guint length);
GSHA*	 gnet_sha_new_string (const gchar* str);
GSHA*    gnet_sha_clone (const GSHA* sha);
void     gnet_sha_delete (GSHA* sha);
	
GSHA*	 gnet_sha_new_incremental (void);
void	 gnet_sha_update (GSHA* sha, const gchar* buffer, guint length);
void	 gnet_sha_final (GSHA* sha);

gboolean gnet_sha_equal (gconstpointer p1, gconstpointer p2);
guint	 gnet_sha_hash (gconstpointer p);
	
gchar*   gnet_sha_get_digest (const GSHA* sha);
gchar*   gnet_sha_get_string (const GSHA* sha);
	
void	 gnet_sha_copy_string (const GSHA* sha, gchar* buffer);
	

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif /* _GNET_SHA_H */
