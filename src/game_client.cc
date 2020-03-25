/*
 *  The Chinese Checkers client.
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

#include <glibmm/main.h>
#include <sigc++/sigc++.h>
#include <iostream>
#include <sstream>
#include <vector>

#include "config.h"
#include "game_client.hh"
#include "utility.hh"

// #define DEBUG_CLIENT 1


using namespace std;
using namespace Gnet;


GameClient::Player::Player(Glib::ustring name_, int color_)
	: name(name_),
	  color(color_)
{
}


GameClient::GameClient()
	: _current_player(0),
	  _name(""),
	  _color(0),
	  _player_number(0),
	  _client_heartbeat(0),
	  _server_heartbeat(0)
{
	Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this,
		&GameClient::heartbeat), true), HEARTBEAT_TIME * 1000);

	_board = NULL;

	_socket.evt_connected.connect(sigc::mem_fun(*this, &GameClient::connected));
	_socket.evt_closed.connect(sigc::mem_fun(*this, &GameClient::disconnected));
	_socket.evt_cancelled.connect(sigc::mem_fun(*this,
		&GameClient::cancelled));
	_socket.evt_data_available.connect(sigc::mem_fun(*this,
		&GameClient::read));
	_socket.evt_error.connect(sigc::mem_fun(*this, &GameClient::error));
}


GameClient::~GameClient()
{
	if (_board)
		delete _board;
}


bool GameClient::ready() const
{
	return (_socket.get_status() == Gnet::Conn::statConnected);
}


unsigned int GameClient::get_port()
{
	return _socket.get_port();
}


Glib::ustring GameClient::get_host_name()
{
	return _socket.get_host_name();
}


Glib::ustring GameClient::get_my_name()
{
	return _name;
}


unsigned int GameClient::get_my_color()
{
	return _color;
}


bool GameClient::is_spectator()
{
	return _spectator;
}


unsigned int GameClient::get_my_player_number()
{
	return _player_number;
}


unsigned int GameClient::get_current_player()
{
	return _current_player;
}


unsigned int GameClient::get_player_color(unsigned int posn)
{
	return _players[posn].color;
}


Glib::ustring GameClient::get_player_name(unsigned int posn)
{
	return _players[posn].name;
}


GameBoard *GameClient::get_board()
{
	return _board;
}


void GameClient::chat(Glib::ustring text)
{
	if (ready())
		_socket << "PLAYER_CHAT " + text + "\n";
}


void GameClient::change_name(Glib::ustring name)
{
	_name = name;
	if (ready())
		_socket << "PLAYER_NAME " + _name + "\n";
}


void GameClient::change_color(int color)
{
	_color = color;
	if (ready())
		_socket << "PLAYER_COLOR " + util::to_str(_color) + "\n";
}


void GameClient::show_move(MoveList *move_list)
{
	if (!ready())
		return;

	Glib::ustring move_str = util::to_str<unsigned int>(move_list->size());
	for (MoveList::iterator h = move_list->begin(); h < move_list->end(); h++)
	{
		move_str += " " + util::to_str<unsigned int>(*h);
	}

	_socket << "GAME_SHOWMOVE " + move_str + "\n";
}


void GameClient::hide_move()
{
	if (ready())
		_socket << "GAME_HIDEMOVE \n";
}


void GameClient::make_move(MoveList *move_list)
{
	if (!ready())
		return;

	Glib::ustring move_str = util::to_str<unsigned int>(move_list->size());
	for (MoveList::iterator h = move_list->begin(); h < move_list->end(); h++)
	{
		move_str += " " + util::to_str<unsigned int>(*h);
	}

	_socket << "GAME_MAKEMOVE " + move_str + "\n";
}


void GameClient::undo_move()
{
	if (ready())
		_socket << "GAME_UNDOMOVE\n";
}


void GameClient::redo_move()
{
	if (ready())
		_socket << "GAME_REDOMOVE\n";
}


void GameClient::restart_game()
{
	if (ready())
		_socket << "GAME_RESTART\n";
}


void GameClient::rotate_players()
{
	if (ready())
		_socket << "GAME_ROTATE\n";
}


void GameClient::shuffle_players()
{
	if (ready())
		_socket << "GAME_SHUFFLE\n";
}


void GameClient::reconfigure_game(unsigned int num_players, bool long_jumps,
								  bool hop_others, bool stop_others)
{
	if (ready())
		_socket << "GAME_RECONFIG " + util::to_str(num_players) +
			(long_jumps?" 1":" 0") +
			(hop_others?" 1":" 0") +
			(stop_others?" 1":" 0") + "\n" ;
}


void GameClient::join_game(Glib::ustring host, unsigned int port,
						   bool spectator)
{
	leave_game();
	if (ready())
		return;

	_spectator = spectator;
	_socket.connect(host, port);
}


void GameClient::resync_game()
{
	if (ready())
		_socket << "SERVER_SYNC\n";
}


void GameClient::leave_game()
{
	_current_player = 0;

	if (ready())
		_socket.close();
}


void GameClient::heartbeat()
{
	_client_heartbeat = (_client_heartbeat + 1) % TIMEOUT_HEARTBEAT;

	if (!ready())
		return;

	if (_client_heartbeat == _server_heartbeat)
	{
		_socket.close();
		return;
	}
	_socket << "CLIENT_HEARTBEAT " + util::to_str(_client_heartbeat) + "\n";
	return;
}


void GameClient::connected()
{
	_server_heartbeat = _client_heartbeat;

	if (_spectator)
		_socket << "SPECTATOR_ADD " + _name + "\n";
	else
		_socket << "PLAYER_ADD " + util::to_str(_color) + " " + _name + "\n";

	evt_connected();
}


void GameClient::cancelled()
{
	evt_cancelled();
}


void
GameClient::disconnected()
{
	for (int i = 1; i <= 6; i++)
	{
		_players[i].name = "";
		_players[i].color = 0;
		cmd_player_remove(i);
	}
	if (_board)
	{
		delete _board;
		_board = NULL;
	}
	_current_player = 0;
	evt_disconnected();
}


void
GameClient::read(Glib::ustring message)
{
	Glib::ustring command, arguments;

	int seperator = message.find_first_of(" ");
	if (seperator < 0)
	{
		command = message.substr(0, message.length());
	}
	else
	{
		command = message.substr(0, seperator);
		arguments = message.substr(seperator+1, message.length()-seperator-1);
	}

#ifdef DEBUG_CLIENT
		cerr << "CLIENT RECEIVED : " << message << endl;
#endif

	if (command == "HELLO")
		command_HELLO(arguments);
	else if (command == "SET_PLAYER_NUMBER")
		command_SET_PLAYER_NUMBER(arguments);
	else if (command == "PLAYER_ADD")
		command_PLAYER_ADD(arguments);
	else if (command == "SPECTATOR_ADD")
		command_SPECTATOR_ADD(arguments);
	else if (command == "PLAYER_REMOVE")
		command_PLAYER_REMOVE(arguments);
	else if (command == "PLAYER_CHAT")
		command_PLAYER_CHAT(arguments);
	else if (command == "PLAYER_FINISH")
		command_PLAYER_FINISH(arguments);
	else if (command == "CLIENT_MESSAGE")
		command_CLIENT_MESSAGE(arguments);
	else if (command == "CLIENT_HEARTBEAT")
		command_CLIENT_HEARTBEAT(arguments);
	else if (command == "SERVER_HEARTBEAT")
		command_SERVER_HEARTBEAT(arguments);
	else if (command == "GAME_SETUP")
		command_GAME_SETUP(arguments);
	else if (command == "PLAYER_CHOOSE_NAME")
		command_PLAYER_CHOOSE_NAME(arguments);
	else if (command == "PLAYER_CHOOSE_COLOR")
		command_PLAYER_CHOOSE_COLOR(arguments);
	else if (command == "GAME_TURN")
		command_GAME_TURN(arguments);
	else if (command == "GAME_SHOWMOVE")
		command_GAME_SHOWMOVE(arguments);
	else if (command == "GAME_HIDEMOVE")
		command_GAME_HIDEMOVE(arguments);
	else if (command == "GAME_MAKEMOVE")
		command_GAME_MAKEMOVE(arguments);
	else if (command == "GAME_UNDOMOVE")
		command_GAME_UNDOMOVE(arguments);
	else if (command == "GAME_BOARD")
		command_GAME_BOARD(arguments);

#ifdef DEBUG_CLIENT
	else
		cerr << "* ERROR * " << "GameClient received invalid message : "
			<< message << endl;
#endif
}


void GameClient::error(Conn::Error error)
{
	if (error == Conn::errAddressLookup || error==Conn::errNoTCPConnection)
		evt_message("Failed to connect to the game!");
	else if (ready())
		evt_message("Error while communicating with the game!");
}


void GameClient::command_HELLO(const Glib::ustring& arguments)
{
	Glib::ustring version = arguments;

	if (version != PROTO_VERSION)
	{
		evt_message("Couldn't connect to the game!  "
			"You're using a different version of cheech than the server.  "
			"Make sure you're both running the most recent version.");
		leave_game();
	}
}


void GameClient::command_SET_PLAYER_NUMBER(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);

	_player_number = posn;
	cmd_set_player_number(posn);
}


void GameClient::command_PLAYER_ADD(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);

	_players[posn].color = util::from_str<int>(arguments.substr(2));
	_players[posn].name = arguments.substr(4);

	cmd_player_add(posn, _players[posn].name, _players[posn].color);
}


void GameClient::command_SPECTATOR_ADD(const Glib::ustring& arguments)
{
	Glib::ustring name = arguments;

	cmd_spectator_add(name);
}


void GameClient::command_PLAYER_REMOVE(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);

	_players[posn].name = "";
	_players[posn].color = 0;

	cmd_player_remove(posn);
}


void GameClient::command_PLAYER_CHAT(const Glib::ustring& arguments)
{
	evt_message(arguments);
}


void GameClient::command_PLAYER_FINISH(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);
	unsigned int move_count = util::from_str<unsigned int>(arguments.substr(2));

	cmd_player_finish(posn, move_count);
}


void GameClient::command_CLIENT_MESSAGE(const Glib::ustring& arguments)
{
	evt_message(arguments);
}


void GameClient::command_CLIENT_HEARTBEAT(const Glib::ustring& arguments)
{
	_server_heartbeat = util::from_str<unsigned int>(arguments);
}


void GameClient::command_SERVER_HEARTBEAT(const Glib::ustring& arguments)
{
	_socket << "SERVER_HEARTBEAT " + arguments + "\n";
}


void GameClient::command_GAME_SETUP(const Glib::ustring& arguments)
{
	unsigned int num_players = util::from_str<unsigned int>(arguments);
	bool long_jumps = (bool)((int)util::from_str<int>(arguments.substr(2)));
	bool hop_others = (bool)((int)util::from_str<int>(arguments.substr(4)));
	bool stop_others = (bool)((int)util::from_str<int>(arguments.substr(6)));

	if (_board)
		_board->reconfigure_board(num_players, long_jumps, hop_others,
								  stop_others);
	else
		_board = new GameBoard(num_players, long_jumps, hop_others,
							   stop_others);
}


void GameClient::command_PLAYER_CHOOSE_NAME(const Glib::ustring& arguments)
{
	Glib::ustring name = arguments;

	cmd_choose_new_name(name);
}


void GameClient::command_PLAYER_CHOOSE_COLOR(const Glib::ustring& arguments)
{
	int color = util::from_str<int>(arguments);
	Glib::ustring name = arguments.substr(2);

	cmd_choose_new_color(name, color);
}


void GameClient::command_GAME_TURN(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);
	GameServer::GameStatus status = (GameServer::GameStatus)
		util::from_str<unsigned int>(arguments.substr(2));
	unsigned int move_number =
		util::from_str<unsigned int>(arguments.substr(4));

	_current_player = posn;
	cmd_game_turn(posn, status, move_number);
}


void GameClient::command_GAME_SHOWMOVE(const Glib::ustring& arguments)
{
	istringstream argStream(arguments);
	MoveList move_list;
	int numMoves;

	argStream >> numMoves;
	for (int i = 0; i < numMoves; i++)
	{
		int j;
		argStream >> j;
		move_list.push_back(j);
	}

	cmd_game_show_move(&move_list);
}


void GameClient::command_GAME_HIDEMOVE(const Glib::ustring& arguments)
{
	cmd_game_hide_move();
}


void GameClient::command_GAME_MAKEMOVE(const Glib::ustring& arguments)
{
	istringstream argStream(arguments);
	MoveList move_list;
	int numMoves;

	argStream >> numMoves;
	for (int i = 0; i < numMoves; i++)
	{
		int j;
		argStream >> j;
		move_list.push_back(j);
	}

	_board->make_move_list(move_list);
	cmd_game_make_move(&move_list);
}


void GameClient::command_GAME_UNDOMOVE(const Glib::ustring& arguments)
{
	istringstream argStream(arguments);
	unsigned int from;
	unsigned int to;

	argStream >> from;
	argStream >> to;

	_board->move_peg(from, to);
	cmd_game_undo_move(from, to);
}


void GameClient::command_GAME_BOARD(const Glib::ustring& arguments)
{
	istringstream argStream(arguments);
	int holePlayer;

	for (unsigned int i = 0; i < GameBoard::SIZE; i++)
	{
		GameHole* hole = (*_board)[i];
		if (hole != 0)
		{
			argStream >> holePlayer;
			hole->set_current_player(static_cast<int>(holePlayer));
		}
	}
	_board->reset_peg_lists();
	cmd_game_resync();
}
