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
	_self_penalty = 1;
	_paranoid = true;
}


Glib::ustring BotMean::get_default_name() const
{
	switch (_depth)
	{
		case 2:
			return "Jimbo";
		case 3:
			return "Nelson";
        case 4:
            return "Burns";
        case 5:
            return "Scorpio";
		default:
			return "Who,Now?";
	}
}


BotBase* BotMean::clone_for_search() const
{
	BotMean *clone = new BotMean(_depth);
	clone->set_self_penalty(_self_penalty);
	clone->set_self_bonus(_self_bonus);
	clone->set_tt_bits(18);
	return clone;
}


long BotMean::score_move(GameBoard *board, unsigned int player,
						 MoveList *move)
{
	// The root player's own progress is scaled by the self penalty; the
	// paranoid kernel calls score_this_move() with `player` set, so make
	// sure it knows who we are.
	if (_current_depth == _depth)
		_my_player_num = player;

	return BotLookAhead::score_move(board, player, move);
}


void BotMean::find_best_move(GameBoard *board, unsigned int player,
							 std::vector<MoveList> *best_moves,
							 long *best_score)
{
	// The paranoid alpha-beta kernel applies the sign flips internally,
	// so unlike the old recursive search there is nothing to negate here.
	BotLookAhead::find_best_move(board, player, best_moves, best_score);
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
