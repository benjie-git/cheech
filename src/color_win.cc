/*
 *   The Peg Color Choosing Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "color_win.hh"
#include "game_images.hh"


color_win::color_win() : color_win_glade()
{
}


void color_win::set_force_info(Glib::ustring *name, int color)
{
	if (name)
	{
		Glib::ustring msg = "<b>" + *name + " is already using " +
			GameImages::get_color_name(color) + 
			".\nPlease choose a differnt color.</b>";
		change_label->set_markup(msg);
		change_label->show();
	}
	else
	{
		change_label->hide();
	}
}


void color_win::on_disconnected()
{
	hide();
}


void color_win::on_color_cancel_button_clicked()
{
	hide();
}


void color_win::on_change_button_clicked()
{
	signal_change_color(get_selected_color());
	hide();
}


int color_win::get_selected_color()
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
