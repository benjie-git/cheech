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

#ifndef INCL_GAME_CLIENT_HH
#define INCL_GAME_CLIENT_HH

#include <vector>
#include <sigc++/sigc++.h>

#include "gnet_conn.hh"
#include "game_board.hh"
#include "game_server.hh"


class GameClient : public sigc::trackable
{
private:
	class Player
	{
	public:
		Glib::ustring		name;
		int					color;

	public:
		Player(Glib::ustring name_ = "", int color_ = 0);
	};

private:
	const static int HEARTBEAT_TIME = 5;
	const static int TIMEOUT_HEARTBEAT = 6;

private:
	Gnet::ConnBuffered	_socket;

	GameBoard*			_board;
	unsigned int		_current_player;
	Player				_players[7];

	Glib::ustring		_name;
	unsigned int		_color;
	unsigned int		_player_number;
	bool				_spectator;

	unsigned int		_client_heartbeat;
	unsigned int		_server_heartbeat;

public:
	GameClient();
	virtual ~GameClient();

	GameBoard *get_board();

	sigc::signal<void, unsigned int> cmd_set_player_number;
	sigc::signal<void, unsigned int, Glib::ustring, int> cmd_player_add;
	sigc::signal<void, Glib::ustring> cmd_spectator_add;
	sigc::signal<void, unsigned int> cmd_player_remove;
	sigc::signal<void, unsigned int, unsigned int> cmd_player_finish;
	sigc::signal<void> cmd_game_resync;
	sigc::signal<void, Glib::ustring> cmd_choose_new_name;
	sigc::signal<void, Glib::ustring, int> cmd_choose_new_color;
	sigc::signal<void, unsigned int, GameServer::GameStatus, 
				 unsigned int> cmd_game_turn;
	sigc::signal<void, unsigned int, unsigned int> cmd_game_undo_move;
	sigc::signal<void, MoveList*> cmd_game_show_move;
	sigc::signal<void> cmd_game_hide_move;
	sigc::signal<void, MoveList*> cmd_game_make_move;

	sigc::signal<void, Glib::ustring> evt_message;
	sigc::signal<void> evt_connected;
	sigc::signal<void> evt_cancelled;
	sigc::signal<void> evt_disconnected;

	bool ready() const;
	unsigned int get_port();
	Glib::ustring get_host_name();
	unsigned int get_my_player_number();
	Glib::ustring get_my_name();
	unsigned int get_my_color();
	bool is_spectator();
	unsigned int get_current_player();
	unsigned int get_player_color(unsigned int posn);
	Glib::ustring get_player_name(unsigned int posn);

	void chat(Glib::ustring text);
	void change_name(Glib::ustring name);
	void change_color(int color);
	void show_move(MoveList *move_list);
	void hide_move();
	void make_move(MoveList *move_list);
	void undo_move();
	void redo_move();
	void restart_game();
	void rotate_players();
	void shuffle_players();
	void reconfigure_game(unsigned int num_players, bool long_jumps,
						  bool hop_others, bool stop_others);

	void join_game(Glib::ustring host, unsigned int port, bool spectator);
	void resync_game();
	void leave_game();

private:
	GameClient(const GameClient& client);
	GameClient& operator=(const GameClient& client);

	void heartbeat();
	void connected();
	void cancelled();
	void disconnected();
	void read(Glib::ustring message);
	void error(Gnet::Conn::Error error);

	// First message sent from server, includes protocol version
	void command_HELLO(const Glib::ustring& arguments);
	// Server told us what player number we are
	void command_SET_PLAYER_NUMBER(const Glib::ustring& arguments);
	// Add a player to the game
	void command_PLAYER_ADD(const Glib::ustring& arguments);
	// Add a spectator to the game
	void command_SPECTATOR_ADD(const Glib::ustring& arguments);
	// Remove a player from the game
	void command_PLAYER_REMOVE(const Glib::ustring& arguments);
	// Player chatted me
	void command_PLAYER_CHAT(const Glib::ustring& arguments);
	// Player finished
	void command_PLAYER_FINISH(const Glib::ustring& arguments);
	// Server sent me a text message
	void command_CLIENT_MESSAGE(const Glib::ustring& arguments);
	// Receive confirmation that server heard my heartbeat
	void command_CLIENT_HEARTBEAT(const Glib::ustring& arguments);
	// Receive server's heartbeat
	void command_SERVER_HEARTBEAT(const Glib::ustring& arguments);
	// The board/num_players has changed
	void command_GAME_SETUP(const Glib::ustring& arguments);
	// We need to choose another name becasue NAME is already in use
	void command_PLAYER_CHOOSE_NAME(const Glib::ustring& arguments);
	// We need to choose another color becasue NAME is already using COLOR
	void command_PLAYER_CHOOSE_COLOR(const Glib::ustring& arguments);
	// It is now player N's turn
	void command_GAME_TURN(const Glib::ustring& arguments);
	// Show thses holes as selected on the board
	void command_GAME_SHOWMOVE(const Glib::ustring& arguments);
	// Deselect all holes
	void command_GAME_HIDEMOVE(const Glib::ustring& arguments);
	// Make/Commit this move to the board
	void command_GAME_MAKEMOVE(const Glib::ustring& arguments);
	// Undo/Redo the last move
	void command_GAME_UNDOMOVE(const Glib::ustring& arguments);
	// Reset the game board
	void command_GAME_BOARD(const Glib::ustring& arguments);
};

#endif   // #ifndef INCL_GAME_CLIENT_HH
