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
#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

about_win_glade::about_win_glade()
{
   Gtk::Window *about_win = this;

   Gtk::Label *label12 = Gtk::make_managed<Gtk::Label>(_("<b>cheech</b> is a multiplatform, networked implementation of the classic game Chinese Checkers.  It allows for some of the most common rule varriations that I've seen used.  I personally think the 'long jumps' rule makes the game infinitely more interesting.  :)\n"
		"\n"
		"https://github.com/benjie-git/cheech\n"
		"\n"
		"2005-2026 Ben Levitt"));
   Gtk::Button *close_button = Gtk::make_managed<Gtk::Button>(_("_Close"), true);
   Gtk::Box *hbuttonbox6 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
   Gtk::Box *vbox9 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   label12->set_justify(Gtk::Justification::CENTER);
   label12->set_wrap(true);
   label12->set_use_markup(true);
   hbuttonbox6->set_margin_start(6);
   hbuttonbox6->set_margin_end(6);
   hbuttonbox6->set_margin_top(6);
   hbuttonbox6->set_margin_bottom(6);
   hbuttonbox6->set_halign(Gtk::Align::END);
   hbuttonbox6->append(*close_button);
   vbox9->append(*label12);
   vbox9->append(*hbuttonbox6);
   about_win->set_title(_("About cheech 0.9 -- Chinese Checkers"));
   about_win->set_modal(false);
   about_win->set_resizable(true);
   about_win->set_child(*vbox9);
   close_button->signal_clicked().connect(sigc::mem_fun(*this, &about_win_glade::on_close_button_clicked));
}

about_win_glade::~about_win_glade()
{  }
