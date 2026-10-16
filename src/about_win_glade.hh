//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//
//    The About window (GTK4 version; originally glademm-generated).
//

#ifndef _ABOUT_WIN_GLADE_HH
#  define _ABOUT_WIN_GLADE_HH

#include <gtkmm/window.h>

class about_win_glade : public Gtk::Window
{
	protected:

		about_win_glade();

		~about_win_glade();
private:
		virtual void on_close_button_clicked() = 0;
};
#endif
