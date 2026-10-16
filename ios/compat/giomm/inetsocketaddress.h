/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_INETSOCKETADDRESS_H
#define INCL_COMPAT_GIOMM_INETSOCKETADDRESS_H

#include <glib.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <giomm/socketaddress.h>
#include <giomm/inetaddress.h>

namespace Gio {

class InetSocketAddress : public SocketAddress
{
public:
	static Glib::RefPtr<InetSocketAddress> create(const Glib::ustring& host, guint16 port)
	{
		Glib::RefPtr<InetSocketAddress> address(new InetSocketAddress());
		address->_address = InetAddress::create(host);
		address->_port = port;
		return address;
	}

	Glib::RefPtr<InetAddress> get_address() { return _address; }
	guint16 get_port() const { return _port; }

private:
	Glib::RefPtr<InetAddress> _address;
	guint16 _port = 0;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_INETSOCKETADDRESS_H
