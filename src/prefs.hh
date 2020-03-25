/*
 *  Preferences object that knows how to read and write itself.
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
		unsigned int move_delay;
		unsigned int done_delay;
		unsigned int think_delay;
};

#endif   // #ifndef INCL_PREFS_HH
