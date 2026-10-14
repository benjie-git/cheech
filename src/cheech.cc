/*
 *  cheech application's main.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

#include <iostream>
#include <glib/gi18n.h>
#include <glibmm/optioncontext.h>
#include <gdkmm/display.h>
#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/stylecontext.h>

#include "config.h"
#include "prefs.hh"
#include "main_win.hh"
#include "game_images.hh"
#include "utility.hh"


// cheech Options
Glib::ustring name;
int color;
Glib::ustring host_name;
bool host;
bool spectator;
int num_players;
int port;
int cheechweb_port;
bool long_jumps;
bool hop_others;
bool stop_others;


void process_options(int &argc, char **&argv)
{
	try
	{
		Glib::OptionContext opt_context;
		Glib::OptionGroup opt_group(
			"cheech options", "Options defining the game");

		Glib::OptionEntry opt_host;
		opt_host.set_long_name("host");
		opt_host.set_short_name('h');
		opt_host.set_description("host a game");
		opt_group.add_entry(opt_host, host);

		Glib::OptionEntry opt_hostname;
		opt_hostname.set_long_name("join");
		opt_hostname.set_short_name('j');
		opt_hostname.set_arg_description("hostname");
		opt_hostname.set_description(
			"join a game hosted on hostname");
		opt_group.add_entry(opt_hostname, host_name);

		Glib::OptionEntry opt_port;
		opt_port.set_long_name("port");
		opt_port.set_short_name('p');
		opt_port.set_arg_description("port");
		opt_port.set_description(
			"port of the cheech server");
		opt_group.add_entry(opt_port, port);

		Glib::OptionEntry opt_cheechweb_port;
		opt_cheechweb_port.set_long_name("cheechweb-port");
		opt_cheechweb_port.set_short_name('P');
		opt_cheechweb_port.set_arg_description("port");
		opt_cheechweb_port.set_description(
			"port to use for the cheechweb server");
		opt_group.add_entry(opt_cheechweb_port, cheechweb_port);

		Glib::OptionEntry opt_name;
		opt_name.set_long_name("name");
		opt_name.set_short_name('n');
		opt_name.set_arg_description("name");
		opt_name.set_description(
			"name to use during the game");
		opt_group.add_entry(opt_name, name);

		Glib::OptionEntry opt_color;
		opt_color.set_long_name("color");
		opt_color.set_short_name('c');
		opt_color.set_arg_description("N(1-8)");
		opt_color.set_description(
			"color to use for this computer player");
		opt_group.add_entry(opt_color, color);

		Glib::OptionEntry opt_spectator;
		opt_spectator.set_long_name("spectator");
		opt_spectator.set_short_name('s');
		opt_spectator.set_description(
			"join as a spectator");
		opt_group.add_entry(opt_spectator, spectator);

		Glib::OptionEntry opt_num_players;
		opt_num_players.set_long_name("num-players");
		opt_num_players.set_short_name('N');
		opt_num_players.set_arg_description("num");
		opt_num_players.set_description(
			"how many players will be in this game");
		opt_group.add_entry(opt_num_players, num_players);

		Glib::OptionEntry opt_long_jumps;
		opt_long_jumps.set_long_name("long-jumps");
		opt_long_jumps.set_short_name('L');
		opt_long_jumps.set_description(
			"allow long jumps");
		opt_group.add_entry(opt_long_jumps, long_jumps);

		Glib::OptionEntry opt_hop_others;
		opt_hop_others.set_long_name("hop-others");
		opt_hop_others.set_short_name('H');
		opt_hop_others.set_description(
			"allow hopping through other players' triangles");
		opt_group.add_entry(opt_hop_others, hop_others);

		Glib::OptionEntry opt_stop_others;
		opt_stop_others.set_long_name("stop-others");
		opt_stop_others.set_short_name('O');
		opt_stop_others.set_description(
			"allow stopping in other players' triangles");
		opt_group.add_entry(opt_stop_others, stop_others);

		opt_context.set_main_group(opt_group);

		opt_context.parse(argc, argv);
	}
	catch (Glib::OptionError er)
	{
		std::cout << "Bad command line arguments.  Try cheech --help"
			<< std::endl;
//		exit(1);
	}

	// Defaults
	if (host || host_name != "")
	{
		Prefs prefs;
		prefs.read();
		
		if (name == "") name = prefs.name;
		if (color == 0) color = prefs.color;
		if (port == 0) port = host ? prefs.host_port : prefs.join_port;
		if (cheechweb_port == 0) cheechweb_port = (prefs.start_cheechweb) ? 
												prefs.cheechweb_port : 0;
		if (num_players == 0) num_players = prefs.num_players;
		if (num_players < 1) num_players = 1;
		if (num_players > 6) num_players = 6;
		if (!long_jumps) long_jumps = prefs.long_jumps;
		if (!hop_others) hop_others = prefs.hop_others;
		if (!stop_others) stop_others = prefs.stop_others;
	}
}


int main(int argc, char **argv)
{
	util::print_banner("cheech");

#if defined(ENABLE_NLS)
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif //ENABLE_NLS
	// Gio::init() is called inside Gtk::Application::create(), below.

	process_options(argc, argv);

	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create();
	GameImages::init();

	main_win *main_win = new class main_win();

	if (host)
	{
		// Command-line host mode: the local player plus open seats, matching
		// the old num_players server argument.  As a spectator the seats are
		// all open and the local client joins separately, like before.
		std::vector<NewGameSeat> seats;
		NewGameSeat seat;
		seat.kind = (!spectator) ? 0 : 2;	// human / remote
		seat.name = name;
		seat.color = color;
		seat.family = BotUI::NEUTRAL;
		seat.level = 1;
		seat.depth = 0;
		seat.focus_all = true;
		seat.id = 0;
		if (!spectator)
			seats.push_back(seat);
		for (int i = seats.size(); i < num_players; i++)
		{
			seat.kind = 2;	// remote
			seat.name = "";
			seat.id = i;
			seats.push_back(seat);
		}
		main_win->start_server(port, cheechweb_port, long_jumps,
							   hop_others, stop_others, seats);
		if (spectator)
			main_win->start_client("localhost", (unsigned int)port, name,
								   (unsigned int)color, spectator);
	}
	else if (host_name != "")
	{
		main_win->start_client(host_name, (unsigned int)port, name,
							   (unsigned int)color, spectator);
	}
	
	// GTK4 requires application windows to be added after the startup
	// signal has been emitted (i.e. inside run()), not before.  The
	// keyboard accelerators are also registered here: they live on the
	// application, which the window only knows once it has been added.
	app->signal_startup().connect([&]() {
		// Handing the model to the application shows it in the native
		// macOS menu bar (on other platforms it is inert, since the
		// in-window Gtk::PopoverMenuBar is what the user sees).
		app->set_menubar(main_win->get_menu_model());

		// GTK fills the macOS app menu (next to the Apple menu) with
		// items targeting app.* actions; without these, Quit (and About)
		// show up disabled there.  They route to the window's win.*
		// actions so the confirm-quit dialog and everything else applies.
		// Registering app.quit also makes native termination (Cmd+Q) go
		// through the same path instead of killing the process outright.
		// Widget::activate_action() takes the fully-prefixed action name.
		app->add_action("quit", [main_win]()
			{ main_win->activate_action("win.quit"); });
		app->add_action("about", [main_win]()
			{ main_win->activate_action("win.about"); });

		// The theme only rounds the corners of widgets whose CSS node is
		// named `frame`; the .frame style class (which the Server Setup
		// expander borrows for its border) draws a square border.  Round
		// those to match the theme's window radius (5px button radius + 3).
		Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
		css->load_from_string("expander-widget.frame { border-radius: 8px; }");
		Gtk::StyleContext::add_provider_for_display(
			Gdk::Display::get_default(), css,
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

		app->add_window(*main_win);

// GTK4 dropped the GTK3 <Primary>-to-Command mapping; on macOS the Command
// key is the Meta modifier, and the GTK4 docs say apps must register
// macOS-specific shortcuts themselves.
#ifdef MACOS_APP
#  define CHEECH_PRIMARY "<Meta>"
#else
#  define CHEECH_PRIMARY "<Primary>"
#endif
		app->set_accels_for_action("win.new_game", {CHEECH_PRIMARY "n"});
		app->set_accels_for_action("win.end_game", {CHEECH_PRIMARY "t"});
		app->set_accels_for_action("win.leave_game", {CHEECH_PRIMARY "l"});
		app->set_accels_for_action("win.game_settings", {CHEECH_PRIMARY "g"});
		app->set_accels_for_action("win.rotate", {CHEECH_PRIMARY "a"});
		app->set_accels_for_action("win.shuffle", {CHEECH_PRIMARY "f"});
		app->set_accels_for_action("win.restart_game", {CHEECH_PRIMARY "k"});
#ifndef MACOS_APP
		// On macOS GTK's quartz backend already binds Cmd+Q to app.quit
		// (above), which routes to win.quit.
		app->set_accels_for_action("win.quit", {CHEECH_PRIMARY "q"});
#endif
		app->set_accels_for_action("win.undo", {CHEECH_PRIMARY "z"});
		app->set_accels_for_action("win.redo", {CHEECH_PRIMARY "y"});
		app->set_accels_for_action("win.show_last_move", {CHEECH_PRIMARY "h"});
		app->set_accels_for_action("win.add_computer_player", {CHEECH_PRIMARY "d"});
		app->set_accels_for_action("win.remove_computer_players", {CHEECH_PRIMARY "e"});
		app->set_accels_for_action("win.setup_computer_player", {CHEECH_PRIMARY "p"});
		app->set_accels_for_action("win.cut", {CHEECH_PRIMARY "x"});
		app->set_accels_for_action("win.copy", {CHEECH_PRIMARY "c"});
		app->set_accels_for_action("win.paste", {CHEECH_PRIMARY "v"});
	});

	int rc = app->run();
	delete main_win;

	return rc;
}
