/*
 *   Sinmple cheech-playing bot class-- scores moves by distance moved forward.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
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

	score += goal_block_penalty(board, player, move);
	score += goal_exit_penalty(board, player, move);

	return score;
}
