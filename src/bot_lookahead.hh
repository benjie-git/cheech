/*
 *  Smarter cheech-playig bot class-- looks ahead multiple moves, and scores
 *  not just the one move, but the whole board.
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

#ifndef _BOT_LOOKAHEAD_HH
#define _BOT_LOOKAHEAD_HH

#include "bot_base.hh"

#include <cstdint>


class BotLookAhead : public BotBase
{
	public:
		// tt_bits sizes the transposition table (2^bits entries); search
		// clones pass the smaller TT_CLONE_BITS to bound the memory used
		// by the whole worker pool.
		BotLookAhead(unsigned int depth,
					 unsigned int tt_bits = TT_SIZE_BITS);

		virtual void find_best_move(GameBoard *board, unsigned int player,
									std::vector<MoveList> *best_moves,
									long *best_score);

		virtual long score_move(GameBoard *board, unsigned int player,
								MoveList *move);

		virtual Glib::ustring get_default_name() const;
		virtual Glib::ustring get_type_name() const;

		virtual BotBase* clone_for_search() const;
		virtual bool supports_parallel_search() const;
		virtual void prepare_search(GameBoard *board);

		// Changes the search depth at runtime, resizing the per-plie move
		// buffers to match and keeping _current_depth consistent.  Used when a
		// bot realises a full-depth search is unnecessary for the number of
		// players it is focused on.
		void set_depth(unsigned int depth);

		// Weight dividing the root player's own progress: 1 for plain
		// LookAhead and Mean, 2 for Friendly (which halves its own progress
		// to stay helpful).  Search clones copy it (see clone_for_search()).
		void set_self_penalty(int self_penalty);

		// A friend/enemy focus that names a single other player buys no
		// extra planning: shorten a depth-4 search to depth 3.  (No-op while
		// a search is running.)
		virtual void set_friends(unsigned int mask);
		virtual void set_enemies(unsigned int mask);

	protected:
		virtual void on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count);

		// Shortens a depth-4 search to depth 3 when `mask` focuses on a
		// single other player (see set_friends/set_enemies).  No-op while a
		// search is running.
		void maybe_shorten_depth(unsigned int mask);

		virtual long score_move_recurse(GameBoard *board, unsigned int player,
										MoveList *move);

		virtual long score_this_move(GameBoard *board, 
									 unsigned int player,
									 MoveList *move);

		// Progress/finish component of a move's score, without the
		// sportsmanship penalty (see BotBase::goal_block_penalty).
		long score_progress(GameBoard *board,
							unsigned int player,
							MoveList *move);

		void update_distance_cache(GameBoard *board);

		// Paranoid alpha-beta search.  Returns the value of the remaining
		// `remaining` plies from the point of view of `root`, with `player`
		// to move.  The root player maximises and every other player
		// minimises.  When `_paranoid` is false the original cooperative
		// max-max recursion (score_move_recurse) is used instead.
		long paranoid_search(GameBoard *board, unsigned int player,
							 unsigned int root, unsigned int remaining,
							 long alpha, long beta);

		// Value of a single root move under the paranoid search, including
		// the move's own score.
		long paranoid_move_value(GameBoard *board, unsigned int player,
								 MoveList *move);

		unsigned int	_depth;
		unsigned int	_current_depth;

		// When true the paranoid alpha-beta search is used (Mean's
		// adversarial model: every other player minimises the root player's
		// score).  When false the cooperative max-max recursion
		// (score_move_recurse) is used; for plain LookAhead, which does not
		// advance the player between plies, that means planning a run of its
		// own moves with the opponents frozen.
		bool	_paranoid;

		// Root player for the current search (set when a root move is
		// scored), the weight dividing that player's own progress, and the
		// depth the bot was created with (set_depth() can temporarily lower
		// _depth for narrow focuses).
		unsigned int	_root_player;
		int				_self_penalty;
		unsigned int	_full_depth;

		long	_dist_to_goal[7][GameBoard::SIZE];
		// Player count the distance cache was built for (it only depends on
		// static geometry, so it is rebuilt just when this changes).
		unsigned int	_dist_players;

		// One reusable candidate-move pool per remaining-depth level, so
		// the alpha-beta kernel does not allocate a fresh vector per node.
		std::vector<std::vector<MoveList> >	_search_moves;

		// Fixed-size always-replace transposition table.  Invalid entries
		// are distinguished by a generation stamp rather than by clearing
		// the whole table on every turn.  Scores are bounds (see below)
		// rather than exact values once alpha-beta pruning is in use.
		static const unsigned char TT_EXACT = 0;
		static const unsigned char TT_LOWER = 1;
		static const unsigned char TT_UPPER = 2;

		struct TTEntry
		{
			uint64_t		key;
			long			score;
			uint32_t		best;
			unsigned int	gen;
			unsigned char	flag;
		};

		// TT footprint: 2^TT_SIZE_BITS entries of TTEntry.  18 gives 8 MiB
		// per table on desktops; iOS builds trim the main table to 15 (1
		// MiB) and the per-thread search clones to 14 so the worker pool
		// stays bounded.
#if defined(CHEECH_IOS)
		static const unsigned int TT_SIZE_BITS = 15;
		static const unsigned int TT_CLONE_BITS = 14;
#else
		static const unsigned int TT_SIZE_BITS = 18;
		static const unsigned int TT_CLONE_BITS = 16;
#endif
		static const unsigned int TT_SIZE = 1u << TT_SIZE_BITS;
		static const unsigned int TT_MASK = TT_SIZE - 1;

		std::vector<TTEntry>	_tt;
		unsigned int			_tt_gen;
		unsigned int			_tt_mask;
};

#endif // _BOT_LOOKAHEAD_HH
