// generated 2006/4/13 13:56:33 PDT by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/color_win.cc

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
#include "color_win_glade.hh"
#include <gtkmm/checkbutton.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/frame.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

color_win_glade::color_win_glade(
) : Gtk::Window()
{
   Gtk::Window *color_win = this;
   change_label = Gtk::make_managed<Gtk::Label>(_("<b> %1 is already using %2.\n"
		"Please choose a differnt color.</b>"));

   orange = Gtk::make_managed<Gtk::CheckButton>(_("Orange"));
   yellow = Gtk::make_managed<Gtk::CheckButton>(_("Yellow"));
   yellow->set_group(*orange);
   white = Gtk::make_managed<Gtk::CheckButton>(_("White"));
   white->set_group(*orange);
   blue = Gtk::make_managed<Gtk::CheckButton>(_("Blue"));
   blue->set_group(*orange);
   purple = Gtk::make_managed<Gtk::CheckButton>(_("Purple"));
   purple->set_group(*orange);
   black = Gtk::make_managed<Gtk::CheckButton>(_("Black"));
   black->set_group(*orange);
   red = Gtk::make_managed<Gtk::CheckButton>(_("Red"));
   red->set_group(*orange);
   green = Gtk::make_managed<Gtk::CheckButton>(_("Green"));
   green->set_group(*orange);

   Gtk::Grid *table7 = Gtk::make_managed<Gtk::Grid>();
   Gtk::Label *label29 = Gtk::make_managed<Gtk::Label>(_("<b>Color</b>"));
   Gtk::Frame *frame9 = Gtk::make_managed<Gtk::Frame>();
   Gtk::Button *color_cancel_button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
   Gtk::Button *change_button = Gtk::make_managed<Gtk::Button>(_("_Change Color"), true);
   Gtk::Box *hbuttonbox7 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
   Gtk::Box *vbox10 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
   change_label->set_use_markup(true);
   red->set_active(true);
   table7->attach(*red, 0, 0);
   table7->attach(*orange, 1, 0);
   table7->attach(*yellow, 2, 0);
   table7->attach(*green, 3, 0);
   table7->attach(*blue, 0, 1);
   table7->attach(*purple, 1, 1);
   table7->attach(*black, 2, 1);
   table7->attach(*white, 3, 1);
   label29->set_use_markup(true);
   frame9->set_margin_top(3);
   frame9->set_margin_bottom(3);
   frame9->set_margin_start(3);
   frame9->set_margin_end(3);
   frame9->set_child(*table7);
   frame9->set_label_widget(*label29);
   hbuttonbox7->set_halign(Gtk::Align::END);
   hbuttonbox7->set_margin_top(6);
   hbuttonbox7->set_margin_bottom(6);
   hbuttonbox7->set_margin_start(6);
   hbuttonbox7->set_margin_end(6);
   hbuttonbox7->append(*color_cancel_button);
   hbuttonbox7->append(*change_button);
   vbox10->set_margin_top(3);
   vbox10->set_margin_bottom(3);
   vbox10->set_margin_start(3);
   vbox10->set_margin_end(3);
   vbox10->append(*change_label);
   vbox10->append(*frame9);
   vbox10->append(*hbuttonbox7);
   color_win->set_title(_("Change Your Color"));
   color_win->set_resizable(true);
   color_win->set_child(*vbox10);
   color_cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &color_win_glade::on_color_cancel_button_clicked), false);
   change_button->signal_clicked().connect(sigc::mem_fun(*this, &color_win_glade::on_change_button_clicked), false);
   color_win->set_default_widget(*change_button);
}

color_win_glade::~color_win_glade()
{
}
