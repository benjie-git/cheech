// generated 2006/4/4 19:20:50 EST by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/name_win.hh and ./src/name_win.cc

#ifndef _NAME_WIN_GLADE_HH
#  define _NAME_WIN_GLADE_HH

#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>

class name_win_glade : public Gtk::Window
{
protected:
        class Gtk::Label * change_label;
        class Gtk::Entry * name_entry;

        name_win_glade();

        ~name_win_glade();
private:
        virtual void on_name_entry_activate() = 0;
        virtual void on_cancel_button_clicked() = 0;
        virtual void on_change_button_clicked() = 0;
};
#endif
