/*
 *   Preferences object that knows how to read and write itself.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef INCL_PREFS_HH
#define INCL_PREFS_HH

#include <glibmm/ustring.h>
#include <vector>


class Prefs
{
	public:
		Prefs();

		void read();
		void write();
		void set_defaults();
		Glib::ustring get_filename();
		void set_join_hostname(Glib::ustring host);


		Glib::ustring name;
		unsigned int color;

		unsigned int host_port;
		bool start_cheechweb;
		unsigned int cheechweb_port;

		unsigned int num_players;
		bool long_jumps;
		bool hop_others;
		bool stop_others;

		std::vector<Glib::ustring> join_hostnames;
		Glib::ustring recent_hostnames;
		unsigned int join_port;
		bool spectator;

		Glib::ustring bot_type;
		unsigned int bot_level;
		unsigned int bot_depth;

		// Saved New Game seats (encoded by new_game_win, one line per seat).
		std::vector<Glib::ustring> seats;
		unsigned int move_delay;
		unsigned int think_delay;
		unsigned int smarts;
};

#endif   // #ifndef INCL_PREFS_HH
