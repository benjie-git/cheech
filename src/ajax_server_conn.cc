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

#include <iostream>
#include <sstream>
//#include <sigc++/object.h>
#include <sigc++/bind.h>
#include <sigc++/bind_return.h>
#include <glibmm.h>

#include "ajax_server_conn.hh"
#include "utility.hh"
#include "config.h"


#define HEARTBEAT_TIME 10 // seconds
#define HEARTBEAT_FAILURE 6 // how many failures in a row until we drop them


AjaxServerConn::AjaxServerConn(GameClient *client,
								   unsigned int id)
	:_game_client(client),
	 _wait_conn(NULL),
	 _id(id),
	 _timeout_counter(0)
{
}


void AjaxServerConn::init(AjaxServer *server, unsigned int id,
						  Glib::ustring client_hostname)
{
	_ajax_server = server;
	_id = id;
	_client_hostname = client_hostname;
}


void AjaxServerConn::send_cmd(Glib::ustring message)
{
	_pending_cmds.push(message);
	ajax_push_one_msg_to_client();
}


void AjaxServerConn::close_conn()
{
	if (_wait_conn)
	{
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(
			*_wait_conn, &Gnet::Conn::close), false), 100);
		_wait_conn = NULL;
	}
}


void AjaxServerConn::on_connection_closed(Gnet::Conn *conn)
{
	if (_wait_conn == conn)
	{
		_wait_conn = NULL;
	}
}


void AjaxServerConn::ajax_push_one_msg_to_client()
{
	if (_pending_cmds.size() && _wait_conn)
	{
		*(_wait_conn) << _pending_cmds.front();
		_pending_cmds.pop();
		close_conn();
	}
}


bool AjaxServerConn::ping_client()
{
	send_cmd("ping");
	_timeout_counter++;

	if (_timeout_counter >= HEARTBEAT_FAILURE)
	{
		_timeout.disconnect();
		disconnect();
		return false;
	}

	return true;
}


unsigned int AjaxServerConn::get_id()
{
	return _id;
}


Glib::ustring AjaxServerConn::get_client_hostname()
{
	return _client_hostname;
}


void AjaxServerConn::on_connect()
{
	// Send id
	send_cmd("id " + util::to_str(_id));
}


void AjaxServerConn::disconnect()
{
	// Tell Ajax that we're done, close GameClient
	_timeout.disconnect();
	_disconnect.disconnect();

	send_cmd("exit");
	ajax_push_one_msg_to_client();
	if (_game_client)
	{
		_game_client->leave_game();
		//delete _game_client;
		_game_client = NULL;
	}

	on_disconnect();
}


void AjaxServerConn::on_disconnect()
{
	// We just learned that the Ajax client is done
	while (!_pending_cmds.empty())
		_pending_cmds.pop();
	if (_wait_conn)
	{
		_wait_conn->close();
		_wait_conn = NULL;
	}

	_timeout.disconnect();
	_disconnect.disconnect();

	ajax_removebots();
}


void AjaxServerConn::on_message(Glib::ustring message)
{
	// send message
	send_cmd("chat " + message);
}


void AjaxServerConn::on_cmd_game_resync()
{
	// send the game info
	send_cmd("info " +
		util::to_str(_game_client->get_board()->get_num_players()) + " " +
		util::to_str(_game_client->get_board()->get_long_jumps_allowed()) + " " +
		util::to_str(_game_client->get_board()->get_hop_others_allowed()) + " " +
		util::to_str(_game_client->get_board()->get_stop_others_allowed()));

	send_board();
}


void AjaxServerConn::send_board()
{
	// send the _game_client->get_board()'s holes
	// use a stringstream (9=no hole)
	std::ostringstream arg_stream(std::ostringstream::out);
	for (unsigned int i = 0; i < GameBoard::SIZE; i++)
	{
		GameHole* hole = (*_game_client->get_board())[i];
		unsigned int posn;

		if (hole)
		{
			posn = hole->get_current_player();
		}
		else
			posn = 9;

		arg_stream << util::to_str(posn) << " ";
	}
	send_cmd("board " + arg_stream.str());

	if (_game_client->is_spectator())
		return;

	// send number of positions to rotate the board
	int rotation = 0;

	if (_game_client && _game_client->get_board())
	{
		for (int i = 1; i <= 6; i++)
			if (GameBoard::START_MAP[
				_game_client->get_board()->get_num_players()][i]
				== _game_client->get_my_player_number())
					rotation = i;
	}

	send_cmd("rotate " + util::to_str(rotation));
}


void AjaxServerConn::on_cmd_player_add(unsigned int posn,
									   Glib::ustring name, int color)
{
	// send the new/updated player info
	send_cmd("add " + util::to_str(posn) +
		" " + util::to_str(color) + " " + name);
}


void AjaxServerConn::on_cmd_player_remove(unsigned int posn)
{
	// send the posn of the removed player
	send_cmd("remove " + util::to_str(posn));
}


void AjaxServerConn::on_cmd_set_player_number(unsigned int posn)
{
	// send our player number
	send_cmd("setposn " + util::to_str(posn));
}


void AjaxServerConn::on_cmd_player_finish(unsigned int posn,
										  unsigned int move_count)
{
	// send the finishing player's move-count
	send_cmd("finish " + util::to_str(posn) + " " +
		util::to_str(move_count));
}


void AjaxServerConn::on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count)
{
	// send turn#, move count, and status
	send_cmd("turn " + util::to_str(posn) +
		" " + util::to_str(status) + " " + util::to_str(move_count));
	_move_list.clear();
}


void AjaxServerConn::on_cmd_game_show_move(MoveList *move)
{
	// send the move to hilight
	std::ostringstream arg_stream(std::ostringstream::out);
	arg_stream << util::to_str(move->size()) << " ";
	for (unsigned int i = 0; i < move->size(); i++)
		arg_stream << util::to_str((*move)[i]) << " ";
	send_cmd("show " + arg_stream.str());
}


void AjaxServerConn::on_cmd_game_hide_move()
{
	// send the command to un-select the move
	send_cmd("hide");
}


void AjaxServerConn::on_cmd_game_undo_move(unsigned int from, unsigned int to)
{
	// send the command to undo/redo a move
	send_cmd("move 2 " + util::to_str(from) + " " + util::to_str(to));
}


void AjaxServerConn::on_cmd_choose_new_name(Glib::ustring name)
{
	// send the command to choose a new name
	send_cmd("choose_name " + name);
}


void AjaxServerConn::on_cmd_choose_new_color(Glib::ustring name,
	unsigned int color)
{
	// send the command to choose a new color
	send_cmd("choose_color " + util::to_str(color) + " " + name);
}


void AjaxServerConn::on_cmd_game_make_move(MoveList *move)
{
	// send the move to make
	std::ostringstream arg_stream(std::ostringstream::out);
	arg_stream << util::to_str(move->size()) << " ";
	for (unsigned int i = 0; i < move->size(); i++)
		arg_stream << util::to_str((*move)[i]) << " ";
	send_cmd("move " + arg_stream.str());
}


void AjaxServerConn::handle_ajax_message(Glib::ustring message, Gnet::Conn *conn)
{
	Glib::ustring command;
	Glib::ustring arguments;

	int seperator = message.find_first_of(" ");

	if (seperator < 0)
		command = message;
	else
	{
		command = message.substr(0, seperator);
		arguments = message.substr(seperator+1, message.length()-seperator-1);

	}
	util::trim(command);
	util::trim(arguments);

	close_conn();
	_wait_conn = conn;

	_timeout_counter = 0;

	if (command == "init")
		ajax_init(arguments);
	else if (command == "leave")
	{
		ajax_leave();
		close_conn();
		return;
	}
	else if (command == "chat")
		ajax_chat(arguments);
	else if (command == "change_name")
		ajax_change_name(arguments);
	else if (command == "change_color")
		ajax_change_color(arguments);
	else if (command == "game_setup")
		ajax_game_setup(arguments);
	else if (command == "click")
		ajax_click(arguments);
	else if (command == "dblclick")
		ajax_dblclick(arguments);
	else if (command == "commit")
		ajax_commit(arguments);
	else if (command == "clear")
		ajax_clear(arguments);
	else if (command == "restart")
		ajax_restart();
	else if (command == "rotate")
		ajax_rotate();
	else if (command == "shuffle")
		ajax_shuffle();
	else if (command == "addbot")
		ajax_addbot(arguments);
	else if (command == "removebots")
		ajax_removebots();
	else if (command == "undo")
		ajax_undo();
	else if (command == "redo")
		ajax_redo();

	ajax_push_one_msg_to_client();
}


void AjaxServerConn::ajax_init(Glib::ustring arguments)
{
	_game_client = new GameClient();

	_disconnect = _game_client->evt_disconnected.connect(sigc::bind(
		sigc::mem_fun(*_ajax_server, &AjaxServer::on_client_disconnect), this));

	_game_client->evt_connected.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_connect));
	//_game_client->evt_cancelled.connect(sigc::mem_fun(*this,
	//	&AjaxServerConn::on_cancelled));
	_game_client->evt_message.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_message));
	_game_client->cmd_choose_new_name.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_choose_new_name));
	_game_client->cmd_choose_new_color.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_choose_new_color));
	_game_client->cmd_set_player_number.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_set_player_number));
	_game_client->cmd_game_turn.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_turn));
	_game_client->cmd_game_undo_move.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_undo_move));
	_game_client->cmd_player_add.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_player_add));
	_game_client->cmd_player_remove.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_player_remove));
	_game_client->cmd_player_finish.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_player_finish));
	_game_client->cmd_game_resync.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_resync));
	_game_client->cmd_game_show_move.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_show_move));
	_game_client->cmd_game_hide_move.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_hide_move));
	_game_client->cmd_game_make_move.connect(sigc::mem_fun(*this,
		&AjaxServerConn::on_cmd_game_make_move));

	_timeout = Glib::signal_timeout().connect(
		sigc::mem_fun(*this, &AjaxServerConn::ping_client),
		HEARTBEAT_TIME * 1000);

	if (arguments.size() < 5)
		arguments = "1 1 Nameless";

	unsigned int color = util::from_str<unsigned int>(arguments.substr(0,1));
	bool spectator = (bool)util::from_str<unsigned int>(arguments.substr(2,1));
	Glib::ustring name = arguments.substr(4);

	_game_client->change_name(name);
	_game_client->change_color(color);

	Glib::signal_timeout().connect(sigc::bind_return(sigc::bind(sigc::bind(sigc::bind(
		sigc::mem_fun(*_game_client, &GameClient::join_game),
		spectator), _ajax_server->get_cheechd_port()),
		_ajax_server->get_cheechd_hostname()), false), 100);
}


void AjaxServerConn::ajax_leave()
{
	_game_client->leave_game();
}


void AjaxServerConn::ajax_chat(Glib::ustring arguments)
{
	_game_client->chat(arguments);
}


void AjaxServerConn::ajax_change_name(Glib::ustring arguments)
{
	_game_client->change_name(arguments);
}


void AjaxServerConn::ajax_change_color(Glib::ustring arguments)
{
	_game_client->change_color(util::from_str<unsigned int>(arguments));
}


void AjaxServerConn::ajax_game_setup(Glib::ustring arguments)
{
	unsigned int num = util::from_str<unsigned int>(arguments);
	unsigned int longs = util::from_str<unsigned int>(arguments.substr(2,1));
	unsigned int hops = util::from_str<unsigned int>(arguments.substr(4,1));
	unsigned int stops = util::from_str<unsigned int>(arguments.substr(6,1));

	_game_client->reconfigure_game(num, longs, hops, stops);
}


void AjaxServerConn::ajax_click(Glib::ustring arguments)
{
	unsigned int i = util::from_str<unsigned int>(arguments);

	if (!move_list_contains(i))
	{
		_move_list.push_back(i);
		if (!((_move_list.size() == 1 &&
			(*_game_client->get_board())[i]->get_current_player() ==
			_game_client->get_my_player_number()) ||
			_game_client->get_board()->valid_move_list(_move_list, false)))
		{
			_move_list.pop_back();
		}
		else
		{
			_game_client->show_move(&_move_list);
		}
	} else {
		if (_move_list.back() == i)
		{
			_move_list.pop_back();
		}
		else
		{
			while (_move_list.size() > 0 && _move_list.back() != i)
			{
				_move_list.pop_back();
			}
		}
		if (_move_list.size() > 0 )
			_game_client->show_move(&_move_list);
		else
			_game_client->hide_move();
	}
}


void AjaxServerConn::ajax_dblclick(Glib::ustring arguments)
{
	unsigned int i = util::from_str<unsigned int>(arguments);

	if (_move_list.back() != i)
		_move_list.push_back(i);

	ajax_commit("");
}


void AjaxServerConn::ajax_commit(Glib::ustring arguments)
{
	if (_move_list.size() > 1)
		_game_client->make_move(&_move_list);
	_move_list.clear();
}


void AjaxServerConn::ajax_clear(Glib::ustring arguments)
{
	_move_list.clear();
	_game_client->hide_move();
}


void AjaxServerConn::ajax_restart()
{
	if (_game_client)
	{
		if (_game_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				_bots[0]->get_game_client()->restart_game();
		}
		else
		{
			_game_client->restart_game();
		}
	}
}


void AjaxServerConn::ajax_rotate()
{
	if (_game_client)
	{
		if (_game_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				_bots[0]->get_game_client()->rotate_players();
		}
		else
		{
			_game_client->rotate_players();
		}
	}
}


void AjaxServerConn::ajax_shuffle()
{
	if (_game_client)
	{
		if (_game_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				_bots[0]->get_game_client()->shuffle_players();
		}
		else
		{
			_game_client->shuffle_players();
		}
	}
}


void AjaxServerConn::ajax_addbot(Glib::ustring arguments)
{
	BotBase *bot = BotBase::new_bot_of_type(arguments);
	if (!bot)
		bot = BotBase::new_bot_of_type("l3");
	_bots.push_back(bot);
	bot->join_game(_ajax_server->get_cheechd_hostname(),
				   _ajax_server->get_cheechd_port());
	bot->evt_disconnected.connect(sigc::bind(sigc::mem_fun(*this,
		&AjaxServerConn::bot_disconnected), bot));
}


void AjaxServerConn::ajax_removebots()
{
	BotBase *bot;

	while (_bots.size() > 0)
	{
		bot = _bots.back();
		_bots.pop_back();
		bot->leave_game();
	}
}


void AjaxServerConn::ajax_undo()
{
	_game_client->undo_move();
}


void AjaxServerConn::ajax_redo()
{
	_game_client->redo_move();
}


void AjaxServerConn::bot_disconnected(BotBase *bot)
{
	for (std::vector<BotBase*>::iterator b = _bots.begin();
		b < _bots.end(); b++)
			if (*b == bot)
				_bots.erase(b);

	// delete bot;
}


bool AjaxServerConn::move_list_contains(unsigned int i)
{
	for (MoveList::iterator move = _move_list.begin();
		move < _move_list.end(); move++)
			if (*move == i)
				return true;

	return false;
}
