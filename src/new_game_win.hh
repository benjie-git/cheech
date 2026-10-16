//
//    The New/Join Game window logic.
//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//

#ifndef _NEW_GAME_WIN_HH
#  include "new_game_win_glade.hh"
#  define _NEW_GAME_WIN_HH

#include <vector>
#include <map>
#include <gtkmm/separator.h>
#include "bot_catalog.hh"
#include "color_picker.hh"
#include "combo_drop_down.hh"
#include "prefs.hh"


// One seat on the New Game page.  kind: 0 = human, 1 = computer, 2 = remote.
// For computer seats family is Friendly/Neutral/Mean, level 1..4 (Best..
// Nerfed) and depth 0 (Default) or 2..5.  focus_all means "every other
// player"; otherwise focus_ids lists the seat ids the bot focuses on.
struct NewGameSeat
{
	int kind;
	Glib::ustring name;
	// The name the seat had while it was Human (kind 0), remembered for
	// computer seats so switching back to Human restores it (like the iOS
	// app's SeatConfig.humanName).  Empty for seats that were never Human.
	Glib::ustring human_name;
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
		~new_game_win();

		void show_join_page(bool joining);

		sigc::signal<bool(unsigned int, unsigned int,
			bool, bool, bool, std::vector<NewGameSeat>)> signal_start_server;
		sigc::signal<void(Glib::ustring, unsigned int, Glib::ustring,
			unsigned int, bool)> signal_start_client;

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
			Gtk::Box *box;
			ComboDropDown *kind_box;
			ComboDropDown *family_box;
			Gtk::Entry *name_entry;
			ColorPicker *color_picker;
			Gtk::Expander *details;
			ComboDropDown *depth_box;
			ComboDropDown *smarts_box;
			Gtk::Box *focus_box;
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
		void on_join_host_icon_pressed(Gtk::Entry::IconPosition icon_pos);
		void on_join_host_picked(Gtk::ListBoxRow *row);
		bool on_join_host_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
		void on_join_host_delete_selected();
		void refresh_join_hostnames();
		void on_join_button_clicked();
		void on_cheechweb_toggled();

		void on_seat_kind_changed(SeatRow *row);
		void on_seat_family_changed(SeatRow *row);
		void on_seat_name_changed(SeatRow *row);
		void on_seat_color_changed(unsigned int color, SeatRow *row);
		void on_seat_depth_changed(SeatRow *row);
		void on_seat_smarts_changed(SeatRow *row);

		void add_seat(const NewGameSeat &seat);
		void remove_seat(size_t index);
		SeatRow *find_row(unsigned int id);
		void sync_seat_row(SeatRow &row);
		void remember_last_bot(SeatRow *row);
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
