#ifndef INCL_COMPAT_GLIBMM_BYTES_H
#define INCL_COMPAT_GLIBMM_BYTES_H

#include <string>

#include <glib.h>
#include <glibmm/refptr.h>

namespace Glib {

class Bytes
{
public:
	static RefPtr<Bytes> create(gconstpointer data, gsize size)
	{
		RefPtr<Bytes> bytes(new Bytes());
		if (data && size > 0)
			bytes->_data.assign(static_cast<const char*>(data), size);
		return bytes;
	}

	gconstpointer get_data(gsize& size) const
	{
		size = _data.size();
		return _data.data();
	}

	gsize get_size() const
	{
		return _data.size();
	}

private:
	std::string _data;
};

} // namespace Glib

#endif   // #ifndef INCL_COMPAT_GLIBMM_BYTES_H
