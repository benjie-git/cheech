/*
 *   Sinmple cheech-playing bot class-- scores moves by distance moved forward.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _BOT_SIMPLE_HH
#define _BOT_SIMPLE_HH

#include "bot_base.hh"


class BotSimple : public BotBase
{
	public:
		virtual long score_move(GameBoard *board, unsigned int player,
								MoveList *move);

		virtual Glib::ustring get_default_name() const {return "Chong";}

		virtual Glib::ustring get_type_name() const {return "Simple";}
};

#endif // _BOT_SIMPLE_HH
