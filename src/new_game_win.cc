/*
 *  The New/Join Game Window.
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

#include "config.h"
#include "new_game_win.hh"
#include "utility.hh"


new_game_win::new_game_win() : new_game_win_glade()
{
	Prefs prefs;

	prefs.read();

	name_entry->set_text(prefs.name);
	set_selected_color(prefs.color);

	host_port_entry->set_text(util::to_str(prefs.host_port));
	cheechweb->set_active(prefs.start_cheechweb);
	cheechweb_port_entry->set_text(util::to_str(prefs.cheechweb_port));
	cheechweb_port_entry->set_sensitive(cheechweb->get_active());

	num_players->set_value(prefs.num_players);
	long_jumps->set_active(prefs.long_jumps);
	hop_others->set_active(prefs.hop_others);
	stop_others->set_active(prefs.stop_others);

	for (std::vector<Glib::ustring>::iterator i = prefs.join_hostnames.begin();
		 i != prefs.join_hostnames.end(); i++)
			join_host_entry->append_text(*i);
	if (prefs.join_hostnames.size() > 0)
		join_host_entry->set_active_text(prefs.join_hostnames.front());

	join_port_entry->set_text(util::to_str<unsigned int>(prefs.join_port));
	spectator->set_active(prefs.spectator);

	join_host_entry->get_entry()->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_button_clicked));
	join_host_entry->signal_changed().connect(
		sigc::mem_fun(*(join_host_entry->get_entry()), &Widget::grab_focus));
	join_host_entry->set_focus_on_click(false);
}


void new_game_win::show_join_page(bool joining)
{
	if (joining)
	{
		role_notebook->set_current_page(1);
		join_button->grab_default();
	}
	else
	{
		role_notebook->set_current_page(0);
		host_button->grab_default();
	}
}


void new_game_win::on_name_entry_activate()
{
	if (role_notebook->get_current_page()==0)
		on_host_button_clicked();
	else
		on_join_button_clicked();
}


void new_game_win::on_host_port_entry_activate()
{
	on_host_button_clicked();
}


void new_game_win::on_cheechweb_port_entry_activate()
{
	on_host_button_clicked();
}


void new_game_win::on_cheechweb_toggled()
{
	cheechweb_port_entry->set_sensitive(cheechweb->get_active());
}


void new_game_win::on_num_players_activate()
{
	on_host_button_clicked();
}


void new_game_win::on_join_host_entry_activate()
{
	on_join_button_clicked();
}


void new_game_win::on_join_port_entry_activate()
{
	on_join_button_clicked();
}


void new_game_win::on_host_cancel_button_clicked()
{
	hide();
}


void new_game_win::on_host_button_clicked()
{
	if (host_port_entry->get_text().length() > 0
		&& name_entry->get_text().length() >0)
	{
		unsigned int host_port =
			util::from_str<unsigned int>(host_port_entry->get_text());
		unsigned int cheechweb_port = (cheechweb->get_active()) ?
			util::from_str<unsigned int>(cheechweb_port_entry->get_text()) : 0;

		unsigned int num = (unsigned int)num_players->get_value();
		bool lj = long_jumps->get_active();
		bool ho = hop_others->get_active();
		bool so = stop_others->get_active();
		
		if (signal_start_server(host_port, cheechweb_port, num, lj, ho, so))
		{
			Prefs prefs;

			prefs.read();

			prefs.name = name_entry->get_text();
			prefs.color = get_selected_color();
			prefs.spectator = spectator->get_active();

			prefs.host_port = host_port;
			prefs.start_cheechweb = (cheechweb_port > 0);
			if (prefs.start_cheechweb)
				prefs.cheechweb_port = cheechweb_port;
			prefs.num_players = num;
			prefs.long_jumps = lj;
			prefs.hop_others = ho;
			prefs.stop_others = so;
			prefs.write();

			signal_start_client("localhost", prefs.host_port,
				prefs.name, prefs.color, prefs.spectator);
		}

		hide();
	}
}


void new_game_win::on_join_cancel_button_clicked()
{
	hide();
}


void new_game_win::on_join_button_clicked()
{
	if (join_host_entry->get_entry()->get_text().length() > 0 &&
		join_port_entry->get_text().length() > 0 &&
		name_entry->get_text().length() > 0)
	{
		Prefs prefs;

		prefs.read();

		prefs.name = name_entry->get_text();
		prefs.color = get_selected_color();
		prefs.spectator = spectator->get_active();

		Glib::ustring hostname;

		if (join_host_entry->get_active_text() != "")
			hostname = join_host_entry->get_active_text();
		else
			hostname = join_host_entry->get_entry()->get_text();

		prefs.join_port = util::from_str<unsigned int>(join_port_entry->get_text());
		prefs.write();
		
		signal_start_client(hostname, prefs.join_port,
			prefs.name, prefs.color, prefs.spectator);

		hide();
	}
}


unsigned int new_game_win::get_selected_color()
{
	if (red->get_active())
		return 1;
	if (orange->get_active())
		return 2;
	if (yellow->get_active())
		return 3;
	if (green->get_active())
		return 4;
	if (blue->get_active())
		return 5;
	if (purple->get_active())
		return 6;
	if (black->get_active())
		return 7;
	if (white->get_active())
		return 8;

	return 0;
}


void new_game_win::set_selected_color(unsigned int color)
{
	switch (color)
	{
		case 1:
			red->set_active();
			break;
		case 2:
			orange->set_active();
			break;
		case 3:
			yellow->set_active();
			break;
		case 4:
			green->set_active();
			break;
		case 5:
			blue->set_active();
			break;
		case 6:
			purple->set_active();
			break;
		case 7:
			black->set_active();
			break;
		case 8:
			white->set_active();
			break;
	}
}
