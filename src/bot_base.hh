/*
 *  Abstract cheech-playig bot class.
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

#ifndef _BOT_BASE_HH
#define _BOT_BASE_HH

#include <vector>
#include <set>
#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>
#include <glibmm/random.h>

#include "game_client.hh"


class BotBase : public sigc::trackable
{
	public:
		BotBase();
		virtual ~BotBase();

		static BotBase* new_bot_of_type(Glib::ustring type);

		void set_name(Glib::ustring name);
		void set_color(unsigned int color);
		void set_think_delay(int delay);
		void set_move_delay(int delay, int done_delay);
		GameClient *get_game_client();

		sigc::signal<void, Glib::ustring> evt_message;
		sigc::signal<void> evt_connected;
		sigc::signal<void> evt_cancelled;
		sigc::signal<void> evt_disconnected;

		void join_game(Glib::ustring host, unsigned int port);
		void leave_game();

		virtual void find_best_move(GameBoard *board, unsigned int player,
									std::vector<MoveList> *best_moves,
									long *best_score);
		virtual long score_move(GameBoard *board, unsigned int player,
								MoveList *move) = 0;

		virtual Glib::ustring get_default_name() const = 0;

	protected:
		void on_connect();
		void on_cancelled();
		void on_disconnect();
		void on_message(Glib::ustring msg);
		void on_cmd_choose_new_name(Glib::ustring name);
		void on_cmd_choose_new_color(Glib::ustring name, int color);
		virtual void on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count);

		bool is_still_my_turn();
		bool is_blocking_pegs(GameBoard *board, unsigned int player);

		void make_best_move();
		void make_move(MoveList *list);

		void find_better_move(GameBoard *board, unsigned int player,
			MoveList *move,	std::vector<MoveList> *best_moves, long *best_score);
		void find_better_move_for_peg(GameBoard *board, unsigned int player,
			MoveList *move,	std::vector<MoveList> *best_moves, long *best_score,
			std::set<unsigned int> *tos);

		GameClient 		_client;
		int				_think_delay;
		int				_move_step_delay;
		int				_move_done_delay;
		bool			_abort;
		Glib::Rand		_rand;
};

#endif // _BOT_BASE_HH
