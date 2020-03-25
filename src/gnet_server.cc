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

#include <iostream>

#include "gnet_server.hh"

// #define DEBUG_SOCKET 1


using namespace std;

Gnet::Server::Server() 
	:_server(NULL),
	 _buffered(false)
{
}


Gnet::Server::~Server() 
{
	close(); 
}


bool 
Gnet::Server::ready() const
{
	return (_server != NULL); 
}


bool
Gnet::Server::listen(unsigned int port, bool buffered)
{
	_buffered = buffered;
	_server = gnet_server_new(NULL, port, &Gnet::Server::handle_accept_static, this);
	if (!_server)
	{
		evt_error();
		return false;
	}

	return true;
}


void 
Gnet::Server::close()
{
	gnet_server_delete(_server);
}


void 
Gnet::Server::handle_accept_static(GServer* server, GConn* client, gpointer data) 
{
	((Server*)data)->handle_accept(server, client);
}


void
Gnet::Server::handle_accept(GServer* server, GConn* client)
{
	Conn* client_conn;

	if (client == 0) {
#ifdef DEBUG_SOCKET
		cerr << "* ERROR * " << "Server failed." << endl;
#endif
		evt_error();
		return;
	}

	if (_buffered)
		client_conn = new ConnBuffered(client);
	else
		client_conn = new Conn(client);

	evt_connection_available(client_conn);
}
