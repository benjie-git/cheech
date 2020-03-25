/*
 *  The Name Choosing Window.
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

#ifndef _NAME_WIN_HH
#  include "name_win_glade.hh"
#  define _NAME_WIN_HH


class name_win : public name_win_glade
{
	public:
		name_win();

		void set_name(Glib::ustring *name, bool force = false);

		sigc::signal<void, Glib::ustring> signal_change_name;

		void on_disconnected();

	private:
        void on_name_entry_activate();
        void on_cancel_button_clicked();
        void on_change_button_clicked();
};
#endif
