/*
 *  This bot tries to make a good move for him that's bad for everyone else.
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
 
#include "bot_mean.hh"


BotMean::BotMean(unsigned int depth) : BotFriendly(depth)
{
	_self_penalty = 3;
}


Glib::ustring BotMean::get_default_name() const
{
	switch (_depth)
	{
		case 1:
			return "Mean";
		case 2:
			return "Jimbo";
		case 3:
			return "Nelson";
		case 4:
			return "Burns";
		default:
			return "Who,Now?";
	}
}


void BotMean::find_best_move(GameBoard *board, unsigned int player,
							 std::vector<MoveList> *best_moves,
							 long *best_score)
{
	BotFriendly::find_best_move(board, player, best_moves, best_score);

	// negate score if switching between me and not-me on the way 
	// in to the next depth.
	if ((player != _my_player_num &&
		board->get_next_player(player) == _my_player_num) ||
		(player == _my_player_num &&
		board->get_next_player(player) != _my_player_num))
			*best_score *= -1;
}

/*
long BotMean::score_this_move(GameBoard *board, unsigned int player,
			      MoveList *move)
{
  long score = BotFriendly::score_this_move(board, player, move);
    
  if (player != _my_player_num) {
    score = -score;
  }
    
  return score;
}
*/
