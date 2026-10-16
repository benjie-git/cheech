/*
 *   Implements a server socket with an aditional signal for
 *   incoming connections, using the GIO networking library.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */


#ifndef INCL_NET_SERVER_HH
#define INCL_NET_SERVER_HH

#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <glibmm/object.h>
#include <giomm/socketconnection.h>
#include <giomm/socketservice.h>
#include <sigc++/sigc++.h>

#include "net_conn.hh"


namespace Net {
		class Server;
}

class Net::Server : public sigc::trackable
{
public:
	Server();
	virtual ~Server();

	sigc::signal<void(Conn*)>			evt_connection_available;
	sigc::signal<void()>					evt_error;   

	bool listen(unsigned int port, bool buffered = false);
	bool ready() const;
	void close();

private:
	bool on_incoming(const Glib::RefPtr<Gio::SocketConnection>& connection,
					const Glib::RefPtr<Glib::Object>& source_object);

	Glib::RefPtr<Gio::SocketService>		_service;
	bool			_buffered;
};

#endif   // #ifndef INCL_NET_SERVER_HH
