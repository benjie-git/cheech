/*
 *  The Chinese Checkers AjaxServer listens for the Ajax-based web client
 *  to make requests, and then sends them off the the appropriate
 *  AjaxServerConn.
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

#include <iostream>
#include <sigc++/bind.h>
#include <sigc++/bind_return.h>
#include <glibmm/main.h>
#include <glibmm.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "ajax_server.hh"
#include "utility.hh"
#include "config.h"


AjaxServer::AjaxServer()
{
	_next_id = 1;
	_socket.evt_connection_available.connect(sigc::mem_fun(*this,
		&AjaxServer::connection_open));
}


AjaxServer::~AjaxServer()
{
}


bool AjaxServer::listen(unsigned int ajax_port,
		Glib::ustring cheechd_hostname, unsigned int cheechd_port)
{
	_cheechd_hostname = cheechd_hostname;
	_cheechd_port = cheechd_port;
	return _socket.listen(ajax_port, true);
}


void AjaxServer::connection_open(Gnet::Conn* socket)
{
	socket->evt_data_available.connect(sigc::bind(sigc::mem_fun(*this,
		&AjaxServer::handle_request), socket));
	socket->evt_closed.connect(sigc::bind(sigc::mem_fun(*this,
		&AjaxServer::connection_closed), socket));
}


void AjaxServer::connection_closed(Gnet::Conn* socket)
{
	// Tell all the clients that this connection closed.
	// (only the client that is using it will do anything...)
	for (std::list<AjaxServerConn>::iterator i = _clients.begin();
		i != _clients.end(); i++)
			(*i).on_connection_closed(socket);

	delete socket;
}


void AjaxServer::close()
{
	_socket.close();

	for (std::list<AjaxServerConn>::iterator i = _clients.begin();
		i != _clients.end(); i++)
			(*i).disconnect();
}


Glib::ustring AjaxServer::get_cheechd_hostname()
{
	return _cheechd_hostname;
}


unsigned int AjaxServer::get_cheechd_port()
{
	return _cheechd_port;
}


AjaxServerConn* AjaxServer::get_client_by_id(unsigned int id)
{
	for (std::list<AjaxServerConn>::iterator i = _clients.begin();
		i != _clients.end(); i++)
			if ((*i).get_id() == id)
				return &(*i);

	return NULL;
}


void AjaxServer::handle_request(Glib::ustring message, Gnet::Conn *conn)
{
	// We only care about the first line
	if (conn->get_read_count() > 0)
		return;

	int separator = message.find_first_of(" ");

	// If the first line is not a GET request, disconnect
	if (separator <= 0 ||
		(separator > 0 && message.substr(0, separator) != "GET"))
	{
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
			*conn, &Gnet::Conn::close), false), 100);
		return;
	}

	int separator2 = message.find_first_of(" ", separator+1);

	Glib::ustring resource = util::url_decode(
		message.substr(separator+1, separator2-separator-1));

	separator = resource.find_first_of("?");
	if (separator < 0)
	{
		if (resource == "/")
			resource = "/index.html";

		Glib::ustring filename = conv_filename(resource);

		// Send the requested file or send error
		send_file(filename, conn);
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
			*conn, &Gnet::Conn::close), false), 100);
	}
	else
	{
		(*conn) << "HTTP/1.0 200 OK\r\n";
		send_content_type("", conn);

		separator = resource.find_first_of("=", separator);
		Glib::ustring request =
			resource.substr(separator+1, resource.length()-separator-1);
		handle_ajax_message(request, conn);
	}
}


Glib::ustring AjaxServer::conv_filename(Glib::ustring f)
{
	Glib::ustring filename;
	unsigned int pos;

#ifdef WIN32
	char module_path[PATH_MAX];
	char *program_dir;

	GetModuleFileName(NULL, module_path, PATH_MAX);
	PathRemoveFileSpec(module_path);
	program_dir = g_locale_to_utf8(module_path, -1, NULL, NULL, NULL);
	filename = g_strdup_printf("%s\\cheechweb", program_dir) + f;

    while ((pos = filename.find("/", 0)) < filename.size())
		filename.replace(pos, 1, "\\");
#else
	filename = PACKAGE_DATA_DIR "/" PACKAGE "/cheechweb" + f;
#endif

	while ((pos = filename.find("..", 0)) < filename.size())
		filename.replace(pos, 2, "");

	return filename;
}


void AjaxServer::send_file(Glib::ustring f, Gnet::Conn *conn)
{
	Glib::RefPtr<Glib::IOChannel> pfile;

	try
	{
		try
		{
			pfile = Glib::IOChannel::create_from_file(f, "r");
		}
		catch (Glib::FileError)
		{
			(*conn) << "HTTP/1.0 404 Not Found\r\n";
			send_content_type("", conn);
			(*conn) << "<h1>404 Not Found</h1>\r\n";
			(*conn) << "reqested file not found.\n";
			return;
		}

		pfile->set_encoding();

		(*conn) << "HTTP/1.0 200 OK\r\n";
		send_content_type(f, conn);

		gchar buffer[1024];
		gsize bytes_read;

		while (pfile->read(buffer, 1024, bytes_read) == Glib::IO_STATUS_NORMAL)
			conn->write(buffer, bytes_read);
	}
	catch (Glib::IOChannelError)
	{
		(*conn) << "<h1>404 Not Found</h1>\r\n";
		(*conn) << "reqested file '" << f << "' not found.\n";
		return;
	}
	pfile->close();
}


void AjaxServer::send_content_type(Glib::ustring f, Gnet::Conn *conn)
{
	unsigned int pos = f.rfind(".");
	Glib::ustring ext = "";

    if (pos < f.size())
		ext = f.substr(pos+1);

	if (ext == "txt")
		(*conn) << "Content-Type: text/plain\r\n\r\n";
	else if (ext == "xml")
		(*conn) << "Content-Type: text/xml\r\n\r\n";
	else if (ext == "js")
		(*conn) << "Content-Type: text/javascript\r\n\r\n";
	else if (ext == "css")
		(*conn) << "Content-Type: text/css\r\n\r\n";
	else if (ext == "png")
		(*conn) << "Content-Type: image/png\r\n\r\n";
	else if (ext == "jpg" || ext == "jepg")
		(*conn) << "Content-Type: image/jpeg\r\n\r\n";
	else if (ext == "gif")
		(*conn) << "Content-Type: image/gif\r\n\r\n";
	else
		(*conn) << "Content-Type: text/html\r\n\r\n";
}


void AjaxServer::handle_ajax_message(Glib::ustring message, Gnet::Conn *conn)
{
	unsigned int id = 0;
	Glib::ustring command;

	int seperator = message.find_first_of(" ");

	if (seperator < 0)
	{
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
			*(conn), &Gnet::Conn::close), false), 100);
		return;
	}
	else
	{
		id = util::from_str<unsigned int>(message.substr(0, seperator));
		command = message.substr(seperator+1, message.length()-seperator-1);
		util::trim(command);
	}

	AjaxServerConn *client = get_client_by_id(id);
	if (!client)
	{
		if (command.substr(0,4) == "init")
		{
			_clients.resize(_clients.size()+1);
			client = &(_clients.back());
			client->init(this, _next_id++, conn->get_host_name());
		}
		else
		{
			Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
				*(conn), &Gnet::Conn::close), false), 100);
			return;
		}
	}

	// Make sure this connection comes from the same host that
	// this client started on.
	if (client->get_client_hostname() != conn->get_host_name())
	{
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
			*(conn), &Gnet::Conn::close), false), 100);
		return;
	}

	client->handle_ajax_message(command, conn);
}


void AjaxServer::on_client_disconnect(AjaxServerConn *client)
{
	client->disconnect();

	for (std::list<AjaxServerConn>::iterator i = _clients.begin();
		i != _clients.end(); i++)
			if (&(*i) == client)
			{
				_clients.erase(i);
				return;
			}
}
