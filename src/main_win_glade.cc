// generated 2006/4/7 0:19:22 EST by benjie@paddy.(none)
// using glademm V2.6.0
//
// Converted to gtkmm 4.
//
// Please modify the corresponding derived classes in ./src/main_win.cc

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
#include "main_win_glade.hh"
#include <cmath>
#include <gtk/gtk.h>
#include <giomm/menu.h>
#include <gtkmm/popovermenubar.h>
#include <gtkmm/overlay.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/grid.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/paned.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/stylecontext.h>
#include <gdkmm/display.h>
#include "game_images.hh"


static const double PEG_SWATCH_PI = 3.14159265358979;


PegColorSwatch::PegColorSwatch() : _color(0)
{
   set_size_request(18, 18);
   set_draw_func(sigc::mem_fun(*this, &PegColorSwatch::on_draw));
}

void PegColorSwatch::set_color(int color)
{
   _color = color;
   queue_draw();
}

void PegColorSwatch::clear()
{
   _color = 0;
   queue_draw();
}

void PegColorSwatch::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int,
   int)
{
   if (_color <= 0)
      return;

   double w = get_width();
   double h = get_height();
   double r = (w < h ? w : h) / 2.0 - 1.0;
   if (r <= 0)
      return;
   double cx = w / 2.0;
   double cy = h / 2.0;

   const GameColor fill = GameImages::get_peg_fill(_color);
   const GameColor edge = GameImages::get_peg_edge(_color);

   cr->set_source_rgb(edge.r, edge.g, edge.b);
   cr->arc(cx, cy, r, 0, 2 * PEG_SWATCH_PI);
   cr->fill();

   cr->set_source_rgb(fill.r, fill.g, fill.b);
   cr->arc(cx, cy, r * 0.88, 0, 2 * PEG_SWATCH_PI);
   cr->fill();

   cr->set_source_rgba(1.0, 1.0, 1.0, 0.35);
   cr->arc(cx - r * 0.28, cy - r * 0.34, r * 0.30, 0, 2 * PEG_SWATCH_PI);
   cr->fill();
}

main_win_glade::main_win_glade(
) : Gtk::ApplicationWindow()
{
   game_view = new class game_view();

   Gtk::Overlay *overlay1 = Gtk::make_managed<Gtk::Overlay>();

   Gtk::Box *vbox13 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
   Gtk::Box *vbox12 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
   Gtk::Label *label49 = Gtk::make_managed<Gtk::Label>(_("<b>Players</b>"));
   player_name1 = Gtk::make_managed<Gtk::Label>(_(""));
   player_name2 = Gtk::make_managed<Gtk::Label>(_(""));
   player_name3 = Gtk::make_managed<Gtk::Label>(_(""));
   player_name4 = Gtk::make_managed<Gtk::Label>(_(""));
   player_name5 = Gtk::make_managed<Gtk::Label>(_(""));
   player_name6 = Gtk::make_managed<Gtk::Label>(_(""));
   player_peg1 = new class PegColorSwatch();
   player_peg2 = new class PegColorSwatch();
   player_peg3 = new class PegColorSwatch();
   player_peg4 = new class PegColorSwatch();
   player_peg5 = new class PegColorSwatch();
   player_peg6 = new class PegColorSwatch();

   Gtk::Grid *table1 = Gtk::make_managed<Gtk::Grid>();
   Gtk::Label *label47 = Gtk::make_managed<Gtk::Label>(_("<b>Move</b>"));
   move_counter = Gtk::make_managed<Gtk::Label>(_(""));

   message_view = Gtk::make_managed<Gtk::TextView>();

   Gtk::ScrolledWindow *scrolledwindow1 = Gtk::make_managed<Gtk::ScrolledWindow>();
   chat_entry = Gtk::make_managed<Gtk::Entry>();

   chat_area = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   Gtk::Paned *vpaned1 = Gtk::make_managed<Gtk::Paned>(Gtk::Orientation::VERTICAL);
   Gtk::Statusbar *statusbar = Gtk::make_managed<Gtk::Statusbar>();
   Gtk::Box *vbox1 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);

   // The menu is a Gio::Menu model; the win.* actions are registered by
   // main_win and the accelerators are set on the Gtk::Application.
   Glib::RefPtr<Gio::Menu> restart_submenu_menu = Gio::Menu::create();
   restart_submenu_menu->append(_("Rotate Players"), "win.rotate");
   restart_submenu_menu->append(_("Shuffle Players"), "win.shuffle");
   restart_submenu_menu->append(_("Keep Same Order"), "win.restart_game");

   Glib::RefPtr<Gio::Menu> menuitem1_menu = Gio::Menu::create();
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("New Game..."), "win.new_game");
      menuitem1_menu->append_section(section);
   }
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Stop Hosting"), "win.end_game");
      section->append(_("Leave Board"), "win.leave_game");
      menuitem1_menu->append_section(section);
   }
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Change Game Settings..."), "win.game_settings");
      Glib::RefPtr<Gio::MenuItem> restart_item =
          Gio::MenuItem::create(_("Restart Game"), restart_submenu_menu);
      section->append_item(restart_item);
      show_chat_item = Gio::MenuItem::create(_("Show Chat"), "win.show_chat");
      show_chat_section = section;
      section->append_item(show_chat_item);
      menuitem1_menu->append_section(section);
   }
#ifndef MACOS_APP
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Quit"), "win.quit");
      menuitem1_menu->append_section(section);
   }
#endif

   Glib::RefPtr<Gio::Menu> edit_menu = Gio::Menu::create();
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Undo"), "win.undo");
      section->append(_("Redo"), "win.redo");
      section->append(_("Show Last Move"), "win.show_last_move");
      edit_menu->append_section(section);
   }
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Cut"), "win.cut");
      section->append(_("Copy"), "win.copy");
      section->append(_("Paste"), "win.paste");
      section->append(_("Delete"), "win.delete");
      edit_menu->append_section(section);
   }

   Glib::RefPtr<Gio::Menu> player_menu = Gio::Menu::create();
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Change Name..."), "win.change_name");
      section->append(_("Change Color..."), "win.change_color");
      player_menu->append_section(section);
   }
   {
      Glib::RefPtr<Gio::Menu> section = Gio::Menu::create();
      section->append(_("Add Computer Player"), "win.add_computer_player");
      section->append(_("Remove Computer Players"),
          "win.remove_computer_players");
      section->append(_("Setup Computer Player..."),
          "win.setup_computer_player");
      player_menu->append_section(section);
   }

   Glib::RefPtr<Gio::Menu> menuitem4_menu = Gio::Menu::create();
   menuitem4_menu->append(_("How to Play"), "win.how_to_play");
   menuitem4_menu->append(_("About"), "win.about");

   menu_model = Gio::Menu::create();
   {
      Glib::RefPtr<Gio::MenuItem> item(
          Gio::MenuItem::create(_("Game"), ""));
      item->set_submenu(menuitem1_menu);
      menu_model->append_item(item);
   }
   {
      Glib::RefPtr<Gio::MenuItem> item(
          Gio::MenuItem::create(_("Edit"), ""));
      item->set_submenu(edit_menu);
      menu_model->append_item(item);
   }
   {
      Glib::RefPtr<Gio::MenuItem> item(
          Gio::MenuItem::create(_("Player"), ""));
      item->set_submenu(player_menu);
      menu_model->append_item(item);
   }
   {
      Glib::RefPtr<Gio::MenuItem> item(
          Gio::MenuItem::create(_("Help"), ""));
      item->set_submenu(menuitem4_menu);
      menu_model->append_item(item);
   }
#ifdef MACOS_APP
   // The model is exported to the native macOS menu bar by
   // Gtk::Application::set_menubar(); no in-window bar is created.
   menubar1 = nullptr;
#else
   menubar1 = Gtk::make_managed<Gtk::PopoverMenuBar>(menu_model);
#endif

   label49->set_use_markup(true);
   player_name1->set_xalign(0);
   player_name2->set_xalign(0);
   player_name3->set_xalign(0);
   player_name4->set_xalign(0);
   player_name5->set_xalign(0);
   player_name6->set_xalign(0);
   table1->set_row_spacing(2);
   table1->set_column_spacing(3);
   table1->attach(*player_peg1, 0, 0);
   table1->attach(*player_peg2, 0, 1);
   table1->attach(*player_peg3, 0, 2);
   table1->attach(*player_name1, 1, 0);
   table1->attach(*player_name2, 1, 1);
   table1->attach(*player_name3, 1, 2);
   table1->attach(*player_peg4, 2, 0);
   table1->attach(*player_peg5, 2, 1);
   table1->attach(*player_peg6, 2, 2);
   table1->attach(*player_name4, 3, 0);
   table1->attach(*player_name5, 3, 1);
   table1->attach(*player_name6, 3, 2);
   player_name1->set_hexpand(true);
   player_name2->set_hexpand(true);
   player_name3->set_hexpand(true);
   player_name4->set_hexpand(true);
   player_name5->set_hexpand(true);
   player_name6->set_hexpand(true);
   player_peg1->set_margin_start(3);
   player_peg2->set_margin_start(3);
   player_peg3->set_margin_start(3);
   player_peg4->set_margin_start(15);
   player_peg5->set_margin_start(15);
   player_peg6->set_margin_start(15);
   move_counter->set_xalign(0.5);
   label47->set_use_markup(true);
   vbox12->append(*label49);
   vbox12->append(*table1);
   vbox12->set_halign(Gtk::Align::START);
   vbox12->set_valign(Gtk::Align::START);
   vbox12->set_margin_top(6);
   vbox12->set_margin_start(24);
   vbox13->append(*label47);
   vbox13->append(*move_counter);
   vbox13->set_halign(Gtk::Align::END);
   vbox13->set_valign(Gtk::Align::START);
   vbox13->set_margin_top(6);
   vbox13->set_margin_end(24);

   vbox12->get_style_context()->add_class("board-info");
   vbox13->get_style_context()->add_class("board-info");
   {
      static Glib::RefPtr<Gtk::CssProvider> board_info_css;
      if (!board_info_css)
      {
         board_info_css = Gtk::CssProvider::create();
         board_info_css->load_from_data(
            ".board-info label { font-size: 150%; color: #000000; }\n"
            ".board-info label:backdrop { color: #000000; opacity: 1; }");
         Gtk::StyleContext::add_provider_for_display(
            Gdk::Display::get_default(), board_info_css,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      }
   }
   overlay1->set_child(*game_view);
   overlay1->add_overlay(*vbox12);
   vbox12->set_can_target(false);
   overlay1->add_overlay(*vbox13);
   vbox13->set_can_target(false);
   game_view->evt_board_resized.connect([this, vbox12, vbox13]() {
      double s = game_view->get_scale();
      double bw = game_view->get_base_width() * s;
      double bh = game_view->get_base_height() * s;
      auto c = game_view->get_view_center();
      double bx = c.get_x() - bw / 2.0;
      double by = c.get_y() - bh / 2.0;
      int inset = 12;
      vbox12->set_margin_start((int)bx + inset);
      vbox12->set_margin_top((int)by + inset);
      vbox13->set_margin_end((int)(game_view->get_width() - (bx + bw)) + inset + 20);
      vbox13->set_margin_top((int)by + inset);
   });
   message_view->set_editable(false);
   message_view->set_cursor_visible(false);
   message_view->set_pixels_above_lines(0);
   message_view->set_pixels_below_lines(0);
   message_view->set_pixels_inside_wrap(0);
   message_view->set_left_margin(0);
   message_view->set_right_margin(0);
   message_view->set_indent(0);
   message_view->set_wrap_mode(Gtk::WrapMode::WORD);
   message_view->set_justification(Gtk::Justification::LEFT);
   message_view->get_buffer()->set_text(_("Welcome to Chinese Checkers!\n"
		"Choose the 'Host New Board' or 'Join A Board' commands to start playing.\n"
		""));
   scrolledwindow1->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
   scrolledwindow1->set_size_request(-1, 120);
   scrolledwindow1->set_child(*message_view);
   chat_entry->set_visibility(true);
   chat_entry->set_editable(true);
   chat_entry->set_max_length(0);
   chat_entry->set_text(_(""));
   chat_entry->set_activates_default(false);
   chat_area->set_margin_start(3);
   chat_area->set_margin_end(3);
   chat_area->set_margin_top(3);
   chat_area->set_margin_bottom(3);
   // The chat history absorbs any extra height when the pane is resized;
   // the entry keeps its natural fixed height at the bottom.
   scrolledwindow1->set_vexpand(true);
   chat_area->append(*scrolledwindow1);
   chat_area->append(*chat_entry);
   vpaned1->set_start_child(*overlay1);
   vpaned1->set_end_child(*chat_area);
   vpaned1->set_resize_start_child(true);
   vpaned1->set_resize_end_child(true);
   // Never shrink the chat pane below its minimum (history 120px + entry),
   // so the entry can't be pushed off the bottom of the window.
   vpaned1->set_shrink_end_child(false);
   // The paned must claim the vertical slack itself; GtkBox only gives extra
   // space to expanding children.
   vpaned1->set_vexpand(true);
#ifndef MACOS_APP
   vbox1->append(*menubar1);
#endif
   vbox1->append(*vpaned1);
   vbox1->append(*statusbar);
   set_title(_("Chinese Checkers"));
   int board_width = 0;
   int board_height = 0;
   game_view->get_size_request(board_width, board_height);
   set_size_request(board_width + 20, board_height + 120);
   set_resizable(true);
   set_child(*vbox1);
   statusbar->hide();
   // The chat pane starts hidden (glademm never showed it either); it is
   // toggled by the Show Chat menu item in main_win::on_show_chat_activate.
   chat_area->hide();
#ifdef MACOS_APP
   set_default_size((int)lround(board_width * 1.5),
                    (int)lround(board_height * 1.5));
#else
   set_default_size((int)lround(board_width * 1.5),
                    (int)lround(board_height * 1.5) + 30);
#endif
   show();
   chat_entry->signal_activate().connect(sigc::mem_fun(*this, &main_win_glade::on_chat_entry_activate), false);
}

main_win_glade::~main_win_glade()
{  delete game_view;
   delete player_peg1;
   delete player_peg2;
   delete player_peg3;
   delete player_peg4;
   delete player_peg5;
   delete player_peg6;
}
