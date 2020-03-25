/*
 *  Sinmple cheech-playig bot class-- scores moves by distance moved forward.
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

#include "bot_simple.hh"


long BotSimple::score_move(GameBoard *board, unsigned int player,
						   MoveList *move)
{
	long old_dist = (long)(100 * board->get_distance(move->front(), 
			board->get_goal(player)) + 0.5);
	long new_dist = (long)(100 * board->get_distance(move->back(), 
			board->get_goal(player)) + 0.5);

	long score = old_dist - new_dist;

	board->move_peg(move->front(), move->back());
	if (is_blocking_pegs(board, player))
		score -= 5000;
	board->move_peg(move->back(), move->front());

	// Ugly Hack to let BotSimple finish games.
	if (old_dist > 200 && old_dist < 300 && new_dist == 300)
		score += 100;
	else if (new_dist > 200 && new_dist < 300 && old_dist == 300)
		score -= 100;

	return score;
}
