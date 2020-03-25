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

	protected:
		virtual void on_cmd_game_turn(unsigned int posn, 
									  GameServer::GameStatus status,
									  unsigned int move_count);

		virtual long score_move_recurse(GameBoard *board, unsigned int player,
										MoveList *move);

		virtual long score_this_move(GameBoard *board, 
									 unsigned int player,
									 MoveList *move);

		unsigned int	_depth;
		unsigned int	_current_depth;

		std::vector<MoveList>	_scratch_moves;
		std::vector< std::vector<MoveList> >	_scratch_best_moves;
};

#endif // _BOT_LOOKAHEAD_HH
