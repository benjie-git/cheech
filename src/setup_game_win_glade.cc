// generated 2006/4/13 13:50:00 PDT by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/setup_game_win.cc

#include "config.h"
/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif
#include "setup_game_win_glade.hh"
#include <gtkmm/label.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/grid.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/button.h>

setup_game_win_glade::setup_game_win_glade(
) : Gtk::Window()
{
   Gtk::Window *setup_game_win = this;

   Gtk::Label *label65 = Gtk::make_managed<Gtk::Label>(_("Number of Players:"));
   Glib::RefPtr<Gtk::Adjustment> num_players_adj = Gtk::Adjustment::create(3, 1, 6, 1, 1, 0);
   num_players = Gtk::make_managed<Gtk::SpinButton>(num_players_adj, 1, 0);

   Gtk::Grid *table10 = Gtk::make_managed<Gtk::Grid>();
   long_jumps = Gtk::make_managed<Gtk::CheckButton>(_("Allow Long Jumps (Sue Wen Rules)"));
   hop_others = Gtk::make_managed<Gtk::CheckButton>(_("Allow Hopping Through Other Players' Triangles"));
   stop_others = Gtk::make_managed<Gtk::CheckButton>(_("Allow Stopping In Other Players' Triangles"));

   Gtk::Box *vbox19 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   Gtk::Label *label66 = Gtk::make_managed<Gtk::Label>(_("<b>Rules</b>"));
   Gtk::Frame *frame14 = Gtk::make_managed<Gtk::Frame>();
   Gtk::Button *cancel_button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
   Gtk::Button *button5 = Gtk::make_managed<Gtk::Button>(_("_Restart Game"), true);
   Gtk::Box *hbuttonbox13 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
   Gtk::Box *vbox18 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   num_players->set_wrap(true);
   table10->set_margin_top(6);
   table10->set_margin_bottom(6);
   table10->set_margin_start(6);
   table10->set_margin_end(6);
   table10->set_row_spacing(6);
   table10->set_column_spacing(6);
   table10->attach(*label65, 0, 0);
   table10->attach(*num_players, 1, 0);
   num_players->set_hexpand(true);
   long_jumps->set_active(true);
   hop_others->set_active(true);
   stop_others->set_active(true);
   vbox19->append(*long_jumps);
   vbox19->append(*hop_others);
   vbox19->append(*stop_others);
   label66->set_use_markup(true);
   frame14->set_margin_top(6);
   frame14->set_margin_bottom(6);
   frame14->set_margin_start(6);
   frame14->set_margin_end(6);
   frame14->set_child(*vbox19);
   frame14->set_label_widget(*label66);
   hbuttonbox13->set_halign(Gtk::Align::END);
   hbuttonbox13->set_margin_top(6);
   hbuttonbox13->set_margin_bottom(6);
   hbuttonbox13->set_margin_start(6);
   hbuttonbox13->set_margin_end(6);
   hbuttonbox13->append(*cancel_button);
   hbuttonbox13->append(*button5);
   vbox18->append(*table10);
   vbox18->append(*frame14);
   vbox18->append(*hbuttonbox13);
   setup_game_win->set_title(_("Game Options"));
   setup_game_win->set_resizable(true);
   setup_game_win->set_child(*vbox18);
   num_players->signal_activate().connect(sigc::mem_fun(*this, &setup_game_win_glade::on_num_players_activate), false);
   cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &setup_game_win_glade::on_cancel_button_clicked), false);
   button5->signal_clicked().connect(sigc::mem_fun(*this, &setup_game_win_glade::on_ok_button_clicked), false);
   setup_game_win->set_default_widget(*button5);
}

setup_game_win_glade::~setup_game_win_glade()
{
}
