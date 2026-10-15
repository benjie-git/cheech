/*
 *  Implements a simple socket handling reading, writing, and errors.
 *  To actually listen or connect, use Net::Server or Net::Client subclasses.
 *  Uses the GIO networking library.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 */


#ifndef INCL_NET_CONN_HH
#define INCL_NET_CONN_HH

#include <string>

#include <glib.h>
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <glibmm/bytes.h>
#include <glibmm/error.h>
#include <giomm/socketclient.h>
#include <giomm/socketconnection.h>
#include <giomm/inputstream.h>
#include <giomm/outputstream.h>
#include <giomm/asyncresult.h>
#include <giomm/socket.h>
#include <giomm/socketaddress.h>
#include <giomm/inetsocketaddress.h>
#include <giomm/inetaddress.h>
#include <sigc++/sigc++.h>


namespace Net {
	class Conn;
	class ConnBuffered;
}

class Net::Conn : public sigc::trackable
{
	public:
		enum Error { errAddressLookup, errNoTCPConnection, errConnectionBroken,
					 errNoConnection, errIO };
		enum Status { statIdle, statConnecting, statConnected };
		
		const static unsigned int DEFAULT_PACKET_SIZE = 1024;
	
		Conn();
		Conn(const Glib::RefPtr<Gio::SocketConnection>& connection);
		virtual ~Conn();
	
		void connect(const Glib::ustring& host, unsigned int port);
		Status get_status() const;
#if defined(CHEECH_IOS)
		// True for an in-process socketpair connection (no network stack).
		bool is_local() const;
#endif
		Glib::ustring get_host_name() const;
		unsigned int get_port() const;
		virtual bool get_buffered() const;
		void close();

		// Schedule this object to be deleted on the next main-loop iteration.
		// Use this instead of deleting the Conn from inside one of its own
		// signal emissions (close() emits evt_closed synchronously).
		void delete_later();

		// Invoked by delete_later(); do not call directly.
		void dispose();

		const static unsigned int MAX_BUFFER_SIZE = 64 * 1024;

		// Number of times evt_data_available has been called
		unsigned int get_read_count() const;

		sigc::signal<void()>					evt_connected;
		sigc::signal<void()>					evt_cancelled;
		sigc::signal<void(Glib::ustring)>	evt_data_available;
		sigc::signal<void()>					evt_closed;
		sigc::signal<void(Error)>			evt_error;   
	
		Conn& operator<<(const Glib::ustring& data);
		Conn& write(gchar *data, unsigned long count);
	
	protected:
		virtual void do_read();

		void adopt(const Glib::RefPtr<Gio::SocketConnection>& connection);
		void flush();
		void handle_data();

		void on_connect_finished(Glib::RefPtr<Gio::AsyncResult>& result);
		void on_read_finished(Glib::RefPtr<Gio::AsyncResult>& result);
		void on_write_finished(Glib::RefPtr<Gio::AsyncResult>& result);

		Glib::RefPtr<Gio::SocketClient>			_client;
		Glib::RefPtr<Gio::SocketConnection>		_connection;
		Glib::RefPtr<Gio::InputStream>			_input;
		Glib::RefPtr<Gio::OutputStream>			_output;
		Status						_status;
		unsigned int				_read_count;
		Glib::ustring				_host_name;
		unsigned int				_port;
		bool						_local;
		std::string					_read_buffer;
		std::string					_write_pending;
		std::string					_write_active;
		bool						_writing;
};


class Net::ConnBuffered
	: public Net::Conn
{
	public:
		ConnBuffered();
		ConnBuffered(const Glib::RefPtr<Gio::SocketConnection>& connection);
		virtual bool get_buffered() const;
};

#endif   // #ifndef INCL_NET_CONN_HH
