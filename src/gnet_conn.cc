/*
 *  Implements a simple socket handling reading, writing, and errors.
 *  To actually listen or connect, use Gnet::Server or Gnet::Client subclasses.
 *  Uses the GNet library.
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

#include <glibmm/ustring.h>
#include <iostream>

#include "gnet_conn.hh"

// #define DEBUG_SOCKET 1


using namespace std;

Gnet::Conn::Conn()
{
	_status = statIdle;
	_conn = NULL;
	_read_count = 0;
}


Gnet::ConnBuffered::ConnBuffered() : Conn()
{
}


Gnet::Conn::Conn(GConn* gconn)
{
	_conn = gconn;
	_status = statConnected;
	gnet_conn_set_callback(_conn, &Gnet::Conn::handle_event_static, this);
	do_read();
}


Gnet::ConnBuffered::ConnBuffered(GConn* gconn)
{
	_conn = gconn;
	_status = statConnected;
	gnet_conn_set_callback(_conn, &Gnet::ConnBuffered::handle_event_static, this);
	do_read();
}


Gnet::Conn::~Conn() 
{
	close(); 
}


void 
Gnet::Conn::connect(const Glib::ustring& host, unsigned int port)
{
	_conn = gnet_conn_new(host.c_str(), port, &Gnet::Conn::handle_event_static, this);
	gnet_conn_connect(_conn);
	_status = statConnecting;
}


Gnet::Conn::Status 
Gnet::Conn::get_status() const 
{
	return _status; 
}


Glib::ustring 
Gnet::Conn::get_host_name() const
{
	return (_conn) ? _conn->hostname : NULL;
}


unsigned int 
Gnet::Conn::get_port() const
{
	return (_conn) ? _conn->port : 0;
}


unsigned int 
Gnet::Conn::get_read_count() const
{
	return _read_count;
}


bool 
Gnet::Conn::get_buffered() const
{
	return false;
}


bool 
Gnet::ConnBuffered::get_buffered() const
{
	return true;
}


void 
Gnet::Conn::close()
{
	if (_conn)
	{
		gnet_conn_delete(_conn);
		_conn = NULL;
	}
	
	switch (_status)
	{
		case statConnecting:
			_status = statIdle;
			evt_cancelled();
			break;

		case statConnected:
			_status = statIdle;
			evt_closed();
			break;

		case statIdle:
			break;
	}
}


void
Gnet::Conn::do_read()
{
	if (_conn && _status == statConnected)
		gnet_conn_read(_conn);
}


void
Gnet::ConnBuffered::do_read()
{
	if (_conn && _status == statConnected)
		gnet_conn_readline(_conn);
}


void 
Gnet::Conn::handle_event_static(GConn *conn, GConnEvent *event, gpointer data) 
{
	((Conn*)data)->handle_event(conn, event);
}


void
Gnet::Conn::handle_event(GConn *conn, GConnEvent *event)
{
	switch (event->type) 
	{
		case GNET_CONN_CONNECT:
			_status = statConnected;
			evt_connected();
			do_read();
			return;

		case GNET_CONN_READ:
			if (event->length > 0)
			{
				evt_data_available(event->buffer);
				_read_count++;
			}
			do_read();
			return;

		case GNET_CONN_ERROR:
#ifdef DEBUG_SOCKET
			cerr << "* ERROR * " << "GNET_CONN_ERROR on socket." << endl;
#endif
			evt_error(errIO);
			break;

		case GNET_CONN_CLOSE:
			break;

		case GNET_CONN_WRITE:
			// Write succeeded
			return;

		case GNET_CONN_READABLE:
#ifdef DEBUG_SOCKET
			cerr << "* ERROR * " << "GNET_CONN_READABLE on socket." << endl;
#endif
			break;

		case GNET_CONN_WRITABLE:
#ifdef DEBUG_SOCKET
			cerr << "* ERROR * " << "GNET_CONN_WRITABLE on socket." << endl;
#endif
			break;

		case GNET_CONN_TIMEOUT:
#ifdef DEBUG_SOCKET
			cerr << "* ERROR * " << "GNET_CONN_TIMEOUT on socket." << endl;
#endif
			break;
	}

	close();
}


Gnet::Conn&
Gnet::Conn::operator<<(const Glib::ustring &data)
{
	if (_conn && _status == statConnected)
		gnet_conn_write(_conn, (char*)(data.c_str()), data.length());

	return *this;
}


Gnet::Conn&
Gnet::Conn::write(gchar *data, unsigned long count)
{
	if (_conn && _status == statConnected)
		gnet_conn_write(_conn, data, count);

	return *this;
}
