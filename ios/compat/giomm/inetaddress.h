/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_INETADDRESS_H
#define INCL_COMPAT_GIOMM_INETADDRESS_H

#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

namespace Gio {

class InetAddress : public Glib::Object
{
public:
	static Glib::RefPtr<InetAddress> create(const Glib::ustring& string)
	{
		Glib::RefPtr<InetAddress> address(new InetAddress());
		address->_string = string;
		return address;
	}

	Glib::ustring to_string() const { return _string; }

private:
	Glib::ustring _string;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_INETADDRESS_H
