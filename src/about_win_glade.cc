//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//
//    The About window (GTK4 version; originally glademm-generated).
//
//    Please modify the corresponding derived classes in ./src/about_win.hh and ./src/about_win.cc
//

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
#include "about_win_glade.hh"
#include <gdkmm/pixbuf.h>
#include <gtkmm/label.h>
#include <gtkmm/picture.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

about_win_glade::about_win_glade()
{
   Gtk::Window *about_win = this;

   Gtk::Label *label12 = Gtk::make_managed<Gtk::Label>(
      Glib::ustring::compose(_("<b>cheech v%1 -- Chinese Checkers</b>\ncheech is a multiplatform, networked implementation of the classic game Chinese Checkers, with multiple computer players included.  It allows for a few common rule variations.  The 'allow long jumps' rule makes the game considerably more interesting.  Enjoy!\n"
		"\n"
		"<a href=\"https://github.com/benjie-git/cheech\">https://github.com/benjie-git/cheech</a>\n"
		"\n"
		"2005-2026 Ben Levitt"), VERSION));
   Gtk::Button *close_button = Gtk::make_managed<Gtk::Button>(_("_Close"), true);
   Gtk::Box *hbuttonbox6 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
   Gtk::Box *vbox9 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   label12->set_justify(Gtk::Justification::CENTER);
   label12->set_wrap(true);
   // Break long words (like the URL) when needed, so no single word can
   // force the window wider than its default size below.
   label12->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
   label12->set_use_markup(true);
   label12->set_xalign(0.5);
   hbuttonbox6->set_margin_start(6);
   hbuttonbox6->set_margin_end(6);
   hbuttonbox6->set_margin_top(6);
   hbuttonbox6->set_margin_bottom(6);
   hbuttonbox6->set_halign(Gtk::Align::END);
   hbuttonbox6->append(*close_button);
   // Show the app icon at the top.  It is compiled into the binary as a
   // GResource (pixmaps/cheech.gresource.xml), so this works the same on
   // every platform with no path lookup.
   try {
      Glib::RefPtr<Gdk::Pixbuf> icon =
         Gdk::Pixbuf::create_from_resource("/cheech/cheech.png");
      Gtk::Picture *icon_picture = Gtk::make_managed<Gtk::Picture>(icon);
      icon_picture->set_can_shrink(false);
      icon_picture->set_halign(Gtk::Align::CENTER);
      icon_picture->set_margin_bottom(6);
      vbox9->append(*icon_picture);
   } catch (const Glib::Error&) {
      // A missing or unreadable icon is not worth failing over.
   }
   vbox9->append(*label12);
   vbox9->append(*hbuttonbox6);
   about_win->set_title(_("About Cheech"));
   about_win->set_modal(false);
   about_win->set_resizable(true);
   about_win->set_default_size(320, 350);
   about_win->set_child(*vbox9);
   close_button->signal_clicked().connect(sigc::mem_fun(*this, &about_win_glade::on_close_button_clicked));
}

about_win_glade::~about_win_glade()
{  }
