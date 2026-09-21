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
#include <bitset>
#include <atomic>
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

		// How smart the bot plays, as a percentage (0...100).  At 100 it
		// plays its single best choice (all moves tied for the best score);
		// lower values widen the choice to the top-N distinct score tiers.
		void set_smarts(int percent);
		int get_smarts() const;

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

		// Creates a second bot of the same concrete type/strength that can be
		// used to search on a worker thread.  Returns NULL if this bot does not
		// support parallel search.
		virtual BotBase* clone_for_search() const;

		// Whether the root move list can be searched across several threads.
		virtual bool supports_parallel_search() const;

		// Root-level setup (distance cache, transposition generation, ...) that
		// a worker clone must perform before scoring root moves.
		virtual void prepare_search(GameBoard *board);

		// Marks this object as a worker clone: it must never touch the GLib
		// main loop or the shared client.
		void set_search_clone(bool search_clone);

		// Points this object at a shared abort flag owned by the thread that
		// spawned the search.  is_still_my_turn() consults it.
		void set_search_abort(std::atomic<bool> *flag);

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

		// Strong, unscaled penalty for ending a move in another player's goal.
		// Bots may still pass through or (when forced) stop there, but they
		// must not park a peg in an opponent's goal to block them from
		// finishing.  Returns 0 for own/neutral holes.
		long goal_block_penalty(GameBoard *board, unsigned int player,
								MoveList *move) const;

		void make_best_move();
		void make_move(MoveList *list);
#ifdef CHEECH_IOS
		// Returns true while still waiting for the animation gate to open.
		bool try_commit_move(MoveList move);
#endif

		void find_better_move(GameBoard *board, unsigned int player,
			MoveList *move,	std::vector<MoveList> *best_moves, long *best_score);
		void find_better_move_for_peg(GameBoard *board, unsigned int player,
			MoveList *move,	std::vector<MoveList> *best_moves, long *best_score,
			std::bitset<GameBoard::SIZE> *tos);

		// Root-move-level parallelism.  collect_root_moves() enumerates every
		// legal root move in the same order find_better_move() would score
		// them; parallel_root_search() then scores those moves on several
		// threads and merges the results.  Returns true if it handled the
		// search (even if it was aborted).
		bool parallel_root_search(GameBoard *board, unsigned int player,
			std::vector<MoveList> *best_moves, long *best_score);
		void collect_root_moves(GameBoard *board, unsigned int player,
			std::vector<MoveList> *moves);
		void collect_peg_moves(GameBoard *board, unsigned int player,
			MoveList *move, std::bitset<GameBoard::SIZE> *tos,
			std::vector<MoveList> *moves);

		// Scores the given root moves, filling *scores in the same order.
		// Uses the worker-clone pool when allow_parallel permits, otherwise
		// scores serially on this bot.  Moves that could not be scored are
		// left as LONG_MIN.
		bool score_moves(GameBoard *board, unsigned int player,
			std::vector<MoveList> *moves, std::vector<long> *scores,
			bool allow_parallel);

		// Fills *best_moves with every move whose score falls in the top
		// `tiers` distinct score values (tiers >= 1).  *best_score is set to
		// the best score.  Moves that were never scored (LONG_MIN) are
		// skipped.
		void select_top_moves(const std::vector<MoveList> &root_moves,
			const std::vector<long> &scores, int tiers,
			std::vector<MoveList> *best_moves, long *best_score);

		// Number of distinct top score tiers to choose from for the current
		// smarts setting: 1 at 100%, rising to 6 at 50%.
		int top_move_tiers() const;

		GameClient 		_client;
		int				_think_delay;
		int				_move_step_delay;
		int				_move_done_delay;
		int				_smarts;
		bool			_abort;
		bool			_search_clone;
		std::atomic<bool>	*_search_abort;
		std::vector<BotBase*>	_search_clones;
		Glib::Rand		_rand;
};

#endif // _BOT_BASE_HH
