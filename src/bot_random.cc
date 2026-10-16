/*
 *   Very dull cheech-playing bot class-- scores moves randomly.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "bot_random.hh"


long BotRandom::score_move(GameBoard *board, unsigned int player,
						   MoveList *move)
{
	long score = _rand.get_int_range(1, 1000);

	return score;
}
