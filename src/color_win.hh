/*
 *  The Peg Color Choosing Window.
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

#ifndef _COLOR_WIN_HH
#  include "color_win_glade.hh"
#  define _COLOR_WIN_HH


class color_win : public color_win_glade
{
	public:
		color_win();

		void set_force_info(Glib::ustring *name, int color);

		sigc::signal<void, int> signal_change_color;

		void on_disconnected();

	private:
		void on_color_cancel_button_clicked();
		void on_change_button_clicked();
		int get_selected_color();
};

#endif  //_COLOR_WIN_HH
