/*
 *  iOS (POSIX sockets + cheech::Loop) implementation of Gnet::Server,
 *  replacing the GNet-based gnet_server.cc for the iOS build.
 */

#ifdef CHEECH_IOS

#include "gnet_server.hh"

#include <cerrno>
#include <cstring>
#include <string>

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

	std::string peer_hostname(const struct sockaddr* addr, socklen_t len)
	{
		char host[NI_MAXHOST];
		host[0] = '\0';

		if (::getnameinfo(addr, len, host, sizeof(host), nullptr, 0,
						  NI_NUMERICHOST) != 0)
			return std::string();

		return std::string(host);
	}
}

Gnet::Server::Server()
	: _server(nullptr), _buffered(false)
{
}

Gnet::Server::~Server()
{
	close();
}

bool Gnet::Server::ready() const
{
	return _server != nullptr && _server->fd >= 0;
}

bool Gnet::Server::listen(unsigned int port, bool buffered)
{
	close();

	_buffered = buffered;

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return false;

	int one = 1;
	::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	set_socket_options(fd);
	set_nonblocking(fd);

	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(static_cast<uint16_t>(port));

	if (::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0
		|| ::listen(fd, 32) != 0)
	{
		::close(fd);
		return false;
	}

	_server = new GServer();
	_server->fd = fd;
	_server->port = port;
	_server->buffered = buffered;

	cheech::ios_gnet::register_local_server(port, this);

	_server->watch = cheech::Loop::instance().add_fd(
		fd, true, false,
		[this](bool readable, bool, bool error)
		{
			if (!readable && !error)
				return;

			while (_server != nullptr)
			{
				struct sockaddr_storage ss;
				socklen_t len = sizeof(ss);

				int cfd = ::accept(_server->fd,
								   reinterpret_cast<struct sockaddr*>(&ss),
								   &len);
				if (cfd < 0)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						break;
					if (errno == EINTR)
						continue;

					evt_error();
					break;
				}

				set_socket_options(cfd);
				set_nonblocking(cfd);

				GConn* g = new GConn();
				g->fd = cfd;
				g->connected = true;
				g->hostname = peer_hostname(
					reinterpret_cast<struct sockaddr*>(&ss), len);

				handle_accept(_server, g);
			}
		});

	return true;
}

void Gnet::Server::handle_accept(GServer* server, GConn* client)
{
	(void)server;

	if (client == nullptr)
	{
		evt_error();
		return;
	}

	Conn* conn = _buffered
		? static_cast<Conn*>(new ConnBuffered(client))
		: static_cast<Conn*>(new Conn(client));

	evt_connection_available(conn);
}

void Gnet::Server::accept_local(int fd, const Glib::ustring& hostname)
{
	if (_server == nullptr || fd < 0)
	{
		if (fd >= 0)
			::close(fd);
		return;
	}

	set_socket_options(fd);
	set_nonblocking(fd);

	GConn* g = new GConn();
	g->fd = fd;
	g->connected = true;
	g->local = true;
	g->port = static_cast<int>(_server->port);
	g->hostname = hostname;

	handle_accept(_server, g);
}

void Gnet::Server::close()
{
	if (_server == nullptr)
		return;

	cheech::ios_gnet::unregister_local_server(_server->port, this);

	if (_server->watch >= 0)
		cheech::Loop::instance().remove_fd(_server->watch);
	if (_server->fd >= 0)
		::close(_server->fd);

	delete _server;
	_server = nullptr;
}

#endif /* CHEECH_IOS */
