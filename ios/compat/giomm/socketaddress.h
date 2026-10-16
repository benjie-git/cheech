/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_SOCKETADDRESS_H
#define INCL_COMPAT_GIOMM_SOCKETADDRESS_H

#include <glibmm/object.h>

namespace Gio {

class SocketAddress : public Glib::Object
{
public:
	virtual ~SocketAddress() = default;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_SOCKETADDRESS_H
