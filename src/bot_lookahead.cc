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

#include "bot_lookahead.hh"
#include "utility.hh"
#include <stdio.h>

BotLookAhead::BotLookAhead(unsigned int depth) : BotBase()
{
	_depth = depth;
	_current_depth = 0;
	_scratch_moves.resize(depth);
	_scratch_best_moves.resize(depth);
}


Glib::ustring BotLookAhead::get_default_name() const
{
	switch (_depth)
	{
		case 1:
			return "Batty";
		case 2:
			return "Earl";
		case 3:
			return "Sharon";
		case 4:
			return "Cosmo";
		case 5:
			return "Sloth";
		default:
			return "Who,Now?";
	}
}


void BotLookAhead::on_cmd_game_turn(unsigned int posn, 
									GameServer::GameStatus status,
									unsigned int move_count)
{
	_current_depth = _depth;
	BotBase::on_cmd_game_turn(posn, status, move_count);
}


void BotLookAhead::find_best_move(GameBoard *board, unsigned int player,
								  std::vector<MoveList> *best_moves,
								  long *best_score)
{
	// Abort if it's not my turn anymore (undo/etc)
	if (!is_still_my_turn()) return;

	_scratch_moves[_current_depth-1].clear();
	_scratch_moves[_current_depth-1].reserve(10);

	find_better_move(board, player, &(_scratch_moves[_current_depth-1]),
					 best_moves, best_score);
}


long BotLookAhead::score_move(GameBoard *board, unsigned int player,
							  MoveList *move)
{
	return score_move_recurse(board, player, move);
}


long BotLookAhead::score_move_recurse(GameBoard *board, unsigned int player,
									  MoveList *move)
{
	unsigned int front = move->front();
	unsigned int back = move->back();

	board->move_peg(front, back);

	long total_score = score_this_move(board, player, move);

	if (total_score > 9000 /* || total_score < -100*/)
	{
		board->move_peg(back, front);
		return total_score * _current_depth;
	}

	total_score = total_score * _current_depth;

	if (_current_depth > 1 && is_still_my_turn())
	{
		long best_score = LONG_MIN;

		_current_depth--;

		_scratch_best_moves[_current_depth].clear();

		find_best_move(board, player, &(_scratch_best_moves[_current_depth]),
					   &best_score);

		if (_depth - _current_depth <= 2)
			util::delay_ms(0); // Let the client process events between move

		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn())
		{
			board->move_peg(back, front);
			return total_score + best_score;
		}
		
		total_score += best_score;

		_current_depth++;
	}

	board->move_peg(back, front);

	return total_score;
}


long BotLookAhead::score_this_move(GameBoard *board, 
								   unsigned int player,
								   MoveList *move)
{
	unsigned int front = move->front();
	unsigned int back = move->back();
	unsigned int goal = board->get_goal(player);

	long from_dist = (long)(50 * board->get_distance(front, goal) + 0.5);
	long to_dist = (long)(50 * board->get_distance(back, goal) + 0.5);

	long total_score = from_dist - to_dist;

	if (board->player_finished(player))
	    total_score += 10000 + (2000 * _current_depth);
	else if (_current_depth == _depth && is_blocking_pegs(board, player))
		total_score -= 5000;

	return total_score;
}

