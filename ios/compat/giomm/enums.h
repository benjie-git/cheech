#ifndef INCL_COMPAT_GIOMM_ENUMS_H
#define INCL_COMPAT_GIOMM_ENUMS_H

namespace Gio {

enum class SocketFamily
{
	INVALID,
	UNIX,
	IPV4,
	IPV6
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_ENUMS_H
