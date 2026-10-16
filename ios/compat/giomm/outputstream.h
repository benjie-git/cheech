/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_OUTPUTSTREAM_H
#define INCL_COMPAT_GIOMM_OUTPUTSTREAM_H

#include <memory>

#include <glib.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <giomm/asyncresult.h>
#include <giomm/giomm-internal.hh>

namespace Gio {

class OutputStream : public Glib::Object
{
public:
	explicit OutputStream(const std::shared_ptr<internal::Connection>& conn)
		: _conn(conn)
	{
	}

	void write_async(const void* buffer, gsize count, const SlotAsyncReady& slot,
		int io_priority = 0);
	gssize write_finish(const Glib::RefPtr<AsyncResult>& result);

private:
	std::shared_ptr<internal::Connection> _conn;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_OUTPUTSTREAM_H
