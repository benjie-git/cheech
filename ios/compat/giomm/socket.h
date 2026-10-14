#ifndef INCL_COMPAT_GIOMM_SOCKET_H
#define INCL_COMPAT_GIOMM_SOCKET_H

#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <giomm/enums.h>

namespace Gio {

class Socket : public Glib::Object
{
public:
	static Glib::RefPtr<Socket> create()
	{
		return Glib::RefPtr<Socket>(new Socket());
	}

	SocketFamily get_family() const { return _family; }
	void set_family(SocketFamily family) { _family = family; }

private:
	SocketFamily _family = SocketFamily::INVALID;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_SOCKET_H
