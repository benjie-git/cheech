/*
 *  cheechwebd application's main.
 *  cheechwebd is the web-based cheech client's backend
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
#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glibmm/main.h>
#include <glibmm/iochannel.h>
#include <glibmm/optioncontext.h>
#include <sigc++/bind.h>
#include <gnet-2.0/gnet.h>

#include "utility.hh"
#include "ajax_server.hh"

// cheechwebd Options
int ajax_port;
Glib::ustring cheechd_hostname;
int cheechd_port;


void process_options(int &argc, char **&argv)
{
	try
	{
		Glib::OptionContext opt_context;
		Glib::OptionGroup opt_group(
			"cheechwebd server options", "Options defining the cheechweb bridge");

		Glib::OptionEntry opt_ajax_port;
		opt_ajax_port.set_long_name("cheechweb port");
		opt_ajax_port.set_short_name('p');
		opt_ajax_port.set_arg_description("port");
		opt_ajax_port.set_description(
			"port to use for the cheechwebd server (3839)");
		opt_group.add_entry(opt_ajax_port, ajax_port);

		Glib::OptionEntry opt_cheechd_hostname;
		opt_cheechd_hostname.set_long_name("cheechd hostname");
		opt_cheechd_hostname.set_short_name('H');
		opt_cheechd_hostname.set_arg_description("host");
		opt_cheechd_hostname.set_description(
			"hostname of a running cheech game to connect cheechweb clients to (localhost)");
		opt_group.add_entry(opt_cheechd_hostname, cheechd_hostname);

		Glib::OptionEntry opt_cheechd_port;
		opt_cheechd_port.set_long_name("cheechd port");
		opt_cheechd_port.set_short_name('P');
		opt_cheechd_port.set_arg_description("port");
		opt_cheechd_port.set_description(
			"port of a running cheech game to connect cheechweb clients to (3838)");
		opt_group.add_entry(opt_cheechd_port, cheechd_port);

		opt_context.set_main_group(opt_group);

		opt_context.parse(argc, argv);
	}
	catch (Glib::OptionError er)
	{
		std::cout << "Bad command line arguments.  Try cheechwebd --help"
			<< std::endl;
		exit(1);
	}

	// Defaults
	if (ajax_port == 0) ajax_port = 3839;
	if (cheechd_port == 0) cheechd_port = 3838;
	if (cheechd_hostname == "") cheechd_hostname = "localhost";
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

	AjaxServer ajax_server;

	if (!ajax_server.listen(ajax_port, cheechd_hostname, cheechd_port))
	{
		std::cout << "Failed to start cheechweb server on port "
			<< util::to_str(ajax_port) << "." << std::endl;
		exit(1);
	}

	std::cout << "cheechweb server running on port "
		<< util::to_str(ajax_port) << "..." << std::endl;

	m->run();

	return 0;
}
