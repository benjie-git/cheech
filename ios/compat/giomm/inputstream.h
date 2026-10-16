/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_INPUTSTREAM_H
#define INCL_COMPAT_GIOMM_INPUTSTREAM_H

#include <memory>

#include <glib.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/bytes.h>
#include <giomm/asyncresult.h>
#include <giomm/giomm-internal.hh>

namespace Gio {

class InputStream : public Glib::Object
{
public:
	explicit InputStream(const std::shared_ptr<internal::Connection>& conn)
		: _conn(conn)
	{
	}

	void read_bytes_async(gsize count, const SlotAsyncReady& slot, int io_priority = 0);
	Glib::RefPtr<Glib::Bytes> read_bytes_finish(const Glib::RefPtr<AsyncResult>& result);

private:
	std::shared_ptr<internal::Connection> _conn;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_INPUTSTREAM_H
