/*
 *   This bot tries to make a good move for him that's bad for everyone else.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _BOT_MEAN_HH
#define _BOT_MEAN_HH

#include "bot_lookahead.hh"


class BotMean : public BotLookAhead
{
	public:
		BotMean(unsigned int depth,
				unsigned int tt_bits = TT_SIZE_BITS);

		virtual Glib::ustring get_default_name() const;
		virtual Glib::ustring get_type_name() const;

		// The default name for a search depth, without instantiating a bot.
		static Glib::ustring default_name(unsigned int depth);

		virtual BotBase* clone_for_search() const;
};

#endif // _BOT_MEAN_HH
