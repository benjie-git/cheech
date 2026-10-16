/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_SOCKETCONNECTION_H
#define INCL_COMPAT_GIOMM_SOCKETCONNECTION_H

#include <memory>

#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <giomm/socket.h>
#include <giomm/socketaddress.h>
#include <giomm/inetsocketaddress.h>
#include <giomm/inputstream.h>
#include <giomm/outputstream.h>
#include <giomm/giomm-internal.hh>

namespace Gio {

class SocketConnection : public Glib::Object
{
public:
	explicit SocketConnection(const std::shared_ptr<internal::Connection>& conn);
	virtual ~SocketConnection();

	Glib::RefPtr<Socket> get_socket();
	Glib::RefPtr<SocketAddress> get_remote_address();
	Glib::RefPtr<InputStream> get_input_stream();
	Glib::RefPtr<OutputStream> get_output_stream();
	void close();

private:
	std::shared_ptr<internal::Connection> _conn;
	Glib::RefPtr<InputStream> _input;
	Glib::RefPtr<OutputStream> _output;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_SOCKETCONNECTION_H
