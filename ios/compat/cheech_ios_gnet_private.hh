/*
 *  Internal connection/server state for the iOS GNet replacement.
 *  Included only by gnet_conn_ios.cc and gnet_server_ios.cc.
 */

#ifndef CHEECH_IOS_GNET_PRIVATE_HH
#define CHEECH_IOS_GNET_PRIVATE_HH

#include <functional>
#include <string>

#include "cheech_ios_gnet.hh"

namespace Gnet { class Server; }

struct GConn
{
	int fd = -1;
	int watch = -1;
	int port = 0;
	bool connecting = false;
	bool connected = false;
	bool local = false;
	bool want_read = false;
	bool want_write = false;
	std::string hostname;
	std::string readbuf;
	std::string writebuf;
	std::function<void(bool, bool, bool)> handler;
};

struct GServer
{
	int fd = -1;
	int watch = -1;
	unsigned int port = 0;
	bool buffered = false;
};

namespace cheech {
namespace ios_gnet {

// Registry of in-process Gnet::Server instances, keyed by listening port.
// Conn::connect() consults it for loopback hosts so a local client and the
// local server talk over a socketpair instead of the network stack.  This
// keeps hosted local games alive when iOS suspends the app and reclaims its
// network connections.
void register_local_server(unsigned int port, Gnet::Server* server);
void unregister_local_server(unsigned int port, Gnet::Server* server);
Gnet::Server* find_local_server(unsigned int port);

}
}

#endif /* CHEECH_IOS_GNET_PRIVATE_HH */
