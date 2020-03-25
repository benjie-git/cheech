/*
 *  The Peg Color Choosing Window.
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
