//
//    Copyright 2005-2007, ... (kept from original header)
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

#ifndef _NEW_GAME_WIN_HH
#  include "new_game_win_glade.hh"
#  define _NEW_GAME_WIN_HH

#include <vector>
#include <map>
#include <gtkmm/separator.h>
#include "bot_catalog.hh"
#include "color_picker.hh"
#include "prefs.hh"


// One seat on the New Game page.  kind: 0 = human, 1 = computer, 2 = remote.
// For computer seats family is Friendly/Neutral/Mean, level 1..4 (Best..
// Nerfed) and depth 0 (Default) or 2..5.  focus_all means "every other
// player"; otherwise focus_ids lists the seat ids the bot focuses on.
struct NewGameSeat
{
	int kind;
	Glib::ustring name;
	unsigned int color;
	Glib::ustring family;
	unsigned int level;
	unsigned int depth;
	bool focus_all;
	std::vector<unsigned int> focus_ids;
	unsigned int id;
};


class new_game_win : public new_game_win_glade
{
	public:
		new_game_win();

		void show_join_page(bool joining);

		sigc::signal<bool, unsigned int, unsigned int,
			bool, bool, bool, std::vector<NewGameSeat> > signal_start_server;
		sigc::signal<void, Glib::ustring, unsigned int, Glib::ustring,
			unsigned int, bool> signal_start_client;

	private:
		// The widgets of one seat row.  Rows are created plain-new (their
		// children are Gtk::manage'd) and deleted on removal.
		struct SeatRow
		{
			unsigned int id;
			unsigned int color;
			// A thin line between this row and the one above (NULL for the
			// first seat).  Plain-new and deleted with the row.
			Gtk::Separator *sep;
			Gtk::VBox *box;
			Gtk::ComboBoxText *kind_box;
			Gtk::ComboBoxText *family_box;
			Gtk::Entry *name_entry;
			ColorPicker *color_picker;
			Gtk::Expander *details;
			Gtk::ComboBoxText *depth_box;
			Gtk::ComboBoxText *smarts_box;
			Gtk::HBox *focus_box;
			std::vector<Gtk::CheckButton *> focus_checks;
			// The seat ids the checkboxes were built for, parallel to
			// focus_checks, so a rebuilt row keeps each seat's own state.
			std::vector<unsigned int> focus_check_ids;
			// Focus state carried from load_seats(): applied the first time
			// this row's focus checkboxes are built (0-based seat indices).
			bool focus_pending;
			std::vector<unsigned int> focus_pending_ids;
		};

		void on_add_seat_button_clicked();
		void on_remove_seat_button_clicked();
		void on_host_cancel_button_clicked();
		void on_host_button_clicked();
		void on_join_cancel_button_clicked();
		void on_join_button_clicked();
		void on_cheechweb_toggled();

		void on_seat_kind_changed(SeatRow *row);
		void on_seat_family_changed(SeatRow *row);
		void on_seat_name_changed(SeatRow *row);
		void on_seat_color_changed(unsigned int color, SeatRow *row);
		void on_seat_depth_changed(SeatRow *row);

		void add_seat(const NewGameSeat &seat);
		void remove_seat(size_t index);
		SeatRow *find_row(unsigned int id);
		void sync_seat_row(SeatRow &row);
		void refresh_focus_rows();
		Glib::ustring other_label(SeatRow *row);
		Glib::ustring default_name_for(SeatRow *row);
		std::vector<NewGameSeat> collect_seats();
		unsigned int first_unused_color() const;

		unsigned int get_selected_color() const;
		void set_selected_color(unsigned int color);

		void load_seats();
		void save_seats();

		std::vector<SeatRow *> _rows;
		unsigned int _next_seat_id;
		Glib::ustring _player_name;
		// Each seat remembers the name it had as a Human (like the iOS app's
		// humanName), and the cute default it last showed, so typed names
		// survive kind/family/depth changes.
		std::map<unsigned int, Glib::ustring> _human_names;
		std::map<unsigned int, Glib::ustring> _default_names;
		// True while add_seat() populates a row's combos, so the changed
		// handlers do not treat loaded values as user edits.
		bool _loading_seat;
};

#endif
