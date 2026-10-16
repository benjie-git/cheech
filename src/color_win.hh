/*
 *   The Peg Color Choosing Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _COLOR_WIN_HH
#  include "color_win_glade.hh"
#  define _COLOR_WIN_HH


class color_win : public color_win_glade
{
	public:
		color_win();

		void set_force_info(Glib::ustring *name, int color);

		sigc::signal<void(int)> signal_change_color;

		void on_disconnected();

	private:
		void on_color_cancel_button_clicked();
		void on_change_button_clicked();
		int get_selected_color();
};

#endif  //_COLOR_WIN_HH
