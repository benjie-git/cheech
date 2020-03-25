/*
 *  The New/Join Game Window.
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

#ifndef _NEW_GAME_WIN_HH
#  include "new_game_win_glade.hh"
#  define _NEW_GAME_WIN_HH

#include "prefs.hh"


class new_game_win : public new_game_win_glade
{
	public:
		new_game_win();

		void show_join_page(bool joining);

		sigc::signal<bool, unsigned int, unsigned int,
			unsigned int, bool, bool, bool> signal_start_server;
		sigc::signal<void, Glib::ustring, unsigned int, Glib::ustring,
			unsigned int, bool> signal_start_client;

	private:
		void on_host_cancel_button_clicked();
		void on_host_button_clicked();
		void on_join_cancel_button_clicked();
		void on_join_button_clicked();
		void on_name_entry_activate();
		void on_host_port_entry_activate();
		void on_num_players_activate();
		void on_join_host_entry_activate();
		void on_join_port_entry_activate();
		void on_cheechweb_port_entry_activate();
		void on_cheechweb_toggled();

		unsigned int get_selected_color();
		void set_selected_color(unsigned int color);
};

#endif
