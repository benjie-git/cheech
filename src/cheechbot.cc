/*
 *  cheechbot application's main.
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
#include <glibmm/optioncontext.h>
#include <sigc++/bind.h>
#include <gnet-2.0/gnet.h>

#include "utility.hh"
#include "bot_base.hh"


// cheechbot Options
Glib::ustring host_name;
int port;
Glib::ustring name;
int color;
Glib::ustring bot_type;
int thinking_delay;
int move_step_delay;
int move_done_delay;


void printMessage(Glib::ustring msg)
{
	std::cout << msg << std::endl;
}


void connected()
{
	std::cout << "Connected." << std::endl;
}


void cancelled(BotBase *bot, Glib::RefPtr<Glib::MainLoop> m)
{
	delete bot;

	std::cout << "Couldn't Connect." << std::endl;

	m->quit();
}


void disconnected(BotBase *bot, Glib::RefPtr<Glib::MainLoop> m)
{
	delete bot;

	std::cout << "Bot Disconnected." << std::endl;

	m->quit();
}


void process_options(int &argc, char **&argv)
{
	try
	{
		Glib::OptionContext opt_context("hostname");
		Glib::OptionGroup opt_group(
			"bot options", "Options defining how the bot behaves");

		Glib::OptionEntry opt_port;
		opt_port.set_long_name("port");
		opt_port.set_short_name('p');
		opt_port.set_arg_description("port");
		opt_port.set_description(
			"port of the cheech server to connect to (3838)");
		opt_group.add_entry(opt_port, port);

		Glib::OptionEntry opt_name;
		opt_name.set_long_name("name");
		opt_name.set_short_name('n');
		opt_name.set_arg_description("name");
		opt_name.set_description(
			"name for this computer player");
		opt_group.add_entry(opt_name, name);

		Glib::OptionEntry opt_color;
		opt_color.set_long_name("color");
		opt_color.set_short_name('c');
		opt_color.set_arg_description("N(1-8)");
		opt_color.set_description(
			"color to use for this computer player (5)");
		opt_group.add_entry(opt_color, color);

		Glib::OptionEntry opt_type;
		opt_type.set_long_name("type");
		opt_type.set_short_name('t');
		opt_type.set_arg_description("type");
		opt_type.set_description(
			"bot-type: Simple, LookAhead[2-5], Friendly[3-5], Mean[3-5]\n"
			"\t(or s, l[2-5], f[3-5], m[3-5])");
		opt_group.add_entry(opt_type, bot_type);

		Glib::OptionEntry opt_think_delay;
		opt_think_delay.set_long_name("think-delay");
		opt_think_delay.set_short_name('T');
		opt_think_delay.set_arg_description("ms");
		opt_think_delay.set_description(
			"time to show each move while thinking bout it in ms (0)");
		opt_group.add_entry(opt_think_delay, thinking_delay);

		Glib::OptionEntry opt_step_delay;
		opt_step_delay.set_long_name("step-delay");
		opt_step_delay.set_short_name('S');
		opt_step_delay.set_arg_description("ms");
		opt_step_delay.set_description(
			"time to show each step of a move in ms (300)");
		opt_group.add_entry(opt_step_delay, move_step_delay);

		Glib::OptionEntry opt_move_delay;
		opt_move_delay.set_long_name("move-delay");
		opt_move_delay.set_short_name('M');
		opt_move_delay.set_arg_description("ms");
		opt_move_delay.set_description(
			"time to show each completed move in ms (600)");
		opt_group.add_entry(opt_move_delay, move_done_delay);

		opt_context.set_main_group(opt_group);

		opt_context.parse(argc, argv);
	}
	catch (Glib::OptionError er)
	{
		std::cout << argv[0] << ": Bad command line arguments.  "
			"Try cheechbot --help" << std::endl;
		exit(1);
	}

	// Defaults
	if (port == 0) port = 3838;
	if (bot_type == "") bot_type = "l3";
	if (color == 0) color = 5;
	if (thinking_delay == 0) thinking_delay = 0;
	if (move_step_delay == 0) move_step_delay = 300;
	if (move_done_delay == 0) move_done_delay = 600;

	host_name = "";
	if (argc == 2)
		host_name = argv[1];
	else
	{
		std::cout << argv[0] << ": Bad command line arguments.  "
			"Try cheechbot --help" << std::endl;
		exit(1);
	}
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

	BotBase *bot = BotBase::new_bot_of_type(bot_type);

	if (!bot)
	{
		std::cout << argv[0] << ": Bad bot_type.  "
			"Try cheechbot --help" << std::endl;
		exit(1);
	}

	bot->evt_message.connect(sigc::ptr_fun(printMessage));
	bot->evt_connected.connect(sigc::ptr_fun(connected));
	bot->evt_cancelled.connect(sigc::bind(sigc::bind(sigc::ptr_fun(cancelled),
		m), bot));
	bot->evt_disconnected.connect(sigc::bind(sigc::bind(sigc::ptr_fun(disconnected),
		m), bot));

	std::cout << "Chinese Checkers Bot joining game on " << host_name;
	std::cout << " port " << port << " as Chong... ";

	bot->set_think_delay(thinking_delay);
	bot->set_move_delay(move_step_delay, move_done_delay);
	bot->set_name(name);
	bot->set_color(color);
	bot->join_game(host_name, port);
	m->run();

	return 0;
}
