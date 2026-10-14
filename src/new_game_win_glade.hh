//
//    The New/Join Game window layout.  Hand-maintained here because the
//    .glade sources are not in the repository.
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef _NEW_GAME_WIN_GLADE_HH
#  define _NEW_GAME_WIN_GLADE_HH

#include <gtkmm/window.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include <gtkmm/frame.h>
#include <gtkmm/notebook.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/expander.h>
#include <gtkmm/scrolledwindow.h>
#include "GtkComboBoxEntryText.hh"
#include "color_picker.hh"


class new_game_win_glade : public Gtk::Window
{
	protected:
		// The New Game page.
		class Gtk::ScrolledWindow *seats_scroll;
		class Gtk::Box *seats_box;
		class Gtk::Button *add_seat_button;
		class Gtk::Button *remove_seat_button;
		class Gtk::CheckButton *long_jumps;
		class Gtk::CheckButton *hop_others;
		class Gtk::CheckButton *stop_others;
		class Gtk::Entry *host_port_entry;
		class Gtk::CheckButton *cheechweb;
		class Gtk::Entry *cheechweb_port_entry;
		class Gtk::Button *host_button;
		class Gtk::Button *host_cancel_button;

		// The Join page.
		class Gtk::Entry *name_entry;
		class ColorPicker *color_box;
		class Gtk::CheckButton *spectator;
		class GtkComboBoxEntryText *join_host_entry;
		class Gtk::Entry *join_port_entry;
		class Gtk::Button *join_button;
		class Gtk::Button *join_cancel_button;

		class Gtk::Notebook *role_notebook;

		new_game_win_glade();

		~new_game_win_glade();
	private:
		virtual void on_add_seat_button_clicked() = 0;
		virtual void on_remove_seat_button_clicked() = 0;
		virtual void on_host_cancel_button_clicked() = 0;
		virtual void on_host_button_clicked() = 0;
		virtual void on_join_cancel_button_clicked() = 0;
		virtual void on_join_button_clicked() = 0;
		virtual void on_cheechweb_toggled() = 0;
};
#endif
