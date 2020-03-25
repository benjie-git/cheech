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

#ifndef INCL_GAME_SERVER_HH
#define INCL_GAME_SERVER_HH

#include <vector>
#include <sigc++/sigc++.h>

#include "gnet_server.hh"
#include "game_board.hh"

#define PROTO_VERSION "9"


class GameServer : public sigc::trackable
{
private:
	class Player
	{
		public:
			Gnet::Conn*	socket;
			Glib::ustring	name;
			unsigned int	color;
			Glib::ustring	location;
			unsigned int	heartbeat;
			bool			spectator;

			Player(Gnet::Conn *socket_ = NULL, Glib::ustring name_ = "",
				   unsigned int color_ = 0, Glib::ustring location_ = "",
				   unsigned int heartbeat_ = 0, bool spectator = false);
	};

public:
	GameServer(unsigned int port, unsigned int num_players, bool long_jumps,
			   bool hop_others, bool stop_others);
	virtual ~GameServer();

	typedef enum {WaitingForPlayers=0, Start, Playing, Won, End} GameStatus;

private:
	GameServer(const GameServer& server);
	GameServer& operator=(const GameServer& server);

private:
	const static int HEARTBEAT_TIME = 5;
	const static int TIMEOUT_HEARTBEAT = 6;

private:
	Gnet::Server			_socket;
	GameBoard*				_board;
	std::vector<Player>		_players;
	std::vector<Player>		_spectators;
	unsigned int			_num_connected_players;
	unsigned int			_num_players;
	unsigned int			_heartbeat;
	unsigned int			_port;
	bool					_long_jumps;
	bool					_hop_others;
	bool					_stop_others;
	unsigned int			_current_player;
	unsigned int			_move_count;
	std::vector<MoveList>	_undo_stack;
	std::vector<MoveList>	_redo_stack;

public:
	const Gnet::Server& getSocket() const;
	unsigned int get_num_players() const;
	bool ready() const;

	void parse_command(Glib::ustring message);
	
	void help();

	void new_game();
	void restart_game();
	void end_game();
	void reconfigure_game(unsigned int num_players, bool long_jumps,
						  bool hop_others, bool stop_others);

	void pack_players();
	void rotate_players();
	void shuffle_players();

	void kick_player(unsigned int posn);

public:
	sigc::signal<void, Glib::ustring> evt_message;
	sigc::signal<void> evt_game_over;

private:
	GameServer& operator<<(const Glib::ustring& text);

	void heartbeat_players();
	void prepare_player(Gnet::Conn* socket);
	void attempt_set_player_name(Player *player, Glib::ustring name);
	void attempt_set_player_color(Player *player, unsigned int color);

	void game_turn(unsigned int posn);
	void move_increment();
	void player_finish(unsigned int posn);
	void game_over();

	unsigned int get_client_posn(Gnet::Conn* socket);
	unsigned int get_client_posn_from_name(Glib::ustring name);
	GameServer::Player* get_client_player(Gnet::Conn* socket);
	GameServer::GameStatus get_game_status();
	void add_client(Gnet::Conn* socket);
	void remove_client(Gnet::Conn* socket);
	void read_client(Glib::ustring message, Gnet::Conn* socket);
	void error_client(Gnet::Conn::Error error, Gnet::Conn* socket);

	// Client is adding itself as a player
	void command_PLAYER_ADD(Gnet::Conn* socket,
							const Glib::ustring& arguments);
	// Client is adding itself as a spectator
	void command_SPECTATOR_ADD(Gnet::Conn* socket,
							   const Glib::ustring& arguments);
	// Client wants a resync
	void command_SERVER_SYNC(Gnet::Conn* socket,
							 const Glib::ustring& arguments);
	// Confirmation that client heard our heartbeat
	void command_SERVER_HEARTBEAT(Gnet::Conn* socket,
								  const Glib::ustring& arguments);
	// Receive client's heartbeat
	void command_CLIENT_HEARTBEAT(Gnet::Conn* socket,
								  const Glib::ustring& arguments);
	// Player is changing it's name
	void command_PLAYER_NAME(Gnet::Conn* socket,
							 const Glib::ustring& arguments);
	// Player is changing it's color
	void command_PLAYER_COLOR(Gnet::Conn* socket,
							  const Glib::ustring& arguments);
	// Player sent a chat
	void command_PLAYER_CHAT(Gnet::Conn* socket,
							 const Glib::ustring& arguments);
	// Player says to show these holes as selected
	void command_GAME_SHOWMOVE(Gnet::Conn* socket,
								const Glib::ustring& arguments);
	// Player says to deselect all holes
	void command_GAME_HIDEMOVE(Gnet::Conn* socket, 
								const Glib::ustring& arguments);
	// Player says make/commit this move
	void command_GAME_MAKEMOVE(Gnet::Conn* socket,
								const Glib::ustring& arguments);
	// Player requests undo of last move
	void command_GAME_UNDOMOVE(Gnet::Conn* socket,
								const Glib::ustring& arguments);
	// Player requests redo of last move
	void command_GAME_REDOMOVE(Gnet::Conn* socket,
								const Glib::ustring& arguments);
	// Player requests restarting the game
	void command_GAME_RESTART(Gnet::Conn* socket,
							  const Glib::ustring& arguments);
	// Player requests rotating the players
	void command_GAME_ROTATE(Gnet::Conn* socket,
							  const Glib::ustring& arguments);
	// Player requests shuffling the players
	void command_GAME_SHUFFLE(Gnet::Conn* socket,
							  const Glib::ustring& arguments);
	// Player requests a new, possibly different game
	void command_GAME_RECONFIG(Gnet::Conn* socket,
								const Glib::ustring& arguments);
};

#endif   // #ifndef INCL_GAME_SERVER_HH
