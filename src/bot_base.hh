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

		// Player-focus bitmasks (bit i = player i+1).  ALL_PLAYERS is the
		// default and means "everyone", so an unconfigured bot behaves as
		// before.  Friendly bots only simulate (and value) players in their
		// friend set; Mean bots only simulate (and minimise) players in their
		// enemy set.  Players excluded from the relevant mask are frozen
		// obstacles during search, like the opponents LookAhead already
		// ignores.
		static const unsigned int ALL_PLAYERS = 0xFFFFFFFFu;

		virtual void set_friends(unsigned int mask);
		unsigned int get_friends() const;
		virtual void set_enemies(unsigned int mask);
		unsigned int get_enemies() const;

		// Stable-id player focus.  Unlike the bitmasks above, these survive
		// Rotate/Shuffle: the bot asks the server for the player-number/id
		// mapping and resolves the ids to the current numbers just before it
		// searches.
		void set_friend_ids(const std::set<unsigned int>& ids);
		const std::set<unsigned int>& get_friend_ids() const;
		void set_enemy_ids(const std::set<unsigned int>& ids);
		const std::set<unsigned int>& get_enemy_ids() const;

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

		// Short display name of this bot family, independent of depth or
		// skill (e.g. "Neutral" for LookAhead, "Mean", "Friendly").
		virtual Glib::ustring get_type_name() const = 0;

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
		void on_cmd_set_player_number(unsigned int posn);
		void on_cmd_player_ids_end();
		virtual void on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count);

		// Asks the server for the number/id mapping (no-op for search clones
		// and disconnected clients).
		void request_player_mapping();

		// Rebuilds _friends/_enemies by resolving the configured id sets
		// against the client's current number/id mapping.
		void refresh_focus_from_ids();

		// Defers make_best_move() by one main-loop iteration (the gnet
		// workaround) after cancelling any pending mapping timeout.
		void schedule_search();

		// Fallback for servers that never answer REQUEST_PLAYER_IDS: stop
		// waiting and search with whatever mapping we have.  Always returns
		// false so the timeout fires once.
		bool on_mapping_timeout();

		bool is_still_my_turn();
		bool is_blocking_pegs(GameBoard *board, unsigned int player);

		// Whether `player` falls inside the given focus mask (ALL_PLAYERS
		// focuses on everyone).
		bool focuses_on(unsigned int player, unsigned int mask) const;

		// Like GameBoard::get_next_player(), but additionally skips players
		// that are not in the focus mask.  Returns `from` when there is no
		// other focused, unfinished player.
		unsigned int next_focused_player(GameBoard *board, unsigned int from,
										 unsigned int mask) const;

		// Strong, unscaled penalty for ending a move in another player's goal.
		// Bots may still pass through or (when forced) stop there, but they
		// must not park a peg in an opponent's goal to block them from
		// finishing.  Returns 0 for own/neutral holes.
		long goal_block_penalty(GameBoard *board, unsigned int player,
								MoveList *move) const;

		// Strong, unscaled penalty for pulling a peg back out of the player's
		// own goal.  A peg that has reached the goal should never leave it, so
		// detuned bots must not throw away goal progress.  Returns 0 unless the
		// move starts inside the player's own goal and ends outside it.
		long goal_exit_penalty(GameBoard *board, unsigned int player,
							   MoveList *move) const;

		void make_best_move();
		void make_move(MoveList *list);
#if defined(CHEECH_IOS)
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
		// `tiers` distinct score values (tiers >= 1), excluding any move that
		// is far enough below the best one that it is clearly dominant (see
		// kDetuneGapPerTier, capped by kMaxDetuneGap).  When at least one
		// move scores above zero,
		// zero- and negative-scoring moves are excluded; when the best score
		// is zero, negatives are excluded but zeros still compete; if every
		// move is negative they are still considered.  *best_score is set to
		// the best score.
		// Moves that were never scored (LONG_MIN) are skipped.
		void select_top_moves(const std::vector<MoveList> &root_moves,
			const std::vector<long> &scores, int tiers,
			std::vector<MoveList> *best_moves, long *best_score);

		// Number of distinct top score tiers to choose from for the current
		// smarts setting: 1 at 100%, doubling every 10% down to 8 at 70%.
		int top_move_tiers() const;

		GameClient 		_client;
		int				_think_delay;
		int				_move_step_delay;
		int				_move_done_delay;
		int				_smarts;
		bool			_abort;
		bool			_searching;
		bool			_search_pending;
		unsigned int	_friends;
		unsigned int	_enemies;
		bool			_focus_by_ids;
		std::set<unsigned int>	_friend_ids;
		std::set<unsigned int>	_enemy_ids;
		bool			_awaiting_mapping;
		sigc::connection	_mapping_timeout;
		bool			_search_clone;
		std::atomic<bool>	*_search_abort;
		std::vector<BotBase*>	_search_clones;
		Glib::Rand		_rand;
};

#endif // _BOT_BASE_HH
