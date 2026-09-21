/*
 *  Internal connection/server state for the iOS GNet replacement.
 *  Included only by gnet_conn_ios.cc and gnet_server_ios.cc.
 */

#ifndef CHEECH_IOS_GNET_PRIVATE_HH
#define CHEECH_IOS_GNET_PRIVATE_HH

#include <functional>
#include <string>

#include "cheech_ios_gnet.hh"

struct GConn
{
	int fd = -1;
	int watch = -1;
	int port = 0;
	bool connecting = false;
	bool connected = false;
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
	bool buffered = false;
};

#endif /* CHEECH_IOS_GNET_PRIVATE_HH */
