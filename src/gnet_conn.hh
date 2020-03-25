/*
 *  Implements a simple socket handling reading, writing, and errors.
 *  To actually listen or connect, use Gnet::Server or Gnet::Client subclasses.
 *  Uses the GNet library.
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


#ifndef INCL_GNET_CONN_HH
#define INCL_GNET_CONN_HH

#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>
#include <gnet-2.0/gnet.h>


namespace Gnet {
	class Conn;
	class ConnBuffered;
}

class Gnet::Conn : public sigc::trackable
{
	public:
		enum Error { errAddressLookup, errNoTCPConnection, errConnectionBroken,
					 errNoConnection, errIO };
		enum Status { statIdle, statConnecting, statConnected };
		
		const static unsigned int DEFAULT_PACKET_SIZE = 1024;
	
		Conn();
		Conn(GConn* gconn);
		virtual ~Conn();
	
		void connect(const Glib::ustring& host, unsigned int port);
		Status get_status() const;
		Glib::ustring get_host_name() const;
		unsigned int get_port() const;
		virtual bool get_buffered() const;
		void close();

		// Number of times evt_data_available has been called
		unsigned int get_read_count() const;

		sigc::signal<void>					evt_connected;
		sigc::signal<void>					evt_cancelled;
		sigc::signal<void, Glib::ustring>	evt_data_available;
		sigc::signal<void>					evt_closed;
		sigc::signal<void, Error>			evt_error;   
	
		Conn& operator<<(const Glib::ustring& data);
		Conn& write(gchar *data, unsigned long count);
	
	protected:
		virtual void do_read();
		static void handle_event_static(GConn *conn, GConnEvent *event,
										gpointer data);
		void handle_event(GConn *conn, GConnEvent *event);
	
		GConn*						_conn;
		Status						_status;
		unsigned int				_read_count;
};


class Gnet::ConnBuffered
	: public Gnet::Conn
{
	public:
		ConnBuffered();
		ConnBuffered(GConn* gconn);
		virtual bool get_buffered() const;
	protected:
		virtual void do_read();
};

#endif   // #ifndef INCL_GNET_CONN_HH
