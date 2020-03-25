/*
 *  cheechd application's main.
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
#include <string>
#include <stdlib.h>
#include <config.h>
#include <glib/gi18n.h>
#include <glibmm/main.h>
#include <glibmm/iochannel.h>
#include <glibmm/optioncontext.h>
#include <sigc++/bind.h>
#include <gnet-2.0/gnet.h>

#include "utility.hh"
#include "game_server.hh"
#include "ajax_server.hh"


// cheechd Options
int num_players;
int num_games;
int port;
bool long_jumps;
bool hop_others;
bool stop_others;

bool start_cheechwebd;
int cheechweb_port;


void printMessage(Glib::ustring msg)
{
	std::cout << msg << std::endl;
}


void gameOver(GameServer *server, Glib::RefPtr<Glib::MainLoop> m)
{
	num_games--;

	if (num_games)
		server->rotate_players();
	else
		m->quit();
}


bool on_stdin(Glib::IOCondition condition, GameServer *server)
{
	std::string line;

	if(!std::getline(std::cin, line))
	{
		server->end_game();
		exit(0);
	}

	server->parse_command(line);
	return true;
}


void process_options(int &argc, char **&argv)
{
	try
	{
		Glib::OptionContext opt_context;
		Glib::OptionGroup opt_group(
			"cheechd server options", "Options defining the game being served");

		Glib::OptionEntry opt_port;
		opt_port.set_long_name("port");
		opt_port.set_short_name('p');
		opt_port.set_arg_description("port");
		opt_port.set_description(
			"port to use for the cheech server (3838)");
		opt_group.add_entry(opt_port, port);

		Glib::OptionEntry opt_num_players;
		opt_num_players.set_long_name("num-players");
		opt_num_players.set_short_name('N');
		opt_num_players.set_arg_description("num");
		opt_num_players.set_description(
			"how many players will be in this game (3)");
		opt_group.add_entry(opt_num_players, num_players);

		Glib::OptionEntry opt_num_games;
		opt_num_games.set_long_name("num-games");
		opt_num_games.set_short_name('g');
		opt_num_games.set_arg_description("num");
		opt_num_games.set_description(
			"play this many games and then stop");
		opt_group.add_entry(opt_num_games, num_games);

		Glib::OptionEntry opt_long_jumps;
		opt_long_jumps.set_long_name("long-jumps");
		opt_long_jumps.set_short_name('L');
		opt_long_jumps.set_description(
			"allow long jumps (default=no)");
		opt_group.add_entry(opt_long_jumps, long_jumps);

		Glib::OptionEntry opt_hop_others;
		opt_hop_others.set_long_name("hop-others");
		opt_hop_others.set_short_name('H');
		opt_hop_others.set_description(
			"allow hopping through other players' triangles (default=no)");
		opt_group.add_entry(opt_hop_others, hop_others);

		Glib::OptionEntry opt_stop_others;
		opt_stop_others.set_long_name("stop-others");
		opt_stop_others.set_short_name('O');
		opt_stop_others.set_description(
			"allow stopping in other players' triangles (default=no)");
		opt_group.add_entry(opt_stop_others, stop_others);

		Glib::OptionEntry opt_start_cheechwebd;
		opt_start_cheechwebd.set_long_name("start-cheechweb");
		opt_start_cheechwebd.set_short_name('W');
		opt_start_cheechwebd.set_description(
			"start the cheechweb server (default=no)");
		opt_group.add_entry(opt_start_cheechwebd, start_cheechwebd);

		Glib::OptionEntry opt_cheechweb_port;
		opt_cheechweb_port.set_long_name("cheechweb-port");
		opt_cheechweb_port.set_short_name('P');
		opt_cheechweb_port.set_arg_description("port");
		opt_cheechweb_port.set_description(
			"port to use for the cheechweb server (3839)");
		opt_group.add_entry(opt_cheechweb_port, cheechweb_port);

		opt_context.set_main_group(opt_group);

		opt_context.parse(argc, argv);
	}
	catch (Glib::OptionError er)
	{
		std::cout << "Bad command line arguments.  Try cheechd --help"
			<< std::endl;
		exit(1);
	}

	// Defaults
	if (port == 0) port = 3838;
	if (cheechweb_port == 0 && start_cheechwebd) cheechweb_port = 3839;
	if (num_players == 0) num_players = 3;
	if (num_players < 1) num_players = 1;
	if (num_players > 6) num_players = 6;
}


int main(int argc, char **argv)
{
#if defined(ENABLE_NLS)
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif //ENABLE_NLS

	gnet_init();

	process_options(argc, argv);

	Glib::RefPtr<Glib::MainLoop> m = Glib::MainLoop::create();

	GameServer *server = new GameServer(port, num_players, long_jumps,
										hop_others, stop_others);
	AjaxServer *ajax_server;

	server->evt_message.connect(sigc::ptr_fun(printMessage));
	if (num_games)
		server->evt_game_over.connect(sigc::bind(sigc::bind(sigc::ptr_fun(gameOver), m), server));
	server->new_game();

	if (!server->ready())
	{
		delete server;
		std::cout << "Chinese Checkers GameServer failed to start on port "
			<< port << std::endl;
		return 0;
	}

	std::cout << "Chinese Checkers GameServer started on port "
		<< port << std::endl;

	if (cheechweb_port)
	{
		ajax_server = new AjaxServer();
		if (!ajax_server->listen(cheechweb_port, "localhost", port))
		{
			delete ajax_server;
			std::cout << "CheechWeb Server failed to start on port "
				<< cheechweb_port << std::endl;
			ajax_server = NULL;
		}
		else
		{
			std::cout << "CheechWeb Server started on port "
				<< cheechweb_port << std::endl;
		}
	}


	std::cout << "Game started for " << num_players << " players." << std::endl;
	std::cout << "Long Jumps" << (long_jumps?" ":" not ") << "allowed."
		<< std::endl;
	std::cout << "Hopping through other players' triangles"
		<< (hop_others?" ":" not ") << "allowed." << std::endl;
	std::cout << "Stopping in other players' triangles"
		<< (stop_others?" ":" not ") << "allowed." << std::endl;

	std::cout << std::endl;

	Glib::signal_io().connect(sigc::bind(sigc::ptr_fun(on_stdin), server),
							  Glib::IOChannel::create_from_fd(0),
							  Glib::IO_IN);

	m->run();

	return 0;
}
