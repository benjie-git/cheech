/*
 *   This bot tries to make a good move for him that's also good for others.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "bot_friendly.hh"


BotFriendly::BotFriendly(unsigned int depth, unsigned int tt_bits) :
	BotLookAhead(depth, tt_bits)
{
	// Friendly halves its own progress: it values the advancement of the
	// players it focuses on as much as its own (see _self_penalty).
	_self_penalty = 2;
}


Glib::ustring BotFriendly::default_name(unsigned int depth)
{
	switch (depth)
	{
		case 2:
			return "Earl";
		case 3:
			return "Unicorn";
		case 4:
			return "Leela";
		case 5:
			return "Princess";
		default:
			return "Who,Now?";
	}
}


Glib::ustring BotFriendly::get_default_name() const
{
	return default_name(_depth);
}


Glib::ustring BotFriendly::get_type_name() const
{
	return "Friendly";
}


BotBase* BotFriendly::clone_for_search() const
{
	BotFriendly *clone = new BotFriendly(_full_depth, TT_CLONE_BITS);
	clone->set_self_penalty(_self_penalty);
	clone->set_friends(get_friends());
	clone->set_enemies(get_enemies());
	return clone;
}
