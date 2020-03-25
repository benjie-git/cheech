/*
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
 */

#include <iostream>
#include <string>
#include <glibmm.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "prefs.hh"
#include "utility.hh"


Prefs::Prefs()
{
	set_defaults();
}


void Prefs::set_defaults()
{
	name = "Cheech";
	color = 1;

	host_port = 3838;
	start_cheechweb = false;
	cheechweb_port = 3839;
	num_players = 2;
	long_jumps = false;
	hop_others = true;
	stop_others = true;

	join_hostnames.push_back("localhost");
	join_port = 3838;
	spectator = false;

	bot_type = "LookAhead(2)";
	move_delay = 400;
	done_delay = 600;
	think_delay = 0;
}


Glib::ustring Prefs::get_filename()
{
	Glib::ustring filename;

#ifdef WIN32
	char module_path[PATH_MAX];
	char *program_dir;

	GetModuleFileName(NULL, module_path, PATH_MAX);
	PathRemoveFileSpec(module_path);
	program_dir = g_locale_to_utf8(module_path, -1, NULL, NULL, NULL);
	filename = program_dir;
	filename += "\\cheech.ini";
#else
	Glib::ustring home = Glib::getenv("HOME");
	
	if (home == "")
	{
		std::cerr << "Home directory location unknown." << std::endl;
		return ".cheech";
	}
	filename = home +"/.cheech";
#endif

	return filename;
}


void Prefs::read(void)
{
	Glib::RefPtr<Glib::IOChannel> pfile;

	try
	{
		pfile = Glib::IOChannel::create_from_file(get_filename(), "r");
	}
	catch (Glib::FileError)
	{
		write();
		return;
	}

	join_hostnames.clear();

	Glib::ustring line, key, value;

	while (pfile->read_line(line) == Glib::IO_STATUS_NORMAL)
	{
		while (line.substr(line.length()-1,1) == "\n" ||
			line.substr(line.length()-1,1) == "\r")
				line = line.substr(0, line.length()-1);

		int seperator = line.find_first_of(" ");
		if (seperator < 0)
		{
			key = line.substr(0, line.length()-1);
			value = "";
		}
		else
		{
			key = line.substr(0, seperator);
			value = line.substr(seperator+1, line.length()-seperator-1);
		}

		if (key == "name")
			name = value;
		else if (key == "color")
			color = util::from_str<unsigned int>(value);

		else if (key == "host_port")
			host_port = util::from_str<unsigned int>(value);
		else if (key == "start_cheechweb")
			start_cheechweb = (value == "yes") ? true : false;
		else if (key == "cheechweb_port")
			cheechweb_port = util::from_str<unsigned int>(value);

		else if (key == "num_players")
			num_players = util::from_str<unsigned int>(value);
		else if (key == "long_jumps")
			long_jumps = (value == "yes") ? true : false;
		else if (key == "hop_others")
			hop_others = (value == "yes") ? true : false;
		else if (key == "stop_others")
			stop_others = (value == "yes") ? true : false;

		else if (key == "join_hostname")
			join_hostnames.push_back(value);
		else if (key == "join_port")
			join_port = util::from_str<unsigned int>(value);
		else if (key == "spectator")
			spectator = (value == "yes") ? true : false;

		else if (key == "bot_type")
			bot_type = value;
		else if (key == "move_delay")
			move_delay = util::from_str<unsigned int>(value);
		else if (key == "done_delay")
			done_delay = util::from_str<unsigned int>(value);
		else if (key == "think_delay")
			think_delay = util::from_str<unsigned int>(value);
	}

	pfile->close();
}


void Prefs::write(void)
{
	Glib::RefPtr<Glib::IOChannel> pfile;

	try
	{
		pfile = Glib::IOChannel::create_from_file(get_filename(), "w");
	}
	catch (Glib::FileError)
	{
		std::cerr << "Can't open preferences at '"
			<< get_filename() << "'." << std::endl;
		return;
	}

#ifndef WIN32
	Glib::ustring new_line = "\n";
#else
	Glib::ustring new_line = "\r\n";
#endif

	pfile->write("name " + name + new_line);
	pfile->write("color " + util::to_str(color) + new_line);

	pfile->write("host_port " + util::to_str(host_port) + new_line);
	pfile->write("start_cheechweb ");
	pfile->write((start_cheechweb ? "yes" : "no") + new_line);
	pfile->write("cheechweb_port " + util::to_str(cheechweb_port) + new_line);

	pfile->write("num_players " + util::to_str(num_players) + new_line);
	pfile->write("long_jumps ");
	pfile->write((long_jumps ? "yes" : "no") + new_line);
	pfile->write("hop_others ");
	pfile->write((hop_others ? "yes" : "no") + new_line);
	pfile->write("stop_others ");
	pfile->write((stop_others ? "yes" : "no") + new_line);

	for (std::vector<Glib::ustring>::iterator i = join_hostnames.begin();
		 i != join_hostnames.end(); i++)
			pfile->write("join_hostname " + *i + new_line);

	pfile->write("join_port " + util::to_str(join_port) + new_line);
	pfile->write("spectator ");
	pfile->write((spectator ? "yes" : "no") + new_line);

	pfile->write("bot_type " + bot_type + new_line);
	pfile->write("move_delay " + util::to_str(move_delay) + new_line);
	pfile->write("done_delay " + util::to_str(done_delay) + new_line);
	pfile->write("think_delay " + util::to_str(think_delay) + new_line);

	pfile->close();
}


void Prefs::set_join_hostname(Glib::ustring host)
{
	for (std::vector<Glib::ustring>::iterator i = join_hostnames.begin();
		 i < join_hostnames.end(); i++)
			if ((*i) == host)
				join_hostnames.erase(i);

	join_hostnames.insert(join_hostnames.begin(), host);
}
