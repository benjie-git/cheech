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
using namespace Net;


GameClient::Player::Player(Glib::ustring name_, int color_, unsigned int id_)
	: name(name_),
	  color(color_),
	  id(id_)
{
}


GameClient::GameClient()
	: _current_player(0),
	  _name(""),
	  _color(0),
	  _player_number(0),
	  _my_player_id(0),
	  _spectator(false),
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
	return (_socket.get_status() == Net::Conn::statConnected);
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
	if (posn > 6)
		return 0;
	return _players[posn].color;
}


Glib::ustring GameClient::get_player_name(unsigned int posn)
{
	if (posn > 6)
		return "";
	return _players[posn].name;
}


unsigned int GameClient::get_my_player_id()
{
	return _my_player_id;
}


unsigned int GameClient::get_player_id(unsigned int posn)
{
	if (posn > 6)
		return 0;
	return _players[posn].id;
}


unsigned int GameClient::get_posn_for_id(unsigned int id)
{
	if (!id)
		return 0;
	for (unsigned int posn = 1; posn <= 6; ++posn)
		if (_players[posn].id == id)
			return posn;
	return 0;
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

	Glib::ustring move_str = util::to_str<unsigned int>((uint)move_list->size());
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

	Glib::ustring move_str = util::to_str<unsigned int>((uint)move_list->size());
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


void GameClient::request_player_ids()
{
	if (ready())
		_socket << "REQUEST_PLAYER_IDS\n";
}


void GameClient::leave_game()
{
	_current_player = 0;

	if (ready())
		_socket.close();
}


void GameClient::dispose()
{
	delete this;
}


void GameClient::heartbeat()
{
#if defined(CHEECH_IOS)
	// An in-process connection cannot suffer a network partition, and iOS may
	// suspend the whole app (freezing this timer) while backgrounded, so the
	// timeout would only ever fire spuriously.  Local games never heartbeat.
	if (_socket.is_local())
		return;
#endif

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
		_players[i].id = 0;
		cmd_player_remove(i);
	}
	_my_player_id = 0;
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

	Glib::ustring::size_type seperator = message.find_first_of(" ");
	if (seperator == Glib::ustring::npos)
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
	else if (command == "PLAYER_ID")
		command_PLAYER_ID(arguments);
	else if (command == "PLAYER_ID_END")
		command_PLAYER_ID_END(arguments);

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


bool GameClient::parse_move_list(const Glib::ustring& arguments,
								 MoveList& move_list)
{
	istringstream argStream(std::string(arguments.c_str()));
	int numMoves;

	argStream >> numMoves;
	if (argStream.fail() || numMoves < 0 || numMoves > 64)
		return false;

	for (int i = 0; i < numMoves; i++)
	{
		int j;
		argStream >> j;
		if (argStream.fail())
			return false;

		if (j < 0 || j >= static_cast<int>(GameBoard::SIZE))
			return false;

		move_list.push_back(j);
	}

	return true;
}


bool GameClient::valid_move_index(unsigned int hole) const
{
	return (hole < GameBoard::SIZE);
}


void GameClient::command_SET_PLAYER_NUMBER(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);

	_player_number = posn;
	cmd_set_player_number(posn);
}


void GameClient::command_PLAYER_ADD(const Glib::ustring& arguments)
{
	istringstream argStream(std::string(arguments.c_str()));
	unsigned int posn = 0;
	int color = 0;
	std::string name;

	argStream >> posn >> color;
	std::getline(argStream, name);

	if (argStream.fail() || posn < 1 || posn > 6)
		return;

	Glib::ustring player_name(name);
	util::ltrim(player_name);

	_players[posn].color = color;
	_players[posn].name = player_name;
	_players[posn].id = 0;

	cmd_player_add(posn, _players[posn].name, _players[posn].color);
}


void GameClient::command_PLAYER_ID(const Glib::ustring& arguments)
{
	Glib::ustring::size_type separator = arguments.find_first_of(" ");
	if (separator == Glib::ustring::npos)
		return;

	unsigned int posn = util::from_str<unsigned int>(
		arguments.substr(0, separator));
	unsigned int id = util::from_str<unsigned int>(
		arguments.substr(separator+1));

	if (posn < 1 || posn > 6)
		return;

	_players[posn].id = id;
	if (posn == _player_number)
		_my_player_id = id;

	cmd_player_id(posn, id);
}


void GameClient::command_PLAYER_ID_END(const Glib::ustring& arguments)
{
	cmd_player_ids_end();
}


void GameClient::command_SPECTATOR_ADD(const Glib::ustring& arguments)
{
	Glib::ustring name = arguments;

	cmd_spectator_add(name);
}


void GameClient::command_PLAYER_REMOVE(const Glib::ustring& arguments)
{
	unsigned int posn = util::from_str<unsigned int>(arguments);

	if (posn < 1 || posn > 6)
		return;

	_players[posn].name = "";
	_players[posn].color = 0;
	_players[posn].id = 0;
	if (posn == _player_number)
		_my_player_id = 0;

	cmd_player_remove(posn);
}


void GameClient::command_PLAYER_CHAT(const Glib::ustring& arguments)
{
	evt_message(arguments);
}


void GameClient::command_PLAYER_FINISH(const Glib::ustring& arguments)
{
	istringstream argStream(std::string(arguments.c_str()));
	unsigned int posn = 0;
	unsigned int move_count = 0;

	argStream >> posn >> move_count;

	if (argStream.fail() || posn < 1 || posn > 6)
		return;

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
	istringstream argStream(std::string(arguments.c_str()));
	unsigned int num_players = 0;
	int long_jumps = 0, hop_others = 0, stop_others = 0;

	argStream >> num_players >> long_jumps >> hop_others >> stop_others;

	// num_players indexes START_MAP[7][7] and _pegs[6][]; reject anything
	// outside 1..6 instead of corrupting memory.
	if (argStream.fail() || num_players < 1 || num_players > 6)
		return;

	if (_board)
		_board->reconfigure_board(num_players, long_jumps != 0,
								  hop_others != 0, stop_others != 0);
	else
		_board = new GameBoard(num_players, long_jumps != 0,
							   hop_others != 0, stop_others != 0);
}


void GameClient::command_PLAYER_CHOOSE_NAME(const Glib::ustring& arguments)
{
	Glib::ustring name = arguments;

	cmd_choose_new_name(name);
}


void GameClient::command_PLAYER_CHOOSE_COLOR(const Glib::ustring& arguments)
{
	istringstream argStream(std::string(arguments.c_str()));
	int color = 0;
	std::string name;

	argStream >> color;
	std::getline(argStream, name);

	if (argStream.fail())
		return;

	Glib::ustring player_name(name);
	util::ltrim(player_name);

	cmd_choose_new_color(player_name, color);
}


void GameClient::command_GAME_TURN(const Glib::ustring& arguments)
{
	istringstream argStream(std::string(arguments.c_str()));
	unsigned int posn = 0;
	unsigned int status = 0;
	unsigned int move_number = 0;

	argStream >> posn >> status >> move_number;

	if (argStream.fail() || posn > 6 || status > 4)
		return;

	_current_player = posn;
	cmd_game_turn(posn, (GameServer::GameStatus)status, move_number);
}


void GameClient::command_GAME_SHOWMOVE(const Glib::ustring& arguments)
{
	MoveList move_list;
	if (!parse_move_list(arguments, move_list))
		return;

	cmd_game_show_move(&move_list);
}


void GameClient::command_GAME_HIDEMOVE(const Glib::ustring& arguments)
{
	cmd_game_hide_move();
}


void GameClient::command_GAME_MAKEMOVE(const Glib::ustring& arguments)
{
	MoveList move_list;
	if (!parse_move_list(arguments, move_list))
		return;

	if (!_board)
		return;

	_board->make_move_list(move_list);
	cmd_game_make_move(&move_list);
}


void GameClient::command_GAME_UNDOMOVE(const Glib::ustring& arguments)
{
	istringstream argStream(std::string(arguments.c_str()));
	unsigned int from;
	unsigned int to;

	argStream >> from >> to;

	if (argStream.fail() || !_board || !valid_move_index(from)
		|| !valid_move_index(to))
			return;

	_board->move_peg(from, to);
	cmd_game_undo_move(from, to);
}


void GameClient::command_GAME_BOARD(const Glib::ustring& arguments)
{
	if (!_board)
		return;

	istringstream argStream(std::string(arguments.c_str()));
	int holePlayer;

	for (unsigned int i = 0; i < GameBoard::SIZE; i++)
	{
		GameHole* hole = (*_board)[i];
		if (hole != 0)
		{
			argStream >> holePlayer;
			if (argStream.fail())
				return;

			// reset_peg_lists() indexes _pegs[player-1][] with this value.
			if (holePlayer < 0 || holePlayer > 6)
				return;

			hole->set_current_player(holePlayer);
		}
	}
	_board->reset_peg_lists();
	cmd_game_resync();
}
