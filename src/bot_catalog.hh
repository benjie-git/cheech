//
//    Shared bot presentation helpers for the setup dialogs: family labels
//    as shown to users (Neutral = LookAhead), fixed skill levels, and depth
//    handling.  Mirrors the iOS BotCatalog/BotSkill.
//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//

#ifndef _BOT_CATALOG_HH
#define _BOT_CATALOG_HH

#include <vector>
#include <stdlib.h>

#include <glibmm/ustring.h>

#include "bot_base.hh"
#include "bot_lookahead.hh"
#include "bot_friendly.hh"
#include "bot_mean.hh"
#include "utility.hh"


namespace BotUI
{

const Glib::ustring FRIENDLY = "Friendly";
const Glib::ustring NEUTRAL = "Neutral";
const Glib::ustring MEAN = "Mean";


// The engine prefix a family label stands for.
inline Glib::ustring family_prefix(const Glib::ustring &family)
{
	if (family == NEUTRAL)
		return "LookAhead";
	return family;
}


// The user-facing family label for a bot type, e.g. "LookAhead(3)" ->
// "Neutral".  Family labels pass through unchanged.
inline Glib::ustring family_label(const Glib::ustring &type)
{
	Glib::ustring prefix = type.substr(0, type.find("("));

	if (prefix == "LookAhead")
		return NEUTRAL;
	if (prefix == "Mean" || prefix == "Friendly")
		return prefix;
	return type;
}


// The family's usual depth, as offered in the simplified pickers.
inline unsigned int default_depth(const Glib::ustring &family)
{
	if (family == FRIENDLY)
		return 4;
	if (family == NEUTRAL)
		return 3;
	if (family == MEAN)
		return 4;
	return 3;
}


// Replaces the trailing "(N)" depth of a type, e.g. "Mean(4)" -> "Mean(2)".
// A bare family prefix gets the depth appended, e.g. "Mean" -> "Mean(4)".
inline Glib::ustring with_depth(const Glib::ustring &type, unsigned int depth)
{
	Glib::ustring::size_type open = type.find("(");

	if (open == Glib::ustring::npos)
		return type + "(" + util::to_str(depth) + ")";
	return type.substr(0, open) + "(" + util::to_str(depth) + ")";
}


// The depth dropdown: 0 = Default (the family's usual depth, including the
// single-friend Friendly step-down); 2..5 pin the depth.
inline std::vector<unsigned int> depth_values()
{
	std::vector<unsigned int> values;

	values.push_back(0);
	values.push_back(2);
	values.push_back(3);
	values.push_back(4);
	values.push_back(5);
	return values;
}


inline Glib::ustring depth_label(unsigned int depth)
{
	if (depth == 0)
		return "Default";
	if (depth == 5)
		return "5 (Slow)";
	return util::to_str(depth);
}


// The depth a setting stands for.  Default resolves to the family's usual
// depth, stepped down to 3 for a Friendly bot that helps exactly one other
// player (a depth-4 cooperative plan optimises the friend's second move
// instead of finding the friend good first moves).
inline unsigned int resolved_depth(const Glib::ustring &family,
	unsigned int depth, unsigned int friend_count)
{
	if (depth != 0)
		return depth;

	unsigned int resolved = default_depth(family);

	if (family == FRIENDLY && friend_count == 1 && resolved > 3)
		resolved = 3;
	return resolved;
}


// The full engine type for a family/depth/friend-count combination.
inline Glib::ustring bot_type(const Glib::ustring &family,
	unsigned int depth, unsigned int friend_count)
{
	return with_depth(family_prefix(family),
		resolved_depth(family, depth, friend_count));
}


inline std::vector<unsigned int> skill_levels()
{
	std::vector<unsigned int> levels;

	levels.push_back(1);
	levels.push_back(2);
	levels.push_back(3);
	levels.push_back(4);
	return levels;
}


inline Glib::ustring skill_label(unsigned int level)
{
	switch (level)
	{
		case 2:
			return "Great";
		case 3:
			return "Mid";
		case 4:
			return "Nerfed";
		case 1:
		default:
			return "Best";
	}
}


// Smarts percentage for a skill level.  With an explicit Depth dropdown
// there is no hidden depth swap, so every family uses the same ladder.
inline unsigned int skill_smarts(unsigned int level)
{
	switch (level)
	{
		case 2:
			return 90;
		case 3:
			return 80;
		case 4:
			return 70;
		case 1:
		default:
			return 100;
	}
}


// The search depth in a bot type string, e.g. Friendly(3) -> 3.  Types
// without a depth report 0.
inline unsigned int type_depth(const Glib::ustring &type)
{
	size_t paren = type.find("(");

	if (paren == Glib::ustring::npos)
		return 0;
	return atoi(type.substr(paren + 1).c_str());
}


// The bot's default (cute) name for a type, e.g. "Unicorn" for Friendly(3).
// The catalog is what maps a family to its bot class; the base class and
// the classes themselves know no other bots.
inline Glib::ustring cute_name(const Glib::ustring &type)
{
	unsigned int depth = type_depth(type);
	Glib::ustring family = family_label(type);

	if (family == FRIENDLY)
		return BotFriendly::default_name(depth);
	else if (family == MEAN)
		return BotMean::default_name(depth);
	else if (family == NEUTRAL)
		return BotLookAhead::default_name(depth);

	return BotBase::default_name(depth);
}

}

#endif
