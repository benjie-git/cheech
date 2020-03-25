/*
 *  The Name Choosing Window.
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

#include "name_win.hh"


name_win::name_win() : name_win_glade()
{
}


void name_win::set_name(Glib::ustring *name, bool force)
{
	if (force)
	{
		Glib::ustring msg = "<b>The name " + *name + " is already being used.\n"
			"Please choose a differnt name.</b>";
		change_label->set_markup(msg);
		change_label->show();
	}
	else
	{
		change_label->hide();
	}

	name_entry->set_text(*name);
	name_entry->select_region(0, -1);
}


void name_win::on_disconnected()
{
	hide();
}


void name_win::on_name_entry_activate()
{
	on_change_button_clicked();
}

void name_win::on_cancel_button_clicked()
{
	hide();
}

void name_win::on_change_button_clicked()
{
	signal_change_name(name_entry->get_text());
	hide();
}
