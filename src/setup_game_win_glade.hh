// generated 2006/3/28 12:58:25 PST by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/setup_game_win.hh and ./src/setup_game_win.cc

#ifndef _SETUP_GAME_WIN_GLADE_HH
#  define _SETUP_GAME_WIN_GLADE_HH

#include <gtkmm/window.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>

class setup_game_win_glade : public Gtk::Window
{
protected:
        class Gtk::SpinButton * num_players;
        class Gtk::CheckButton * long_jumps;
        class Gtk::CheckButton * hop_others;
        class Gtk::CheckButton * stop_others;

        setup_game_win_glade();

        ~setup_game_win_glade();
private:
        virtual void on_num_players_activate() = 0;
        virtual void on_cancel_button_clicked() = 0;
        virtual void on_ok_button_clicked() = 0;
};
#endif
