// generated 2006/2/17 12:05:05 PST by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/help_win.hh and ./src/help_win.cc

#ifndef _HELP_WIN_GLADE_HH
#  define _HELP_WIN_GLADE_HH

#include <gtkmm/window.h>

class help_win_glade : public Gtk::Window
{
protected:
        help_win_glade();

        ~help_win_glade();
private:
        virtual void on_close_button_clicked() = 0;
};
#endif
