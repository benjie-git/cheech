/*
 *  Implements a server socket with an aditional signal for
 *  incoming connections, using the GIO networking library.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
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
