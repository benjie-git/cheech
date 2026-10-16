/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 *
 *  Minimal giomm networking shim for the iOS build of cheech.
 *
 *  Implements the small slice of Gio::SocketClient / SocketConnection /
 *  SocketService / InputStream / OutputStream that the shared networking
 *  wrappers (src/net_conn.*, src/net_server.*) use, on top of POSIX sockets
 *  and cheech::Loop.  This lets the same wrapper sources compile on both the
 *  desktop (real giomm) and iOS (this shim).
 *
 *  Loopback connections between two endpoints in the same process are made
 *  with an AF_UNIX socketpair instead of going through the network stack;
 *  such connections report is_local() == true.
 */

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <string>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cheech_loop.hh"

#include <glibmm/error.h>

#include "giomm/socketclient.h"
#include "giomm/socketservice.h"

namespace Gio {
namespace internal {

void set_nonblocking(int fd)
{
	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags >= 0)
		::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_socket_options(int fd)
{
#ifdef SO_NOSIGPIPE
	int yes = 1;
	::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
#else
	(void)fd;
#endif
}

bool is_loopback_host(const std::string& host)
{
	return (host == "127.0.0.1" || host == "localhost" || host == "::1");
}

std::string peer_hostname(const struct sockaddr* address, socklen_t length)
{
	char host[NI_MAXHOST];
	if (::getnameinfo(address, length, host, sizeof(host), NULL, 0,
			NI_NUMERICHOST) == 0)
		return std::string(host);
	return std::string();
}

static std::map<unsigned int, SocketService*>& service_map()
{
	static std::map<unsigned int, SocketService*> map;
	return map;
}

void register_service(unsigned int port, SocketService* service)
{
	service_map()[port] = service;
}

void unregister_service(unsigned int port)
{
	service_map().erase(port);
}

SocketService* find_service(unsigned int port)
{
	std::map<unsigned int, SocketService*>::iterator it = service_map().find(port);
	return (it == service_map().end()) ? NULL : it->second;
}

Connection::~Connection()
{
	shutdown();
}

void Connection::shutdown()
{
	if (closed)
		return;

	closed = true;
	reading = false;
	writing = false;
	read_slot = SlotAsyncReady();
	write_slot = SlotAsyncReady();

	if (watch >= 0)
	{
		cheech::Loop::instance().remove_fd(watch);
		watch = -1;
	}

	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

void Connection::update_watch()
{
	if (closed || fd < 0)
		return;

	bool want_readable = reading;
	bool want_writable = writing;

	if (!want_readable && !want_writable)
	{
		if (watch >= 0)
		{
			cheech::Loop::instance().remove_fd(watch);
			watch = -1;
		}
		return;
	}

	if (watch < 0)
	{
		std::shared_ptr<Connection> self = shared_from_this();
		watch = cheech::Loop::instance().add_fd(fd, want_readable, want_writable,
			[self](bool readable, bool writable, bool error)
			{
				self->on_events(readable, writable, error);
			});
	}
	else
	{
		cheech::Loop::instance().mod_fd(watch, want_readable, want_writable);
	}
}

void Connection::on_events(bool readable, bool writable, bool error)
{
	if (closed)
		return;

	if (error)
	{
		if (reading)
		{
			reading = false;
			read_failed = true;
			SlotAsyncReady slot = read_slot;
			read_slot = SlotAsyncReady();
			Glib::RefPtr<AsyncResult> result(new AsyncResult());
			result->failed = true;
			slot(result);
		}
		if (closed)
			return;
		if (writing)
		{
			writing = false;
			write_failed = true;
			SlotAsyncReady slot = write_slot;
			write_slot = SlotAsyncReady();
			Glib::RefPtr<AsyncResult> result(new AsyncResult());
			result->failed = true;
			slot(result);
		}
		return;
	}

	if (readable && reading)
		try_read();
	if (closed)
		return;

	if (writable && writing)
		try_write();
	if (closed)
		return;

	update_watch();
}

void Connection::try_read()
{
	if (!reading || closed)
		return;

	char buffer[65536];
	ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);

	if (n > 0)
	{
		read_data.assign(buffer, static_cast<size_t>(n));
		read_failed = false;
	}
	else if (n == 0)
	{
		// End of file: return an empty result; the caller closes the socket.
		read_data.clear();
		read_failed = false;
	}
	else
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		if (errno == EINTR)
			return;
		read_data.clear();
		read_failed = true;
	}

	reading = false;
	SlotAsyncReady slot = read_slot;
	read_slot = SlotAsyncReady();

	Glib::RefPtr<AsyncResult> result(new AsyncResult());
	result->failed = read_failed;
	slot(result);
}

void Connection::try_write()
{
	if (!writing || closed)
		return;

	while (write_offset < write_data.size())
	{
		ssize_t n = ::send(fd, write_data.data() + write_offset,
			write_data.size() - write_offset, 0);

		if (n > 0)
			write_offset += static_cast<size_t>(n);
		else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			return;
		else if (n < 0 && errno == EINTR)
			continue;
		else
		{
			write_failed = true;
			break;
		}
	}

	if (closed)
		return;

	write_total = write_failed ? 0 : static_cast<gsize>(write_offset);
	writing = false;
	SlotAsyncReady slot = write_slot;
	write_slot = SlotAsyncReady();

	Glib::RefPtr<AsyncResult> result(new AsyncResult());
	result->failed = write_failed;
	slot(result);
}

} // namespace internal

// ---------------------------------------------------------------------------
// InputStream / OutputStream
// ---------------------------------------------------------------------------

void InputStream::read_bytes_async(gsize count, const SlotAsyncReady& slot, int)
{
	(void)count;

	if (!_conn || _conn->closed)
		return;

	_conn->read_data.clear();
	_conn->read_failed = false;
	_conn->read_slot = slot;
	_conn->reading = true;
	_conn->update_watch();
}

Glib::RefPtr<Glib::Bytes> InputStream::read_bytes_finish(
	const Glib::RefPtr<AsyncResult>& result)
{
	if (result && result->failed)
		throw Glib::Error(result->message.empty() ? "read failed" : result->message);
	if (!_conn)
		throw Glib::Error("stream closed");

	return Glib::Bytes::create(_conn->read_data.data(), _conn->read_data.size());
}

void OutputStream::write_async(const void* buffer, gsize count,
	const SlotAsyncReady& slot, int)
{
	if (!_conn || _conn->closed)
		return;

	_conn->write_data.assign(static_cast<const char*>(buffer), count);
	_conn->write_offset = 0;
	_conn->write_total = 0;
	_conn->write_failed = false;
	_conn->write_slot = slot;
	_conn->writing = true;
	_conn->update_watch();
}

gssize OutputStream::write_finish(const Glib::RefPtr<AsyncResult>& result)
{
	if (result && result->failed)
		throw Glib::Error(result->message.empty() ? "write failed" : result->message);

	return static_cast<gssize>(_conn ? _conn->write_total : 0);
}

// ---------------------------------------------------------------------------
// SocketConnection
// ---------------------------------------------------------------------------

SocketConnection::SocketConnection(const std::shared_ptr<internal::Connection>& conn)
	: _conn(conn)
{
	_input = Glib::RefPtr<InputStream>(new InputStream(_conn));
	_output = Glib::RefPtr<OutputStream>(new OutputStream(_conn));
}

SocketConnection::~SocketConnection()
{
	if (_conn)
		_conn->shutdown();
}

Glib::RefPtr<Socket> SocketConnection::get_socket()
{
	Glib::RefPtr<Socket> socket = Socket::create();
	if (_conn)
		socket->set_family(_conn->family);
	return socket;
}

Glib::RefPtr<SocketAddress> SocketConnection::get_remote_address()
{
	if (!_conn || _conn->remote_host.empty())
		return Glib::RefPtr<SocketAddress>();

	return Glib::RefPtr<SocketAddress>(
		InetSocketAddress::create(_conn->remote_host, _conn->remote_port));
}

Glib::RefPtr<InputStream> SocketConnection::get_input_stream()
{
	return _input;
}

Glib::RefPtr<OutputStream> SocketConnection::get_output_stream()
{
	return _output;
}

void SocketConnection::close()
{
	if (_conn)
		_conn->shutdown();
}

// ---------------------------------------------------------------------------
// SocketClient
// ---------------------------------------------------------------------------

struct SocketClient::ConnectState
{
	int fd = -1;
	int watch = -1;
	bool done = false;
	bool failed = false;
	std::string host;
	guint16 port = 0;
	SocketFamily family = SocketFamily::INVALID;
	Glib::RefPtr<SocketConnection> result;
	SlotAsyncReady slot;
};

Glib::RefPtr<SocketClient> SocketClient::create()
{
	return Glib::RefPtr<SocketClient>(new SocketClient());
}

SocketClient::~SocketClient()
{
	cancel_pending();
}

void SocketClient::cancel_pending()
{
	if (!_connect)
		return;

	if (_connect->watch >= 0)
	{
		cheech::Loop::instance().remove_fd(_connect->watch);
		_connect->watch = -1;
	}

	if (_connect->fd >= 0)
	{
		::close(_connect->fd);
		_connect->fd = -1;
	}

	_connect.reset();
}

void SocketClient::connect_to_host_async(const Glib::ustring& host_and_port,
	guint16 default_port, const SlotAsyncReady& slot)
{
	cancel_pending();

	_connect = std::make_shared<ConnectState>();
	_connect->host = host_and_port;
	_connect->port = default_port;
	_connect->slot = slot;

	// In-process loopback: connect both ends with a socketpair when a
	// SocketService in this process is listening on the port.
	if (internal::is_loopback_host(host_and_port))
	{
		SocketService* service = internal::find_service(default_port);
		if (service)
		{
			int sv[2];
			if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0)
			{
				internal::set_socket_options(sv[0]);
				internal::set_nonblocking(sv[0]);

				std::shared_ptr<internal::Connection> conn =
					std::make_shared<internal::Connection>();
				conn->fd = sv[0];
				conn->local = true;
				conn->connected = true;
				conn->family = SocketFamily::UNIX;
				conn->remote_host = host_and_port;
				conn->remote_port = default_port;

				_connect->family = SocketFamily::UNIX;
				_connect->result =
					Glib::RefPtr<SocketConnection>(new SocketConnection(conn));
				_connect->done = true;

				service->accept_local(sv[1], host_and_port, default_port);

				std::shared_ptr<ConnectState> state = _connect;
				cheech::Loop::instance().post([state]()
				{
					Glib::RefPtr<AsyncResult> result(new AsyncResult());
					state->slot(result);
				});
				return;
			}
		}
	}

	// Resolve the host and start a non-blocking TCP connect.
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	char service[16];
	std::snprintf(service, sizeof(service), "%u",
		static_cast<unsigned int>(default_port));

	struct addrinfo* res = NULL;
	if (::getaddrinfo(host_and_port.c_str(), service, &hints, &res) != 0 || !res)
	{
		_connect->failed = true;
		_connect->done = true;
		std::shared_ptr<ConnectState> state = _connect;
		cheech::Loop::instance().post([state]()
		{
			Glib::RefPtr<AsyncResult> result(new AsyncResult());
			result->failed = true;
			state->slot(result);
		});
		return;
	}

	int fd = -1;
	SocketFamily family = SocketFamily::INVALID;

	for (struct addrinfo* ai = res; ai; ai = ai->ai_next)
	{
		fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0)
			continue;

		internal::set_socket_options(fd);
		internal::set_nonblocking(fd);

		if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0 || errno == EINPROGRESS)
		{
			family = (ai->ai_family == AF_INET6) ? SocketFamily::IPV6
												: SocketFamily::IPV4;
			break;
		}

		::close(fd);
		fd = -1;
	}

	::freeaddrinfo(res);

	if (fd < 0)
	{
		_connect->failed = true;
		_connect->done = true;
		std::shared_ptr<ConnectState> state = _connect;
		cheech::Loop::instance().post([state]()
		{
			Glib::RefPtr<AsyncResult> result(new AsyncResult());
			result->failed = true;
			state->slot(result);
		});
		return;
	}

	_connect->fd = fd;
	_connect->family = family;

	std::shared_ptr<ConnectState> state = _connect;
	_connect->watch = cheech::Loop::instance().add_fd(fd, false, true,
		[state](bool, bool writable, bool error)
		{
			if (!writable && !error)
				return;

			int so_error = 0;
			socklen_t length = sizeof(so_error);
			bool ok = !error &&
				::getsockopt(state->fd, SOL_SOCKET, SO_ERROR, &so_error, &length) == 0 &&
				so_error == 0;

			if (state->watch >= 0)
			{
				cheech::Loop::instance().remove_fd(state->watch);
				state->watch = -1;
			}

			Glib::RefPtr<AsyncResult> result(new AsyncResult());

			if (ok)
			{
				std::shared_ptr<internal::Connection> conn =
					std::make_shared<internal::Connection>();
				conn->fd = state->fd;
				conn->connected = true;
				conn->family = state->family;
				conn->remote_host = state->host;
				conn->remote_port = state->port;

				state->result =
					Glib::RefPtr<SocketConnection>(new SocketConnection(conn));
				state->done = true;
				state->fd = -1;
			}
			else
			{
				state->failed = true;
				state->done = true;
				::close(state->fd);
				state->fd = -1;
				result->failed = true;
			}

			state->slot(result);
		});
}

Glib::RefPtr<SocketConnection> SocketClient::connect_to_host_finish(
	const Glib::RefPtr<AsyncResult>& result)
{
	std::shared_ptr<ConnectState> state = _connect;
	_connect.reset();

	if (!state || state->failed || (result && result->failed) || !state->result)
		throw Glib::Error("failed to connect");

	return state->result;
}

// ---------------------------------------------------------------------------
// SocketService
// ---------------------------------------------------------------------------

Glib::RefPtr<SocketService> SocketService::create()
{
	return Glib::RefPtr<SocketService>(new SocketService());
}

SocketService::~SocketService()
{
	close();
}

bool SocketService::add_inet_port(guint16 port)
{
	if (_listening)
		return false;

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return false;

	int yes = 1;
	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	internal::set_socket_options(fd);
	internal::set_nonblocking(fd);

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	if (::bind(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) != 0 ||
		::listen(fd, 32) != 0)
	{
		::close(fd);
		return false;
	}

	_fd = fd;
	_port = port;
	_listening = true;
	internal::register_service(port, this);
	return true;
}

void SocketService::start()
{
	if (_listening && _watch < 0)
	{
		_watch = cheech::Loop::instance().add_fd(_fd, true, false,
			[this](bool, bool, bool)
			{
				on_accept();
			});
	}

	_active = true;
}

void SocketService::stop()
{
	_active = false;

	if (_watch >= 0)
	{
		cheech::Loop::instance().remove_fd(_watch);
		_watch = -1;
	}
}

bool SocketService::is_active() const
{
	return _active;
}

void SocketService::close()
{
	stop();

	if (_listening)
	{
		internal::unregister_service(_port);
		_listening = false;
	}

	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
}

Glib::SignalProxy<bool(const Glib::RefPtr<SocketConnection>&,
	const Glib::RefPtr<Glib::Object>&)> SocketService::signal_incoming()
{
	return _incoming;
}

void SocketService::on_accept()
{
	while (true)
	{
		struct sockaddr_storage address;
		socklen_t length = sizeof(address);
		int fd = ::accept(_fd, reinterpret_cast<struct sockaddr*>(&address), &length);

		if (fd < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}

		internal::set_socket_options(fd);
		internal::set_nonblocking(fd);

		std::shared_ptr<internal::Connection> conn =
			std::make_shared<internal::Connection>();
		conn->fd = fd;
		conn->connected = true;
		conn->family = (address.ss_family == AF_INET6) ? SocketFamily::IPV6
													   : SocketFamily::IPV4;
		conn->remote_host = internal::peer_hostname(
			reinterpret_cast<struct sockaddr*>(&address), length);

		Glib::RefPtr<SocketConnection> connection(new SocketConnection(conn));
		Glib::RefPtr<Glib::Object> source;
		_incoming.emit(connection, source);
	}
}

void SocketService::accept_local(int fd, const Glib::ustring& hostname, guint16 port)
{
	internal::set_socket_options(fd);
	internal::set_nonblocking(fd);

	std::shared_ptr<internal::Connection> conn =
		std::make_shared<internal::Connection>();
	conn->fd = fd;
	conn->local = true;
	conn->connected = true;
	conn->family = SocketFamily::UNIX;
	conn->remote_host = hostname;
	conn->remote_port = port;

	Glib::RefPtr<SocketConnection> connection(new SocketConnection(conn));
	Glib::RefPtr<Glib::Object> source;
	_incoming.emit(connection, source);
}

} // namespace Gio
