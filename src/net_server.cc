/*
 *   Implements a server socket with an aditional signal for
 *   incoming connections, using the GIO networking library.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include <iostream>

#include "net_server.hh"

// #define DEBUG_SOCKET 1


using namespace std;

Net::Server::Server() 
	:_buffered(false)
{
}


Net::Server::~Server() 
{
	close(); 
}


bool 
Net::Server::ready() const
{
	return (_service && _service->is_active()); 
}


bool
Net::Server::listen(unsigned int port, bool buffered)
{
	close();

	_buffered = buffered;

	_service = Gio::SocketService::create();

	if (!_service->add_inet_port(static_cast<guint16>(port)))
	{
		_service.reset();
		evt_error();
		return false;
	}

	_service->signal_incoming().connect(
		sigc::mem_fun(*this, &Net::Server::on_incoming), false);
	_service->start();

	return true;
}


void 
Net::Server::close()
{
	if (_service)
	{
		_service->stop();
		_service->close();
		_service.reset();
	}
}


bool
Net::Server::on_incoming(const Glib::RefPtr<Gio::SocketConnection>& connection,
						const Glib::RefPtr<Glib::Object>& source_object)
{
	(void)source_object;

	Conn* client_conn;

	if (_buffered)
		client_conn = new ConnBuffered(connection);
	else
		client_conn = new Conn(connection);

	evt_connection_available(client_conn);
	return true;
}
