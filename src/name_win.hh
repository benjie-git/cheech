/*
 *   The Name Choosing Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _NAME_WIN_HH
#  include "name_win_glade.hh"
#  define _NAME_WIN_HH


class name_win : public name_win_glade
{
	public:
		name_win();

		void set_name(Glib::ustring *name, bool force = false);

		sigc::signal<void(Glib::ustring)> signal_change_name;

		void on_disconnected();

	private:
        void on_name_entry_activate();
        void on_cancel_button_clicked();
        void on_change_button_clicked();
};
#endif
