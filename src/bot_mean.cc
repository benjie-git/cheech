/*
 *   This bot tries to make a good move for him that's bad for everyone else.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "bot_mean.hh"


BotMean::BotMean(unsigned int depth, unsigned int tt_bits) :
	BotLookAhead(depth, tt_bits)
{
	// Mean values its own progress as-is, but runs the paranoid adversarial
	// search: every other player minimises the root player's score.
	_self_penalty = 1;
	_paranoid = true;
}


Glib::ustring BotMean::default_name(unsigned int depth)
{
	switch (depth)
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


Glib::ustring BotMean::get_default_name() const
{
	return default_name(_depth);
}


Glib::ustring BotMean::get_type_name() const
{
	return "Mean";
}


BotBase* BotMean::clone_for_search() const
{
	BotMean *clone = new BotMean(_full_depth, TT_CLONE_BITS);
	clone->set_self_penalty(_self_penalty);
	clone->set_friends(get_friends());
	clone->set_enemies(get_enemies());
	return clone;
}
