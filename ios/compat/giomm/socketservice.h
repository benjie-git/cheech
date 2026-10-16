/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_COMPAT_GIOMM_SOCKETSERVICE_H
#define INCL_COMPAT_GIOMM_SOCKETSERVICE_H

#include <glib.h>
#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <glibmm/signalproxy.h>
#include <giomm/socketconnection.h>

namespace Gio {

class SocketService : public Glib::Object
{
public:
	static Glib::RefPtr<SocketService> create();
	virtual ~SocketService();

	bool add_inet_port(guint16 port);
	void start();
	void stop();
	bool is_active() const;
	void close();

	Glib::SignalProxy<bool(const Glib::RefPtr<SocketConnection>&,
		const Glib::RefPtr<Glib::Object>&)> signal_incoming();

	// Internal: adopts a connection established in-process (a socketpair
	// created by SocketClient for a loopback host); ownership of fd transfers
	// to the service.
	void accept_local(int fd, const Glib::ustring& hostname, guint16 port);

private:
	void on_accept();

	int _fd = -1;
	int _watch = -1;
	guint16 _port = 0;
	bool _active = false;
	bool _listening = false;

	Glib::SignalProxy<bool(const Glib::RefPtr<SocketConnection>&,
		const Glib::RefPtr<Glib::Object>&)> _incoming;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_SOCKETSERVICE_H
