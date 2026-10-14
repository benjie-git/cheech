/*
 *  Implements a simple socket handling reading, writing, and errors.
 *  To actually listen or connect, use Net::Server or Net::Client subclasses.
 *  Uses the GIO networking library.
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
#include <memory>
#include <iostream>
#include <vector>

#include "net_conn.hh"

// #define DEBUG_SOCKET 1


using namespace std;


static bool
is_line_terminator(char c)
{
	return (c == '\n' || c == '\r' || c == '\0');
}


// Split complete lines out of buffer, leaving any trailing partial line
// behind.  A "\r\n" pair counts as a single terminator.
static void
extract_lines(std::string& buffer, std::vector<std::string>& lines)
{
	size_t start = 0;
	size_t i = 0;

	while (i < buffer.size())
	{
		char c = buffer[i];

		if (is_line_terminator(c))
		{
			// A trailing '\r' may yet be followed by '\n'; wait for it.
			if (c == '\r' && (i + 1) == buffer.size())
				break;

			lines.push_back(buffer.substr(start, i - start));

			size_t next = i + 1;
			if (c == '\r' && next < buffer.size() && buffer[next] == '\n')
				next++;

			start = next;
			i = next;
		}
		else
			i++;
	}

	if (start > 0)
		buffer.erase(0, start);
}


Net::Conn::Conn()
{
	_status = statIdle;
	_read_count = 0;
	_port = 0;
	_local = false;
	_writing = false;
}


Net::ConnBuffered::ConnBuffered() : Conn()
{
}


Net::Conn::Conn(const Glib::RefPtr<Gio::SocketConnection>& connection)
{
	_status = statIdle;
	_read_count = 0;
	_port = 0;
	_local = false;
	_writing = false;

	adopt(connection);
	_status = statConnected;
	do_read();
}


Net::ConnBuffered::ConnBuffered(const Glib::RefPtr<Gio::SocketConnection>& connection)
	: Conn(connection)
{
}


Net::Conn::~Conn() 
{
	close(); 
}


void
Net::Conn::adopt(const Glib::RefPtr<Gio::SocketConnection>& connection)
{
	_connection = connection;

	if (!_connection)
		return;

	_input = _connection->get_input_stream();
	_output = _connection->get_output_stream();

	Glib::RefPtr<Gio::Socket> socket = _connection->get_socket();
	if (socket)
		_local = (socket->get_family() == Gio::SocketFamily::UNIX);

	if (_host_name.empty())
	{
		Glib::RefPtr<Gio::SocketAddress> remote = _connection->get_remote_address();
		Glib::RefPtr<Gio::InetSocketAddress> inet =
			std::dynamic_pointer_cast<Gio::InetSocketAddress>(remote);
		if (inet)
		{
			Glib::RefPtr<Gio::InetAddress> address = inet->get_address();
			if (address)
				_host_name = address->to_string();
		}
	}
}


void 
Net::Conn::connect(const Glib::ustring& host, unsigned int port)
{
	close();

	_host_name = host;
	_port = port;

	_client = Gio::SocketClient::create();
	_status = statConnecting;

	_client->connect_to_host_async(host, static_cast<guint16>(port),
		sigc::mem_fun(*this, &Net::Conn::on_connect_finished));
}


void
Net::Conn::on_connect_finished(Glib::RefPtr<Gio::AsyncResult>& result)
{
	if (_status != statConnecting)
		return;

	Glib::RefPtr<Gio::SocketConnection> connection;

	try
	{
		connection = _client->connect_to_host_finish(result);
	}
	catch (const Glib::Error&)
	{
		evt_error(errNoTCPConnection);
		close();
		return;
	}

	_client.reset();
	adopt(connection);

	if (!_connection)
	{
		evt_error(errNoTCPConnection);
		close();
		return;
	}

	_status = statConnected;
	evt_connected();
	do_read();
}


Net::Conn::Status 
Net::Conn::get_status() const 
{
	return _status; 
}


#if defined(CHEECH_IOS)
bool
Net::Conn::is_local() const
{
	return _local;
}
#endif


Glib::ustring 
Net::Conn::get_host_name() const
{
	return _host_name;
}


unsigned int 
Net::Conn::get_port() const
{
	return _port;
}


unsigned int 
Net::Conn::get_read_count() const
{
	return _read_count;
}


bool 
Net::Conn::get_buffered() const
{
	return false;
}


bool 
Net::ConnBuffered::get_buffered() const
{
	return true;
}


void 
Net::Conn::close()
{
	Glib::RefPtr<Gio::SocketConnection> connection = _connection;

	_connection.reset();
	_input.reset();
	_output.reset();
	_client.reset();

	_writing = false;
	_read_buffer.clear();
	_write_active.clear();
	_write_pending.clear();

	if (connection)
	{
		try
		{
			connection->close();
		}
		catch (const Glib::Error&)
		{
		}
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
Net::Conn::do_read()
{
	if (_input && _status == statConnected)
		_input->read_bytes_async(DEFAULT_PACKET_SIZE,
			sigc::mem_fun(*this, &Net::Conn::on_read_finished));
}


void
Net::Conn::on_read_finished(Glib::RefPtr<Gio::AsyncResult>& result)
{
	if (_status != statConnected || !_input)
		return;

	Glib::RefPtr<Glib::Bytes> bytes;

	try
	{
		bytes = _input->read_bytes_finish(result);
	}
	catch (const Glib::Error&)
	{
		evt_error(errIO);
		close();
		return;
	}

	gsize size = 0;
	const char* data = bytes ? static_cast<const char*>(bytes->get_data(size)) : NULL;

	// A zero-length read means the peer closed the connection.
	if (!data || size == 0)
	{
		close();
		return;
	}

	_read_buffer.append(data, size);
	handle_data();

	if (_status == statConnected)
		do_read();
}


void
Net::Conn::handle_data()
{
	std::vector<std::string> lines;

	if (get_buffered())
		extract_lines(_read_buffer, lines);
	else if (!_read_buffer.empty())
	{
		lines.push_back(_read_buffer);
		_read_buffer.clear();
	}

	for (size_t i = 0; i < lines.size(); i++)
	{
		if (_status != statConnected)
			return;

		evt_data_available(lines[i]);
		_read_count++;
	}
}


Net::Conn&
Net::Conn::operator<<(const Glib::ustring &data)
{
	if (_status == statConnected)
	{
		_write_pending.append(data.c_str(), data.length());
		flush();
	}

	return *this;
}


Net::Conn&
Net::Conn::write(gchar *data, unsigned long count)
{
	if (_status == statConnected)
	{
		_write_pending.append(data, count);
		flush();
	}

	return *this;
}


void
Net::Conn::flush()
{
	if (_writing || _write_pending.empty() || _status != statConnected || !_output)
		return;

	// Swap the pending data into the active buffer so the pointer passed to
	// the async write remains stable until the write completes.
	_write_active.swap(_write_pending);
	_writing = true;

	_output->write_async(_write_active.data(), _write_active.size(),
		sigc::mem_fun(*this, &Net::Conn::on_write_finished));
}


void
Net::Conn::on_write_finished(Glib::RefPtr<Gio::AsyncResult>& result)
{
	if (_status != statConnected || !_output)
	{
		_writing = false;
		_write_active.clear();
		return;
	}

	gssize written = 0;

	try
	{
		written = _output->write_finish(result);
	}
	catch (const Glib::Error&)
	{
		_writing = false;
		_write_active.clear();
		evt_error(errIO);
		close();
		return;
	}

	_writing = false;

	const size_t requested = _write_active.size();
	if (written < 0)
		written = 0;

	if (static_cast<size_t>(written) < requested)
		_write_pending.insert(0, _write_active.data() + written,
			requested - static_cast<size_t>(written));

	_write_active.clear();
	flush();
}
