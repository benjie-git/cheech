/*
 *  In-process local-server registry for the iOS GNet replacement.
 */

#ifdef CHEECH_IOS

#include "cheech_ios_gnet_private.hh"

#include <map>

#include "gnet_server.hh"

namespace cheech {
namespace ios_gnet {

namespace {
	std::map<unsigned int, Gnet::Server*>& servers()
	{
		static std::map<unsigned int, Gnet::Server*> registry;
		return registry;
	}
}

void register_local_server(unsigned int port, Gnet::Server* server)
{
	servers()[port] = server;
}

void unregister_local_server(unsigned int port, Gnet::Server* server)
{
	auto it = servers().find(port);
	if (it != servers().end() && it->second == server)
		servers().erase(it);
}

Gnet::Server* find_local_server(unsigned int port)
{
	auto it = servers().find(port);
	return it == servers().end() ? nullptr : it->second;
}

}
}

#endif /* CHEECH_IOS */
