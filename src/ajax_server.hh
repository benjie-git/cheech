/*
 *   The Chinese Checkers AjaxServer listens for the Ajax-based web client
 *   to make requests, and then sends them off the the appropriate
 *   AjaxServerConn.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_AJAX_SERVER_HH
#define INCL_AJAX_SERVER_HH

#include <list>
#include <sigc++/sigc++.h>

#include "ajax_server_conn.hh"
#include "net_server.hh"

class AjaxServerConn;

class AjaxServer : public sigc::trackable
{
public:
	AjaxServer();
	virtual ~AjaxServer();

	bool listen(unsigned int ajax_port,
		Glib::ustring cheechd_hostname, unsigned int cheechd_port);
	void close();

	// Frees the server on a later main-loop iteration (used instead of a
	// direct delete so pending disconnect processing can never touch a
	// freed server).
	void dispose() { delete this; }

	Glib::ustring get_cheechd_hostname();
	unsigned int get_cheechd_port();

	void on_client_disconnect(AjaxServerConn *client);
	void schedule_client_sweep();

private:
	void connection_open(Net::Conn* socket);
	void connection_closed(Net::Conn* socket);
	AjaxServerConn* get_client_by_id(unsigned int id);
	bool sweep_dead_clients();

	// Handle messages from Ajax Frontend to GameClient
	void handle_request(Glib::ustring message, Net::Conn *conn);

	// Send files
	void send_file(Glib::ustring f, Net::Conn *conn);
	void send_content_type(Glib::ustring f, Net::Conn *conn);
	Glib::ustring conv_filename(Glib::ustring f);

	void handle_ajax_message(Glib::ustring message, Net::Conn *conn);
	unsigned int ajax_init(Glib::ustring arguments);

	Glib::ustring						_cheechd_hostname;
	unsigned int						_cheechd_port;
	Net::Server						_socket;
	std::list<AjaxServerConn>			_clients;
};

#endif   // #ifndef INCL_AJAX_SERVER_HH
