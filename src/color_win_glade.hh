// generated 2006/2/17 12:05:05 PST by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/color_win.hh and ./src/color_win.cc

#ifndef _COLOR_WIN_GLADE_HH
#  define _COLOR_WIN_GLADE_HH

#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/checkbutton.h>

class color_win_glade : public Gtk::Window
{
protected:
        class Gtk::Label * change_label;
        class Gtk::CheckButton * orange;
        class Gtk::CheckButton * yellow;
        class Gtk::CheckButton * white;
        class Gtk::CheckButton * blue;
        class Gtk::CheckButton * purple;
        class Gtk::CheckButton * black;
        class Gtk::CheckButton * red;
        class Gtk::CheckButton * green;

        color_win_glade();

        ~color_win_glade();
private:
        virtual void on_color_cancel_button_clicked() = 0;
        virtual void on_change_button_clicked() = 0;
};
#endif
