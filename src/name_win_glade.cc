// generated 2006/4/13 13:56:33 PDT by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/name_win.cc

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
#include "name_win_glade.hh"
#include <gtkmm/box.h>
#include <gtkmm/button.h>

name_win_glade::name_win_glade(
) : Gtk::Window()
{
   Gtk::Window *name_win = this;
   change_label = Gtk::make_managed<Gtk::Label>(_("<b>The name %1 is already in use.\n"
		"Please choose a differnt name.</b>"));

   Gtk::Label *label53 = Gtk::make_managed<Gtk::Label>(_("Name:"));
   name_entry = Gtk::make_managed<Gtk::Entry>();

   Gtk::Box *hbox7 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
   Gtk::Button *cancel_button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
   Gtk::Button *change_button = Gtk::make_managed<Gtk::Button>(_("_Change Name"), true);
   Gtk::Box *hbuttonbox9 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
   Gtk::Box *vbox13 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
   change_label->set_use_markup(true);
   name_entry->set_hexpand(true);
   hbox7->append(*label53);
   hbox7->append(*name_entry);
   hbuttonbox9->set_halign(Gtk::Align::END);
   hbuttonbox9->append(*cancel_button);
   hbuttonbox9->append(*change_button);
   vbox13->set_margin_top(6);
   vbox13->set_margin_bottom(6);
   vbox13->set_margin_start(6);
   vbox13->set_margin_end(6);
   vbox13->append(*change_label);
   vbox13->append(*hbox7);
   vbox13->append(*hbuttonbox9);
   name_win->set_title(_("Change Your Name"));
   name_win->set_resizable(true);
   name_win->set_child(*vbox13);
   name_entry->signal_activate().connect(sigc::mem_fun(*this, &name_win_glade::on_name_entry_activate), false);
   cancel_button->signal_clicked().connect(sigc::mem_fun(*this, &name_win_glade::on_cancel_button_clicked), false);
   change_button->signal_clicked().connect(sigc::mem_fun(*this, &name_win_glade::on_change_button_clicked), false);
   name_win->set_default_widget(*change_button);
}

name_win_glade::~name_win_glade()
{
}
