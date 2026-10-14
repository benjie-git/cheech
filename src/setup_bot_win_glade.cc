// generated 2006/4/13 14:42:35 PDT by benjie@paddy.(none)
// using glademm V2.6.0
//
// Converted to gtkmm 4.
//
// Please modify the corresponding derived classes in ./src/setup_bot_win.cc

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
#include "setup_bot_win_glade.hh"
#include <gtkmm/label.h>
#include <gtkmm/grid.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>

setup_bot_win_glade::setup_bot_win_glade(
) : Gtk::Window()
{
   Gtk::Window *setup_bot_win = this;

   Gtk::Label *label59 = Gtk::make_managed<Gtk::Label>(_("Type:"));
   type_box = new GtkComboBoxText();

   Gtk::Grid *table8 = Gtk::make_managed<Gtk::Grid>();
   Gtk::Label *label62 = Gtk::make_managed<Gtk::Label>(_("Move Delay:"));
   Gtk::Label *label63 = Gtk::make_managed<Gtk::Label>(_("Depth:"));
   Gtk::Label *label64 = Gtk::make_managed<Gtk::Label>(_("Smarts:"));

   Glib::RefPtr<Gtk::Adjustment> move_scale_adj =
       Gtk::Adjustment::create(0.40000000596, 0, 2, 0.10000000149, 0.5, 0);
   move_scale = Gtk::make_managed<Gtk::Scale>(move_scale_adj,
       Gtk::Orientation::HORIZONTAL);

   depth_box = new GtkComboBoxText();
   smarts_box = new GtkComboBoxText();

   Gtk::Grid *table9 = Gtk::make_managed<Gtk::Grid>();
   Gtk::Button *defaults = Gtk::make_managed<Gtk::Button>(_("Reset to Defaults"));
   Gtk::Box *hbuttonbox11 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
   hbuttonbox11->set_halign(Gtk::Align::END);
   Gtk::Box *vbox16 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   Gtk::Label *label60 = Gtk::make_managed<Gtk::Label>(_("<b>Configuration</b>"));
   Gtk::Frame *frame13 = Gtk::make_managed<Gtk::Frame>();
   Gtk::Box *vbox15 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   config_table = table9;
   Gtk::Label *label56 = Gtk::make_managed<Gtk::Label>(_("<b>Computer Player</b>"));
   Gtk::Frame *frame12 = Gtk::make_managed<Gtk::Frame>();
   add_button = Gtk::make_managed<Gtk::Button>(_("Add Computer Player"));
   remove_button = Gtk::make_managed<Gtk::Button>(_("Remove Computer Players"));
   Gtk::Button *ok_button = Gtk::make_managed<Gtk::Button>(_("OK"));
   Gtk::Box *hbuttonbox10 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
   hbuttonbox10->set_halign(Gtk::Align::END);
   Gtk::Box *vbox14 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   label59->set_justify(Gtk::Justification::RIGHT);
   table8->set_margin_start(6);
   table8->set_margin_end(6);
   table8->set_margin_top(6);
   table8->set_margin_bottom(6);
   table8->set_row_spacing(3);
   table8->set_column_spacing(3);
   table8->attach(*label59, 0, 0);
   table8->attach(*type_box, 1, 0);
   label62->set_justify(Gtk::Justification::RIGHT);
   label63->set_justify(Gtk::Justification::RIGHT);
   label64->set_justify(Gtk::Justification::RIGHT);
   move_scale->set_digits(3);
   move_scale->set_draw_value(true);
   move_scale->set_value_pos(Gtk::PositionType::TOP);
   table9->attach(*label63, 0, 0);
   table9->attach(*label64, 0, 1);
   table9->attach(*label62, 0, 2);
   table9->attach(*depth_box, 1, 0);
   table9->attach(*smarts_box, 1, 1);
   table9->attach(*move_scale, 1, 2);
   move_scale->set_hexpand(true);
   hbuttonbox11->append(*defaults);
   vbox16->append(*table9);
   vbox16->append(*hbuttonbox11);
   vbox16->set_margin_start(6);
   vbox16->set_margin_end(6);
   vbox16->set_margin_top(6);
   vbox16->set_margin_bottom(6);
   frame13->set_child(*vbox16);
   frame13->set_label_widget(*label60);
   frame13->set_margin_start(6);
   frame13->set_margin_end(6);
   frame13->set_margin_top(6);
   frame13->set_margin_bottom(6);
   vbox15->append(*table8);
   vbox15->append(*frame13);
   label56->set_use_markup(true);
   label60->set_use_markup(true);
   frame12->set_child(*vbox15);
   frame12->set_label_widget(*label56);
   frame12->set_margin_start(3);
   frame12->set_margin_end(3);
   frame12->set_margin_top(3);
   frame12->set_margin_bottom(3);
   hbuttonbox10->append(*add_button);
   hbuttonbox10->append(*remove_button);
   hbuttonbox10->append(*ok_button);
   add_button->set_sensitive(false);
   vbox14->set_margin_start(6);
   vbox14->set_margin_end(6);
   vbox14->set_margin_top(6);
   vbox14->set_margin_bottom(6);
   vbox14->append(*frame12);
   vbox14->append(*hbuttonbox10);
   setup_bot_win->set_title(_("Computer Player Setup"));
   setup_bot_win->set_resizable(true);
   setup_bot_win->set_child(*vbox14);
   set_default_widget(*add_button);
   defaults->signal_clicked().connect(sigc::mem_fun(*this, &setup_bot_win_glade::on_defaults_activate), false);
   add_button->signal_clicked().connect(sigc::mem_fun(*this, &setup_bot_win_glade::on_add_button_activate), false);
   remove_button->signal_clicked().connect(sigc::mem_fun(*this, &setup_bot_win_glade::on_remove_button_activate), false);
   ok_button->signal_clicked().connect(sigc::mem_fun(*this, &setup_bot_win_glade::on_ok_button_activate), false);
}

setup_bot_win_glade::~setup_bot_win_glade()
{  delete type_box;
   delete depth_box;
   delete smarts_box;
}
