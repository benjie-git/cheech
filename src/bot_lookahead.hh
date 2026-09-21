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
		BotLookAhead(unsigned int depth);

		virtual void find_best_move(GameBoard *board, unsigned int player,
									std::vector<MoveList> *best_moves,
									long *best_score);

		virtual long score_move(GameBoard *board, unsigned int player,
								MoveList *move);

		virtual Glib::ustring get_default_name() const;

		virtual BotBase* clone_for_search() const;
		virtual bool supports_parallel_search() const;
		virtual void prepare_search(GameBoard *board);

	protected:
		virtual void on_cmd_game_turn(unsigned int posn, 
									  GameServer::GameStatus status,
									  unsigned int move_count);

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

		bool	_paranoid;

		long	_dist_to_goal[7][GameBoard::SIZE];

		std::vector<MoveList>	_scratch_moves;

		// One reusable move buffer per remaining-depth level, so the
		// alpha-beta kernel does not allocate a fresh vector per node.
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

		static const unsigned int TT_SIZE_BITS = 20;
		static const unsigned int TT_SIZE = 1u << TT_SIZE_BITS;
		static const unsigned int TT_MASK = TT_SIZE - 1;

		std::vector<TTEntry>	_tt;
		unsigned int			_tt_gen;
		unsigned int			_tt_mask;

		void set_tt_bits(unsigned int bits);
};

#endif // _BOT_LOOKAHEAD_HH
