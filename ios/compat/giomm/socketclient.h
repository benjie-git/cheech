/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_SOCKETCLIENT_H
#define INCL_COMPAT_GIOMM_SOCKETCLIENT_H

#include <memory>

#include <glib.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <giomm/asyncresult.h>
#include <giomm/socketconnection.h>

namespace Gio {

class SocketClient : public Glib::Object
{
public:
	static Glib::RefPtr<SocketClient> create();
	virtual ~SocketClient();

	void connect_to_host_async(const Glib::ustring& host_and_port,
		guint16 default_port, const SlotAsyncReady& slot);
	Glib::RefPtr<SocketConnection> connect_to_host_finish(
		const Glib::RefPtr<AsyncResult>& result);

private:
	struct ConnectState;
	std::shared_ptr<ConnectState> _connect;

	void cancel_pending();
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_SOCKETCLIENT_H
