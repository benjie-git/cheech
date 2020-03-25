/*
 *  The Chinese Checkers AjaxServerConn handles requests from an Ajax-based 
 *  web client, and sends commands to it.  It keeps an extra connection open
 *  to the web frontend to send events.
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

#ifndef INCL_AJAX_SERVER_CONN_HH
#define INCL_AJAX_SERVER_CONN_HH

#include <queue>
#include <sigc++/sigc++.h>

#include "ajax_server.hh"
#include "game_client.hh"
#include "gnet_conn.hh"
#include "bot_base.hh"

class AjaxServer;

class AjaxServerConn : public sigc::trackable
{
public:
	AjaxServerConn(GameClient* client_ = NULL,
			   unsigned int id_ = 0);

	void init(AjaxServer *server, unsigned int id,
			  Glib::ustring client_hostname);
	void disconnect();
	void send_cmd(Glib::ustring message);
	void close_conn();
	void on_connection_closed(Gnet::Conn *conn);
	void ajax_push_one_msg_to_client();
	bool ping_client();
	unsigned int get_id();
	Glib::ustring get_client_hostname();

	// Handle messages from GameClient to Ajax Frontend
	void on_message(Glib::ustring message);
	void on_connect();
	void on_disconnect();
	void on_cmd_game_resync();
	void send_board();
	void on_cmd_player_add(unsigned int posn, Glib::ustring name, int color);
	void on_cmd_player_remove(unsigned int posn);
	void on_cmd_set_player_number(unsigned int posn);
	void on_cmd_game_turn(unsigned int posn, GameServer::GameStatus status,
						  unsigned int move_count);
	void on_cmd_game_undo_move(unsigned int from, unsigned int to);
	void on_cmd_player_finish(unsigned int posn, unsigned int move_count);
	void on_cmd_game_show_move(MoveList *move);
	void on_cmd_game_hide_move();
	void on_cmd_choose_new_name(Glib::ustring name);
	void on_cmd_choose_new_color(Glib::ustring name, unsigned int color);
	void on_cmd_game_make_move(MoveList *move);

	// Handle messages from Ajax Frontend to GameClient
	void handle_ajax_message(Glib::ustring message, Gnet::Conn *conn);
	void ajax_init(Glib::ustring arguments);
	void ajax_leave();
	void ajax_chat(Glib::ustring arguments);
	void ajax_change_name(Glib::ustring arguments);
	void ajax_change_color(Glib::ustring arguments);
	void ajax_game_setup(Glib::ustring arguments);
	void ajax_click(Glib::ustring arguments);
	void ajax_dblclick(Glib::ustring arguments);
	void ajax_commit(Glib::ustring arguments);
	void ajax_clear(Glib::ustring arguments);
	void ajax_restart();
	void ajax_rotate();
	void ajax_shuffle();
	void ajax_addbot(Glib::ustring arguments);
	void ajax_removebots();
	void ajax_undo();
	void ajax_redo();

	void bot_disconnected(BotBase *bot);
	bool move_list_contains(unsigned int i);


private:
	AjaxServer					*_ajax_server;
	GameClient					*_game_client;
	Gnet::Conn					*_wait_conn;
	unsigned int				_id;
	Glib::ustring				_client_hostname;
	sigc::connection			_timeout;
	sigc::connection			_disconnect;
	unsigned int				_timeout_counter;
	std::queue<Glib::ustring>	_pending_cmds;
	std::vector<BotBase*>		_bots;
	MoveList					_move_list;
};

#endif   // #ifndef INCL_AJAX_SERVER_CONN_HH
