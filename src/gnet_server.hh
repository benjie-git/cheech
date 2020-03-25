/*
 *  Implements a server socket with an aditional signal for
 *  incoming connections, using the GNet library.
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


#ifndef INCL_SOCKET_SERVER_HH
#define INCL_SOCKET_SERVER_HH

#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>
#include <gnet-2.0/gnet.h>

#include "gnet_conn.hh"


namespace Gnet {
		class Server;
}

class Gnet::Server : public sigc::trackable
{
public:
	Server();
	virtual ~Server();

	sigc::signal<void, Conn*>			evt_connection_available;
	sigc::signal<void>					evt_error;   

	bool listen(unsigned int port, bool buffered = false);
	bool ready() const;
	void close();

private:
	static void handle_accept_static(GServer* server, GConn* client, gpointer data);
	void handle_accept(GServer* server, GConn* client);

	GServer*		_server;
	bool			_buffered;
};

#endif   // #ifndef INCL_SOCKET_SERVER_HH
