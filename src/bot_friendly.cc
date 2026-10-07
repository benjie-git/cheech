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


BotFriendly::BotFriendly(unsigned int depth, unsigned int tt_bits) :
	BotLookAhead(depth, tt_bits)
{
	// Friendly halves its own progress: it values the advancement of the
	// players it focuses on as much as its own (see _self_penalty).
	_self_penalty = 2;
}


Glib::ustring BotFriendly::get_default_name() const
{
	switch (_depth)
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
