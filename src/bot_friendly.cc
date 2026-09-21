/*
 *  This bot tries to make a good move for him that's also good for others.
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
 
#include "bot_friendly.hh"


BotFriendly::BotFriendly(unsigned int depth) : BotLookAhead(depth)
{
	_my_player_num = 0;
	_self_penalty = 2;
	// Friendly/Mean express their own weighting through _self_penalty, so do
	// not inherit LookAhead's self-bonus.
	_self_bonus = 1;
	_paranoid = false;
}


// Care about others X times more than about self
void BotFriendly::set_self_penalty(int self_penalty)
{
	_self_penalty = self_penalty;
}


Glib::ustring BotFriendly::get_default_name() const
{
	switch (_depth)
	{
		case 1:
			return "Bonk";
		case 2:
			return "Pony";
		case 3:
			return "Nibbler";
		case 4:
			return "Unicorn";
		default:
			return "Princess";
	}
}


BotBase* BotFriendly::clone_for_search() const
{
	BotFriendly *clone = new BotFriendly(_depth);
	clone->set_self_penalty(_self_penalty);
	clone->set_self_bonus(_self_bonus);
	clone->set_tt_bits(18);
	return clone;
}


long BotFriendly::score_move_recurse(GameBoard *board, unsigned int player,
									 MoveList *move)
{
	if (_current_depth == _depth)
		_my_player_num = player;

	return BotLookAhead::score_move_recurse(board, player, move);
}


long BotFriendly::score_this_move(GameBoard *board, unsigned int player,
								  MoveList *move)
{
	// Own progress is scaled down by the self-penalty, but the sportsmanship
	// penalty must not be diluted, so it is added after the division.
	long total_score = score_progress(board, player, move);

	if (player == _my_player_num)
		total_score = total_score / _self_penalty;

	total_score += goal_block_penalty(board, player, move);

	return total_score;
}


void BotFriendly::find_best_move(GameBoard *board, unsigned int player,
							 std::vector<MoveList> *best_moves,
							 long *best_score)
{
	if (_current_depth < _depth)
		player = board->get_next_player(player);

	BotLookAhead::find_best_move(board, player,
							best_moves, best_score);
}
