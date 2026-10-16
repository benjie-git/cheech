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
#include <giomm/resource.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>
#include <gtkmm/picture.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>

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

   // Show the app icon at the top.  It is compiled into the binary as a
   // GResource (resources/cheech.gresource.xml), so this works the same on
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

   // The acknowledgements/licence text is also compiled in as a
   // GResource, for the same reason.  It holds Pango markup so its
   // links are clickable, just like the About page's.
   Glib::ustring acknowledgements;
   try {
      Glib::RefPtr<const Glib::Bytes> data =
         Gio::Resource::lookup_data_global("/cheech/acknowledgements.pango");
      gsize size = 0;
      auto text = static_cast<const char*>(data->get_data(size));
      acknowledgements.assign(text, text + size);
   } catch (const Glib::Error&) {
      // The binary is mis-built; better to say so than to show nothing.
      acknowledgements = _("The acknowledgements could not be loaded.");
   }
   Gtk::Label *ack_label = Gtk::make_managed<Gtk::Label>();
   ack_label->set_use_markup(true);
   ack_label->set_markup(acknowledgements);
   ack_label->set_wrap(true);
   // Break long words (like URLs) when needed.
   ack_label->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
   ack_label->set_xalign(0.0);
   ack_label->set_selectable(true);
   ack_label->set_margin_start(8);
   ack_label->set_margin_end(8);
   ack_label->set_margin_top(8);
   ack_label->set_margin_bottom(8);
   Gtk::ScrolledWindow *ack_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
   // Fixed viewport width; the label wraps and the page scrolls down only.
   ack_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
   ack_scroll->set_child(*ack_label);

   Gtk::Notebook *notebook = Gtk::make_managed<Gtk::Notebook>();
   notebook->set_expand(true);
   notebook->append_page(*vbox9, _("About"));
   notebook->append_page(*ack_scroll, _("Acknowledgements"));

   Gtk::Box *top_vbox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   top_vbox->append(*notebook);
   top_vbox->append(*hbuttonbox6);

   about_win->set_title(_("About Cheech"));
   about_win->set_modal(false);
   about_win->set_resizable(true);
   about_win->set_default_size(420, 420);
   about_win->set_child(*top_vbox);
   close_button->signal_clicked().connect(sigc::mem_fun(*this, &about_win_glade::on_close_button_clicked));
}

about_win_glade::~about_win_glade()
{  }
