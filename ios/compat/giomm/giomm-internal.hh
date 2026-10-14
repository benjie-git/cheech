#ifndef INCL_COMPAT_GIOMM_INTERNAL_HH
#define INCL_COMPAT_GIOMM_INTERNAL_HH

#include <memory>
#include <string>

#include <sys/socket.h>

#include <glib.h>
#include <glibmm/refptr.h>
#include <giomm/enums.h>
#include <giomm/asyncresult.h>

namespace Gio {

class SocketService;

namespace internal {

void set_nonblocking(int fd);
void set_socket_options(int fd);
bool is_loopback_host(const std::string& host);
std::string peer_hostname(const struct sockaddr* address, socklen_t length);

void register_service(unsigned int port, SocketService* service);
void unregister_service(unsigned int port);
SocketService* find_service(unsigned int port);

// A single client/server connection, shared by the SocketConnection and its
// input/output streams.  Uses cheech::Loop to watch the socket and to
// complete one-shot async reads/writes.
struct Connection : public std::enable_shared_from_this<Connection>
{
	int fd = -1;
	bool closed = false;
	bool local = false;
	bool connected = false;
	SocketFamily family = SocketFamily::INVALID;
	std::string remote_host;
	guint16 remote_port = 0;
	int watch = -1;

	bool reading = false;
	SlotAsyncReady read_slot;
	std::string read_data;
	bool read_failed = false;

	bool writing = false;
	SlotAsyncReady write_slot;
	std::string write_data;
	size_t write_offset = 0;
	gsize write_total = 0;
	bool write_failed = false;

	~Connection();

	void shutdown();
	void update_watch();
	void on_events(bool readable, bool writable, bool error);
	void try_read();
	void try_write();
};

} // namespace internal
} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_INTERNAL_HH
