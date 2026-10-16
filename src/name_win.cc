/*
 *   The Name Choosing Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
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
