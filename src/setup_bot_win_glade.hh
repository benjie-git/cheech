// generated 2006/4/10 18:28:01 EST by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// Converted to gtkmm 4.
//
// Please modify the corresponding derived classes in ./src/setup_bot_win.hh and./src/setup_bot_win.cc

#ifndef _SETUP_BOT_WIN_GLADE_HH
#  define _SETUP_BOT_WIN_GLADE_HH

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/grid.h>
#include "GtkComboBoxText.hh"
#include <gtkmm/scale.h>
#include <gtkmm/button.h>

class setup_bot_win_glade : public Gtk::Window
{
protected:
        class GtkComboBoxText * type_box;
        class GtkComboBoxText * depth_box;
        class GtkComboBoxText * smarts_box;
        class Gtk::Button * add_button;
        class Gtk::Button * remove_button;
        // The Configuration grid; the friend/enemy focus row is added to it
        // dynamically so it lines up with the other config labels.
        class Gtk::Grid * config_table;

        setup_bot_win_glade();

        ~setup_bot_win_glade();
private:
        virtual void on_defaults_activate() = 0;
        virtual void on_add_button_activate() = 0;
        virtual void on_remove_button_activate() = 0;
        virtual void on_ok_button_activate() = 0;
};
#endif
