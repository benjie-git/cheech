/*
 *  iOS (POSIX sockets + cheech::Loop) implementation of Gnet::Conn,
 *  replacing the GNet-based gnet_conn.cc for the iOS build.
 */

#ifdef CHEECH_IOS

#include "gnet_conn.hh"

#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "cheech_ios_gnet_private.hh"
#include "cheech_loop.hh"

namespace
{
	void set_nonblocking(int fd)
	{
		int flags = ::fcntl(fd, F_GETFL, 0);
		if (flags >= 0)
			::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}

	void set_socket_options(int fd)
	{
		int one = 1;
		::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
	}

	// Split complete lines out of buf. Terminators are '\n', '\r' or '\0',
	// with "\r\n" treated as a single terminator. The remainder stays in buf.
	void extract_lines(std::string& buf, std::vector<std::string>& out)
	{
		size_t start = 0;
		size_t i = 0;

		while (i < buf.size())
		{
			char ch = buf[i];

			if (ch == '\n' || ch == '\r' || ch == '\0')
			{
				out.push_back(buf.substr(start, i - start));

				if (ch == '\r' && i + 1 < buf.size() && buf[i + 1] == '\n')
					i += 2;
				else
					i += 1;

				start = i;
			}
			else
			{
				++i;
			}
		}

		buf.erase(0, start);
	}
}

Gnet::Conn::Conn()
	: _conn(nullptr), _status(statIdle), _read_count(0)
{
}

Gnet::Conn::Conn(GConn* gconn)
	: _conn(gconn), _status(statConnected), _read_count(0)
{
	do_read();
}

Gnet::Conn::~Conn()
{
	close();
}

void Gnet::Conn::connect(const Glib::ustring& host, unsigned int port)
{
	close();

	GConn* c = new GConn();
	c->hostname = host;
	c->port = static_cast<int>(port);

	_conn = c;
	_status = statConnecting;

	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo* result = nullptr;
	std::string port_str = std::to_string(port);

	if (::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0
		|| result == nullptr)
	{
		evt_error(errAddressLookup);
		close();
		return;
	}

	int fd = -1;
	int rc = -1;

	for (struct addrinfo* r = result; r != nullptr; r = r->ai_next)
	{
		fd = ::socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		if (fd < 0)
			continue;

		set_socket_options(fd);
		set_nonblocking(fd);

		rc = ::connect(fd, r->ai_addr, r->ai_addrlen);

		if (rc == 0 || errno == EINPROGRESS)
			break;

		::close(fd);
		fd = -1;
	}

	::freeaddrinfo(result);

	if (fd < 0)
	{
		evt_error(errNoTCPConnection);
		close();
		return;
	}

	c->fd = fd;

	if (rc == 0)
	{
		_status = statConnected;
		evt_connected();
		if (_status == statConnected)
			do_read();
		return;
	}

	// Connection in progress; wait until the socket becomes writable.
	c->connecting = true;
	c->want_write = true;
	c->handler = [this](bool, bool writable, bool error)
	{
		(void)writable;

		if (error)
		{
			evt_error(errNoTCPConnection);
			close();
			return;
		}

		if (!_conn)
			return;

		int soerr = 0;
		socklen_t len = sizeof(soerr);

		if (::getsockopt(_conn->fd, SOL_SOCKET, SO_ERROR, &soerr, &len) != 0
			|| soerr != 0)
		{
			evt_error(errNoTCPConnection);
			close();
			return;
		}

		_conn->connecting = false;
		_conn->want_write = false;
		_status = statConnected;

		evt_connected();
		if (_status == statConnected)
			do_read();
	};

	ensure_watch();
}

void Gnet::Conn::close()
{
	GConn* c = _conn;
	_conn = nullptr;

	if (c != nullptr)
	{
		if (c->watch >= 0)
			cheech::Loop::instance().remove_fd(c->watch);
		if (c->fd >= 0)
			::close(c->fd);

		delete c;
	}

	if (_status == statConnecting)
	{
		_status = statIdle;
		evt_cancelled();
	}
	else if (_status == statConnected)
	{
		_status = statIdle;
		evt_closed();
	}
}

void Gnet::Conn::do_read()
{
	if (_conn == nullptr || _status != statConnected)
		return;

	_conn->want_read = true;
	_conn->handler = [this](bool readable, bool writable, bool error)
	{
		handle_events(readable, writable, error);
	};

	ensure_watch();
}

void Gnet::Conn::ensure_watch()
{
	if (_conn == nullptr)
		return;

	GConn* c = _conn;

	if (c->watch < 0)
	{
		c->watch = cheech::Loop::instance().add_fd(
			c->fd, c->want_read, c->want_write,
			[c](bool readable, bool writable, bool error)
			{
				if (c->handler)
					c->handler(readable, writable, error);
			});
	}
	else
	{
		cheech::Loop::instance().mod_fd(c->watch, c->want_read, c->want_write);
	}
}

void Gnet::Conn::handle_events(bool readable, bool writable, bool error)
{
	if (_status != statConnected || _conn == nullptr)
		return;

	if (error)
	{
		evt_error(errIO);
		close();
		return;
	}

	GConn* c = _conn;

	if (writable)
	{
		flush();
		if (_status != statConnected || _conn != c)
			return;
	}

	if (!readable)
		return;

	char buffer[4096];
	bool eof = false;
	bool ioerr = false;

	while (true)
	{
		ssize_t n = ::recv(c->fd, buffer, sizeof(buffer), 0);

		if (n > 0)
		{
			c->readbuf.append(buffer, static_cast<size_t>(n));
			continue;
		}

		if (n == 0)
		{
			eof = true;
			break;
		}

		if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;

		if (errno == EINTR)
			continue;

		ioerr = true;
		break;
	}

	if (ioerr)
	{
		evt_error(errIO);
		close();
		return;
	}

	if (eof)
	{
		close();
		return;
	}

	if (c->readbuf.empty())
		return;

	std::vector<std::string> lines;

	if (get_buffered())
		extract_lines(c->readbuf, lines);
	else
	{
		lines.push_back(c->readbuf);
		c->readbuf.clear();
	}

	for (auto& line : lines)
	{
		if (_status != statConnected)
			return;

		evt_data_available(line);
		++_read_count;
	}
}

void Gnet::Conn::flush()
{
	if (_conn == nullptr || _status != statConnected)
		return;

	GConn* c = _conn;

	while (!c->writebuf.empty())
	{
		ssize_t n = ::send(c->fd, c->writebuf.data(), c->writebuf.size(), 0);

		if (n > 0)
		{
			c->writebuf.erase(0, static_cast<size_t>(n));
		}
		else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			break;
		}
		else if (n < 0 && errno == EINTR)
		{
			continue;
		}
		else
		{
			evt_error(errIO);
			close();
			return;
		}
	}

	c->want_write = !c->writebuf.empty();
	ensure_watch();
}

Gnet::Conn::Status Gnet::Conn::get_status() const
{
	return _status;
}

Glib::ustring Gnet::Conn::get_host_name() const
{
	if (_conn != nullptr)
		return Glib::ustring(_conn->hostname);

	return Glib::ustring();
}

unsigned int Gnet::Conn::get_port() const
{
	if (_conn != nullptr)
		return static_cast<unsigned int>(_conn->port);

	return 0;
}

unsigned int Gnet::Conn::get_read_count() const
{
	return _read_count;
}

bool Gnet::Conn::get_buffered() const
{
	return false;
}

Gnet::Conn& Gnet::Conn::operator<<(const Glib::ustring& data)
{
	if (_conn != nullptr && _status == statConnected)
		_conn->writebuf.append(data.c_str(), data.length());

	flush();
	return *this;
}

Gnet::Conn& Gnet::Conn::write(gchar* data, unsigned long count)
{
	if (_conn != nullptr && _status == statConnected)
		_conn->writebuf.append(data, count);

	flush();
	return *this;
}

Gnet::ConnBuffered::ConnBuffered()
	: Conn()
{
}

Gnet::ConnBuffered::ConnBuffered(GConn* gconn)
	: Conn()
{
	_conn = gconn;
	_status = statConnected;
	do_read();
}

void Gnet::ConnBuffered::do_read()
{
	Conn::do_read();
}

bool Gnet::ConnBuffered::get_buffered() const
{
	return true;
}

#endif /* CHEECH_IOS */
