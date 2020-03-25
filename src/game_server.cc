/*
 *  The Chinese Checkers GameServer.
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
#include <vector>
#include <algorithm>
#include <glibmm/main.h>
#include <sigc++/bind.h>
#include <sigc++/bind_return.h>

#include "utility.hh"
#include "game_server.hh"
#include "ajax_server.hh"

// #define DEBUG_SERVER 1


using namespace std;
using namespace Gnet;

GameServer::Player::Player(Conn *socket_, Glib::ustring name_,
	unsigned int color_, Glib::ustring location_,
	unsigned int heartbeat_, bool spectator_)
	:socket(socket_),
	 name(name_),
	 color(color_),
	 location(location_),
	 heartbeat(heartbeat_),
	 spectator(spectator_)
{
}


GameServer::GameServer(unsigned int port, unsigned int num_players,
					  bool long_jumps, bool hop_others, bool stop_others)
	:_players(7), // the _players vector is 1-based!! (0 is not used)
	 _spectators(0), // the _spectators vector is 0-based
	 _num_connected_players(0),
	 _num_players(num_players),
	 _heartbeat(0),
	 _port(port),
	 _long_jumps(long_jumps),
	 _hop_others(hop_others),
	 _stop_others(stop_others),
	 _current_player(0),
	 _move_count(1)
{
	_board = NULL;
	_socket.evt_connection_available.connect(sigc::mem_fun(*this,
		&GameServer::add_client));

	Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*this,
		&GameServer::heartbeat_players), true), HEARTBEAT_TIME * 1000);
}


GameServer::~GameServer()
{
	if (_board)
		delete _board;
}


const Gnet::Server& GameServer::getSocket() const
{
	return _socket;
}


unsigned int GameServer::get_num_players() const
{
	return _num_players;
}


bool GameServer::ready() const
{
	return (_socket.ready());
}


void GameServer::parse_command(Glib::ustring message)
{
	util::trim(message);

	Glib::ustring command, arguments;
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

#ifdef DEBUG_SERVER
	cerr << "GameServer Console : Command = \"" << command << "\", Args = \""
		<< arguments << "\"" << endl;
#endif

	if (command == "start")
		new_game();
	else if (command == "restart")
		restart_game();
	else if (command == "end")
		end_game();
	else if (command == "pack")
		pack_players();
	else if (command == "rotate")
		rotate_players();
	else if (command == "shuffle")
		shuffle_players();
	else if (command == "resync")
	{
		for (unsigned int i = 1; i <= 6; i++)
			if (_players[i].socket != 0)
				prepare_player(_players[i].socket);
		for (unsigned int i = 0; i < _spectators.size(); i++)
			if (_spectators[i].socket != 0)
				prepare_player(_spectators[i].socket);
	}
	else if (command == "kick" || command == "k")
		if (!arguments.empty())
			kick_player(util::from_str<unsigned int>(arguments));
		else
			evt_message("Bad arguments for command \"kick\"\n");
	else if (command == "list" || command == "l")
	{
		for (unsigned int i = 1; i <= 6; i++)
			if (_players[i].socket != 0)
				evt_message(util::to_str(i) + ": " + _players[i].name);
		for (unsigned int i = 0; i < _spectators.size(); i++)
			if (_spectators[i].socket != 0)
				evt_message(util::to_str(i+7) + ": " + _spectators[i].name + " (spectator)");
	}
	else if (command == "help" || command == "?")
		help();
	else
	{
		evt_message("Unknown command : " + command);
		help();
	}
}


void GameServer::help()
{
	evt_message("Available commands: start, restart, end, pack, "
				"rotate, shuffle, resync, list, kick #, help");
}


void GameServer::new_game()
{
	end_game();

	if (_socket.listen(_port, true))
	{
		restart_game();
	}
	else
	{
		Glib::ustring message = "Failed to start a game using port "
			+ util::to_str(_port) + ".";
		evt_message(message);
	}
}


void GameServer::restart_game()
{
	if (!ready())
		return;

	if (!_board)
	{
		_board = new GameBoard(_num_players, _long_jumps, _hop_others,
							   _stop_others);
		_board->evt_player_finished.connect(sigc::mem_fun(*this,
			&GameServer::player_finish));
		_board->evt_game_over.connect(sigc::mem_fun(*this,
									  &GameServer::game_over));
	}
	else
		_board->reconfigure_board(_num_players, _long_jumps, _hop_others,
								  _stop_others);

	_undo_stack.clear();
	_redo_stack.clear();
	_move_count = 1;

	Glib::ustring message = "Starting a new game for " +
		util::to_str(_board->get_num_players()) + " player(s) ...";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";

	pack_players();
	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket != 0)
			prepare_player(_players[i].socket);
	for (unsigned int i = 0; i < _spectators.size(); i++)
		if (_spectators[i].socket != 0)
			prepare_player(_spectators[i].socket);

	_current_player = 1;
	game_turn(1);
}


void GameServer::end_game()
{
	if (!ready())
		return;

	Glib::ustring message = "Ending the current game ...";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";

	_current_player = 0;
	game_turn(0);

	_socket.close();

	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket != 0)
			_players[i].socket->close();

	while (!_spectators.empty())
	{
		if (_spectators.back().socket != 0)
			_spectators.back().socket->close();
		_spectators.pop_back();
	}
}


void GameServer::reconfigure_game(unsigned int num_players, bool long_jumps,
								  bool hop_others, bool stop_others)
{
	_num_players = num_players;
	_long_jumps = long_jumps;
	_hop_others = hop_others;
	_stop_others = stop_others;

	restart_game();
}


void GameServer::pack_players()
{
	if (!ready())
		return;

	for (unsigned int i = 1; i <= 5; i++)
	{
		if (_players[i].socket == 0)
		{
			unsigned int j;
			for (j = i + 1; j <= 6 && _players[j].socket == 0; j++);
			if (j > 6)
				return;
			swap(_players[i], _players[j]);
		}
	}
}


void GameServer::rotate_players()
{
	if (!ready())
		return;

	Glib::ustring message = "Rotating players ...";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";

	rotate(_players.begin() + 1, _players.begin() + 2, _players.end());
	restart_game();
}


void GameServer::shuffle_players()
{
	if (!ready())
		return;

	Glib::ustring message = "Shuffling players ...";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";

	random_shuffle(_players.begin() + 1, _players.end());
	restart_game();
}


void GameServer::kick_player(unsigned int posn)
{
	if (!ready())
		return;

	if (posn >= 1 && posn <= 6 && _players[posn].socket != NULL)
	{
		Glib::ustring message = "Kicking " + _players[posn].name
			+ " (Player #" + util::to_str(posn) + ") ...";
		evt_message(message);
		*this << "CLIENT_MESSAGE " + message + "\n";

		_players[posn].socket->close();
	}
	else if (posn >= 7 && posn <= _spectators.size()+6 &&
		_spectators[posn-7].socket != NULL)
	{
		Glib::ustring message = "Kicking " + _spectators[posn-7].name
			+ " (Spectator #" + util::to_str(posn-6) + ") ...";
		evt_message(message);
		*this << "CLIENT_MESSAGE " + message + "\n";

		_spectators[posn-7].socket->close();
	}
	else
	{
		evt_message("Player #" + util::to_str(posn) + " does not exist.");
		return;
	}

}


GameServer& GameServer::operator<<(const Glib::ustring& text)
{
	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket != 0)
			*_players[i].socket << text;

	for (unsigned int i = 0; i < _spectators.size(); i++)
		if (_spectators[i].socket != 0)
			*_spectators[i].socket << text;

	return *this;
}


void GameServer::heartbeat_players()
{
	_heartbeat = (_heartbeat + 1) % TIMEOUT_HEARTBEAT;

	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket != NULL && _players[i].heartbeat == _heartbeat)
		{
			*this << "CLIENT_MESSAGE " + _players[i].name +
				" (#" + util::to_str(i) + ") has timed out.\n";
			_players[i].socket->close();
			_players[i].socket = NULL;
		}

	*this << "SERVER_HEARTBEAT " + util::to_str(_heartbeat) + "\n";
}


void GameServer::prepare_player(Conn* socket)
{
	unsigned int posn = get_client_posn(socket);

	if (posn)
		*socket << "SET_PLAYER_NUMBER " + util::to_str(posn) + "\n" ;

	*socket << "GAME_SETUP " + util::to_str(_board->get_num_players()) +
		(_board->get_long_jumps_allowed()?" 1":" 0") +
		(_board->get_hop_others_allowed()?" 1":" 0") +
		(_board->get_stop_others_allowed()?" 1":" 0") + "\n" ;

	for (unsigned int i = 1; i <= 6; i++)
	{
		if (_players[i].socket != 0)
			*socket << "PLAYER_ADD " + util::to_str(i) + " "
				+ util::to_str(_players[i].color)
				+ " " + _players[i].name + "\n";
		else
			*socket << "PLAYER_REMOVE " + util::to_str(i) + "\n";
	}

	*socket << "GAME_BOARD";
	for (unsigned int i = 0; i < GameBoard::SIZE; i++)
	{
		GameHole* hole = (*_board)[i];
		if (hole != 0)
			*socket << " " + util::to_str(hole->get_current_player());
	}
	*socket << "\n";
}


void GameServer::attempt_set_player_name(Player *player, Glib::ustring name)
{
	if (!player || !player->socket)
		return;

	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket && &_players[i] != player &&
			_players[i].name == name)
		{
			*(player->socket) << "PLAYER_CHOOSE_NAME "
				+ _players[i].name + "\n";
			return;
		}

	player->name = name;
}


void GameServer::attempt_set_player_color(Player *player, unsigned int color)
{
	if (!player || !player->socket)
		return;

	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket && &_players[i] != player &&
			_players[i].color != 0 && _players[i].color == color)
		{
			*(player->socket) << "PLAYER_CHOOSE_COLOR "
				+ util::to_str(_players[i].color)
				+ " " + _players[i].name + "\n";
			player->color = 0;
			return;
		}

	player->color = static_cast<int>(color);
}


void GameServer::game_turn(unsigned int posn)
{
	if (_num_connected_players < _board->get_num_players())
		posn = 0;

	if (_board->player_finished(posn))
		posn = 0;

	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket && _players[i].color == 0)
			posn = 0;

	*this << "GAME_TURN " + util::to_str(posn) + " " +
		util::to_str(get_game_status()) + " " +
		util::to_str(_move_count) + "\n";
}


void GameServer::player_finish(unsigned int posn)
{
	evt_message(_players[posn].name + " (#" + util::to_str(posn)
		+ ") has finished in " + util::to_str(_move_count) + " moves.");
	*this << "PLAYER_FINISH " + util::to_str(posn) + "  " +
		util::to_str(_move_count) + "\n";
}


void GameServer::game_over()
{
	Glib::ustring message = "Game Over!";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";
	evt_game_over();
}


GameServer::GameStatus GameServer::get_game_status()
{
	if (_num_connected_players < _num_players)
		return GameServer::WaitingForPlayers;
	else if (_move_count == 1 && _current_player == 1)
		return GameServer::Start;
	else
	{
		int num_finished = 0;
		for (unsigned int i = 1; i <= _num_players; i++)
			if (_board->player_finished(i))
				num_finished++;

		if (num_finished == 0)
			return GameServer::Playing;
		else if (_move_count < _num_players)
			return GameServer::Won;
		else
			return GameServer::End;
	}

	return GameServer::End;
}


unsigned int GameServer::get_client_posn(Conn* socket)
{
	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket == socket)
			return i;

	return 0;
}


unsigned int GameServer::get_client_posn_from_name(Glib::ustring name)
{
	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].name == name)
			return i;

	return 0;
}


GameServer::Player* GameServer::get_client_player(Conn* socket)
{
	for (unsigned int i = 1; i <= 6; i++)
		if (_players[i].socket == socket)
			return &_players[i];

	for (unsigned int i = 0; i < _spectators.size(); i++)
		if (_spectators[i].socket == socket)
			return &_spectators[i];

	return NULL;
}


void GameServer::add_client(Conn* socket)
{
	socket->evt_data_available.connect(sigc::bind(sigc::mem_fun(*this,
		&GameServer::read_client), socket));

	*socket << "HELLO " PROTO_VERSION "\n";

	*socket << "CLIENT_MESSAGE GameServer is version " PROTO_VERSION
		", running a game for " + util::to_str(_board->get_num_players()) +
		" player(s).\n";
}


void GameServer::remove_client(Conn* socket)
{
	Player *sad_player = get_client_player(socket);

	if (sad_player->spectator)
	{
		delete sad_player->socket;
		sad_player->socket = NULL;

		Glib::ustring message = "Spectator " + sad_player->name + " from "
			+ sad_player->location + " has disconnected.";
		evt_message(message);
		*this << "CLIENT_MESSAGE " + message + "\n";

		for (std::vector<Player>::iterator pl = _spectators.begin();
			pl < _spectators.end(); pl++)
				if (&(*pl) == sad_player)
					_spectators.erase(pl, pl);
	}
	else
	{
		unsigned int posn = get_client_posn(socket);

		delete sad_player->socket;
		sad_player->socket = NULL;

		Glib::ustring message = "Player " + sad_player->name
			+ " (#" + util::to_str(posn) + ") from " + sad_player->location
			+ " has disconnected.";
		evt_message(message);
		*this << "CLIENT_MESSAGE " + message + "\n";
		*this << "PLAYER_REMOVE " + util::to_str(posn) + "\n";

		_num_connected_players--;

		if (_num_connected_players == 0 && _board->game_finished())
			restart_game();
	}

	game_turn(_current_player);
}


void GameServer::read_client(Glib::ustring message, Conn* socket)
{
	Glib::ustring command, arguments;

	int seperator = message.find_first_of(" ");
	if (seperator < 0)
		command = message.substr(0, message.length());
	else
	{
		command = message.substr(0, seperator);
		arguments = message.substr(seperator+1, message.length()-seperator-1);
	}

#ifdef DEBUG_SERVER
		cerr << "SERVER RECEIVED : " << message << endl;
#endif

	if (command == "PLAYER_ADD")
		command_PLAYER_ADD(socket, arguments);
	else if (command == "SPECTATOR_ADD")
		command_SPECTATOR_ADD(socket, arguments);
	else if (command == "PLAYER_CHAT")
		command_PLAYER_CHAT(socket, arguments);
	else if (command == "SERVER_SYNC")
		command_SERVER_SYNC(socket, arguments);
	else if (command == "SERVER_HEARTBEAT")
		command_SERVER_HEARTBEAT(socket, arguments);
	else if (command == "CLIENT_HEARTBEAT")
		command_CLIENT_HEARTBEAT(socket, arguments);
	else if (command == "PLAYER_NAME")
		command_PLAYER_NAME(socket, arguments);
	else if (command == "PLAYER_COLOR")
		command_PLAYER_COLOR(socket, arguments);
	else if (command == "GAME_SHOWMOVE")
		command_GAME_SHOWMOVE(socket, arguments);
	else if (command == "GAME_HIDEMOVE")
		command_GAME_HIDEMOVE(socket, arguments);
	else if (command == "GAME_MAKEMOVE")
		command_GAME_MAKEMOVE(socket, arguments);
	else if (command == "GAME_UNDOMOVE")
		command_GAME_UNDOMOVE(socket, arguments);
	else if (command == "GAME_REDOMOVE")
		command_GAME_REDOMOVE(socket, arguments);
	else if (command == "GAME_RESTART")
		command_GAME_RESTART(socket, arguments);
	else if (command == "GAME_ROTATE")
		command_GAME_ROTATE(socket, arguments);
	else if (command == "GAME_SHUFFLE")
		command_GAME_SHUFFLE(socket, arguments);
	else if (command == "GAME_RECONFIG")
		command_GAME_RECONFIG(socket, arguments);

#ifdef DEBUG_SERVER
	else
		cerr << "* ERROR * " << "GameServer received invalid message : "
			<< message << endl;
#endif
}


void GameServer::error_client(Conn::Error error, Conn* socket)
{
#ifdef DEBUG_SERVER
	Player *player = get_client_player(socket);
	cerr << "* ERROR * " << "Error while communicating with a client "
		<< player->name << "!" << endl;
#endif
}


void GameServer::command_PLAYER_ADD(Conn *socket,
									const Glib::ustring& arguments)
{
	if (_num_connected_players == _num_players)
	{
		*socket << "CLIENT_MESSAGE "
			<< "Sorry, this Chinese Checkers game is full.\n";
		// Delayed the close so the message can get there first
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(*socket,
			&Conn::close), false), 100);
		return;
	}

	socket->evt_closed.connect(sigc::bind(sigc::mem_fun(*this,
		&GameServer::remove_client), socket));
	socket->evt_error.connect(sigc::bind(sigc::mem_fun(*this,
		&GameServer::error_client), socket));

	int color = util::from_str<int>(arguments);
	Glib::ustring name = arguments.substr(2);

	unsigned int posn = get_client_posn_from_name(name);
	// if no player by that name, or that player is already connected,
	if (!posn || _players[posn].socket)
		posn = get_client_posn(NULL);

	_players[posn] = Player(socket,
							Glib::ustring("Player ") + util::to_str(posn),
							 0, socket->get_host_name(), _heartbeat);
	_num_connected_players++;

	attempt_set_player_name(&_players[posn], name);

	attempt_set_player_color(&_players[posn], color);

	prepare_player(socket);

	Glib::ustring message = "Server: " + _players[posn].name + " from "
		+  _players[posn].location + " has connected.";
	evt_message(message);

	message = _players[posn].name + " from " +  _players[posn].location
		+ " has connected as player " + util::to_str(posn) + ".\n";
	*this << "CLIENT_MESSAGE " + message;

	*this << "PLAYER_ADD " + util::to_str(posn)
		+ " " + util::to_str(_players[posn].color)
		+ " " + _players[posn].name + "\n";

	game_turn(_current_player);
}


void GameServer::command_SPECTATOR_ADD(Conn *socket,
									   const Glib::ustring& arguments)
{
	Glib::ustring name = arguments;

	_spectators.push_back(Player(socket, name, 0, socket->get_host_name(),
						  _heartbeat, true));

	socket->evt_closed.connect(sigc::bind(sigc::mem_fun(*this,
		&GameServer::remove_client), socket));

	prepare_player(socket);

	Glib::ustring message = _spectators.back().name + " from "
		+  _spectators.back().location + " has connected as a spectator.";
	evt_message(message);
	*this << "CLIENT_MESSAGE " + message + "\n";
	*this << "SPECTATOR_ADD " + name + "\n";

	game_turn(_current_player);
}


void GameServer::command_PLAYER_CHAT(Conn *socket,
									 const Glib::ustring& arguments)
{
	*this << "PLAYER_CHAT <<" + get_client_player(socket)->name + ">> "
		+ arguments + "\n";
}


void GameServer::command_SERVER_SYNC(Conn *socket,
									 const Glib::ustring& arguments)
{
	prepare_player(socket);
}


void GameServer::command_SERVER_HEARTBEAT(Conn *socket,
										  const Glib::ustring& arguments)
{
	get_client_player(socket)->heartbeat =
		util::from_str<unsigned int>(arguments);
}


void GameServer::command_CLIENT_HEARTBEAT(Conn *socket,
										  const Glib::ustring& arguments)
{
	*socket << "CLIENT_HEARTBEAT " + arguments + "\n";
}


void GameServer::command_PLAYER_NAME(Conn *socket,
									 const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	Glib::ustring name = arguments;

	attempt_set_player_name(player, name);

	if (!player->spectator)
		*this << "PLAYER_ADD " + util::to_str(get_client_posn(socket)) + " " +
			util::to_str(player->color) + " " + player->name + "\n";
}


void GameServer::command_PLAYER_COLOR(Conn *socket,
									  const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	int color = util::from_str<int>(arguments);

	attempt_set_player_color(player, color);

	*this << "PLAYER_ADD " + util::to_str(get_client_posn(socket)) + " " +
		util::to_str(player->color) + " " + player->name + "\n";

	if (player->color != 0)
		game_turn(_current_player);
}


void GameServer::command_GAME_SHOWMOVE(Conn *socket,
										const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	if (_num_connected_players >= _board->get_num_players()
		&& get_client_posn(socket) == _current_player)
			*this << "GAME_SHOWMOVE " + arguments + "\n";
}


void GameServer::command_GAME_HIDEMOVE(Conn *socket,
										const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	if (_num_connected_players >= _board->get_num_players() &&
		get_client_posn(socket) == _current_player)
			*this << "GAME_HIDEMOVE \n";
}


void GameServer::command_GAME_MAKEMOVE(Conn *socket,
										const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

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

	if (_num_connected_players < _board->get_num_players())
		*socket << "CLIENT_MESSAGE " << "Waiting for more players to join!\n";
	else if (get_client_posn(socket) != _current_player)
		*socket << "CLIENT_MESSAGE " << "It's not your turn yet!\n";
	else if (!(_board->valid_move_list(move_list, true) &&
			(*_board)[move_list.front()]->get_current_player() == _current_player))
	{
		*socket << "CLIENT_MESSAGE " << "Invalid move! Try again.\n";
		*this << "GAME_HIDEMOVE \n";
	}
	else
	{
		_undo_stack.push_back(move_list);
		_redo_stack.clear();

		_board->make_move_list(move_list);
		*this << "GAME_MAKEMOVE " + arguments + "\n";

		unsigned int next_player = _board->get_next_player(_current_player);

		if (next_player <= _current_player)
			_move_count++;

		_current_player = next_player;
		game_turn(_current_player);
	}
}


void GameServer::command_GAME_UNDOMOVE(Conn *socket,
										const Glib::ustring& arguments)
{
	if (_undo_stack.empty())
		return;

	if (_num_connected_players == _board->get_num_players())
	{
		MoveList move = _undo_stack.back();
		unsigned int was_player = _current_player;

		_board->move_peg(move.back(), move.front());
		_redo_stack.push_back(move);
		_undo_stack.pop_back();

		_current_player = (*_board)[move.front()]->get_current_player();

		*this << "GAME_UNDOMOVE " << util::to_str(move.back()) << " "
			<< util::to_str(move.front()) << "\n";

		if (_current_player >= was_player)
			_move_count--;

		game_turn(_current_player);
	}
}


void GameServer::command_GAME_REDOMOVE(Conn *socket,
										const Glib::ustring& arguments)
{
	if (_redo_stack.empty())
		return;

	if (_num_connected_players == _board->get_num_players())
	{
		MoveList move = _redo_stack.back();
		unsigned int was_player = _current_player;

		_board->move_peg(move.front(), move.back());
		_undo_stack.push_back(move);
		_redo_stack.pop_back();

		_current_player = _board->get_next_player(
			(*_board)[move.back()]->get_current_player());

		*this << "GAME_UNDOMOVE " << util::to_str(move.front()) << " "
			<< util::to_str(move.back()) << "\n";

		if (_current_player <= was_player)
			_move_count++;

		game_turn(_current_player);
	}
}


void GameServer::command_GAME_RESTART(Conn *socket,
									  const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	restart_game();
}


void GameServer::command_GAME_ROTATE(Conn *socket,
									  const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	rotate_players();
}


void GameServer::command_GAME_SHUFFLE(Conn *socket,
									  const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	shuffle_players();
}


void GameServer::command_GAME_RECONFIG(Conn *socket,
									   const Glib::ustring& arguments)
{
	Player *player = get_client_player(socket);

	if (player->spectator)
		return;

	unsigned int num_players = util::from_str<unsigned int>(arguments);
	bool long_jumps = (bool)((int)util::from_str<int>(arguments.substr(2)));
	bool hop_others = (bool)((int)util::from_str<int>(arguments.substr(4)));
	bool stop_others = (bool)((int)util::from_str<int>(arguments.substr(6)));

	reconfigure_game(num_players, long_jumps, hop_others, stop_others);
}
