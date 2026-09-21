/*
 *  iOS replacement for the parts of the GNet public API that cheech's
 *  Gnet::Conn / Gnet::Server wrappers refer to. Only opaque handles are
 *  needed here; the actual connection state lives in
 *  cheech_ios_gnet_private.hh, which is only included by the iOS
 *  implementations in this directory.
 */

#ifndef CHEECH_IOS_GNET_HH
#define CHEECH_IOS_GNET_HH

#include <glib.h>

struct GConn;
struct GServer;

#endif /* CHEECH_IOS_GNET_HH */
