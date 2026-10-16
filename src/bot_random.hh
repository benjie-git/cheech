/*
 *   Very dull cheech-playing bot class-- scores moves randomly.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _BOT_RANDOM_HH
#define _BOT_RANDOM_HH

#include "bot_base.hh"


class BotRandom : public BotBase
{
	public:
		virtual long score_move(GameBoard *board, unsigned int player,
								MoveList *move);

		virtual Glib::ustring get_default_name() const {return "Randy";}

		virtual Glib::ustring get_type_name() const {return "Random";}
};

#endif // _BOT_RANDOM_HH
