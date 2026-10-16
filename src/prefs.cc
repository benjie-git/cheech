/*
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <string>
#include <glibmm.h>

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>
#endif

#include "prefs.hh"
#include "bot_catalog.hh"
#include "utility.hh"


// Numeric values were once saved with the machine's locale active, so
// digits could be grouped ("3,838", "3.838", or "3 838"/no-break-space
// in French locales).  Strip the separators before parsing so stale
// prefs read back as the number that was meant.
static Glib::ustring strip_grouping(const Glib::ustring& value)
{
	std::string clean(value.raw());
	const std::string seps[] =
		{",", ".", " ", "\t", "\xC2\xA0", "\xE2\x80\xAF"};
	for (const std::string &sep : seps)
	{
		std::string::size_type pos = clean.find(sep);
		while (pos != std::string::npos)
		{
			clean.erase(pos, sep.length());
			pos = clean.find(sep, pos);
		}
	}
	return Glib::ustring(clean);
}


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
	long_jumps = true;
	hop_others = true;
	stop_others = true;

	join_hostnames.push_back("localhost");
	join_port = 3838;
	spectator = false;

	bot_type = "Neutral";
	bot_level = 1;
	bot_depth = 0;
	move_delay = 250;
	think_delay = 0;
	smarts = 100;
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
	catch (const Glib::Error&)
	{
		write();
		return;
	}

	join_hostnames.clear();
	seats.clear();

	bool have_level = false;

	Glib::ustring line, key, value;

	while (pfile->read_line(line) == Glib::IOStatus::NORMAL)
	{
		try
		{
		while (!line.empty() && (line.substr(line.length()-1,1) == "\n" ||
			line.substr(line.length()-1,1) == "\r"))
				line = line.substr(0, line.length()-1);

		Glib::ustring::size_type seperator = line.find_first_of(" ");
		if (seperator == Glib::ustring::npos)
		{
			key = line;
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
			color = util::from_str<unsigned int>(strip_grouping(value));

		else if (key == "host_port")
			host_port = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "start_cheechweb")
			start_cheechweb = (value == "yes") ? true : false;
		else if (key == "cheechweb_port")
			cheechweb_port = util::from_str<unsigned int>(strip_grouping(value));

		else if (key == "num_players")
			num_players = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "long_jumps")
			long_jumps = (value == "yes") ? true : false;
		else if (key == "hop_others")
			hop_others = (value == "yes") ? true : false;
		else if (key == "stop_others")
			stop_others = (value == "yes") ? true : false;

		else if (key == "join_hostname")
			join_hostnames.push_back(value);
		else if (key == "seat")
			seats.push_back(value);
		else if (key == "join_port")
			join_port = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "spectator")
			spectator = (value == "yes") ? true : false;

		else if (key == "bot_type")
		{
			// Older files stored a full type like "LookAhead(3)".  Split it
			// into the family label and an explicit depth; a newer bot_depth
			// line (read later) overrides the depth.
			if (value.find("(") != Glib::ustring::npos)
			{
				bot_type = BotUI::family_label(value);
				Glib::ustring::size_type open = value.find("(");
				Glib::ustring::size_type close = value.find(")", open);
				if (close != Glib::ustring::npos)
					bot_depth = util::from_str<unsigned int>(
						value.substr(open + 1, close - open - 1));
			}
			else
				bot_type = value;
		}
		else if (key == "bot_level")
		{
			bot_level = util::from_str<unsigned int>(strip_grouping(value));
			have_level = true;
		}
		else if (key == "bot_depth")
			bot_depth = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "move_delay")
			move_delay = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "think_delay")
			think_delay = util::from_str<unsigned int>(strip_grouping(value));
		else if (key == "smarts")
			smarts = util::from_str<unsigned int>(strip_grouping(value));
		}
		// A damaged or non-UTF-8 line shouldn't abort the whole app.
		catch (const Glib::Error&)
		{
		}
	}

	pfile->close();

	// Files written before skill levels existed stored a raw smarts
	// percentage; map it back onto the nearest level.
	if (!have_level)
	{
		if (smarts >= 98)
			bot_level = 1;
		else if (smarts >= 85)
			bot_level = 2;
		else if (smarts >= 75)
			bot_level = 3;
		else
			bot_level = 4;
	}
}


void Prefs::write(void)
{
	Glib::RefPtr<Glib::IOChannel> pfile;

	try
	{
		pfile = Glib::IOChannel::create_from_file(get_filename(), "w");
	}
	catch (const Glib::Error&)
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

	for (std::vector<Glib::ustring>::iterator i = seats.begin();
		 i != seats.end(); i++)
			pfile->write("seat " + *i + new_line);

	pfile->write("bot_type " + bot_type + new_line);
	pfile->write("bot_level " + util::to_str(bot_level) + new_line);
	pfile->write("bot_depth " + util::to_str(bot_depth) + new_line);
	pfile->write("move_delay " + util::to_str(move_delay) + new_line);
	pfile->write("think_delay " + util::to_str(think_delay) + new_line);
	pfile->write("smarts " + util::to_str(smarts) + new_line);

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
