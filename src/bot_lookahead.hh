/*
 *   Smarter cheech-playing bot class-- looks ahead multiple moves, and scores
 *   not just the one move, but the whole board.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
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

		// The default name for a search depth, without instantiating a bot.
		static Glib::ustring default_name(unsigned int depth);

		virtual BotBase* clone_for_search() const;
		virtual bool supports_parallel_search() const;
		virtual void prepare_search(GameBoard *board);

		// Weight dividing the root player's own progress: 1 for plain
		// LookAhead and Mean, 2 for Friendly (which halves its own progress
		// to stay helpful).  Search clones copy it (see clone_for_search()).
		void set_self_penalty(int self_penalty);

	protected:
		virtual void on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count);

		// Not virtual: no subclass overrides these, and devirtualising the
		// per-node calls lets the compiler inline them.
		long score_move_recurse(GameBoard *board, unsigned int player,
								MoveList *move);

		long score_this_move(GameBoard *board, 
							 unsigned int player,
							 unsigned int front, unsigned int back);

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
		// depth the bot was created with (search clones are built at this
		// depth regardless of the current one).
		unsigned int	_root_player;
		int				_self_penalty;
		unsigned int	_full_depth;

		long	_dist_to_goal[7][GameBoard::SIZE];
		// Static hole ownership (GameBoard::SIZE for non-holes), cached
		// alongside the distances so score_this_move() can apply the goal
		// penalties without walking into the GameHole objects.
		unsigned int	_end_player[GameBoard::SIZE];
		// Player count the distance cache was built for (it only depends on
		// static geometry, so it is rebuilt just when this changes).
		unsigned int	_dist_players;

		// One reusable candidate-move pool per remaining-depth level, so
		// the alpha-beta kernel does not allocate a fresh vector per node.
		std::vector<std::vector<MoveList> >	_search_moves;

		// One reusable path buffer per search depth.  find_best_move() used
		// to construct (and heap-allocate) a fresh MoveList for every node.
		std::vector<MoveList>	_search_paths;

		// Fixed-size always-replace transposition table.  Invalid entries
		// are distinguished by a generation stamp rather than by clearing
		// the whole table on every turn.  Scores are bounds (see below)
		// rather than exact values once alpha-beta pruning is in use.
		static const unsigned char TT_EXACT = 0;
		static const unsigned char TT_LOWER = 1;
		static const unsigned char TT_UPPER = 2;

		// Packed to 24 bytes (from 32): scores always fit in int32 and a
		// move fits in uint16 (front * SIZE + back < 65536).  The generation
		// stamp is 16-bit; it only ever needs to differ from entries left by
		// an older search, and a game never reaches 65536 turns.
		struct TTEntry
		{
			uint64_t		key;
			int32_t			score;
			uint16_t		best;
			uint16_t		gen;
			unsigned char	flag;
			unsigned char	pad;
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
		uint16_t				_tt_gen;
		unsigned int			_tt_mask;
};

#endif // _BOT_LOOKAHEAD_HH
