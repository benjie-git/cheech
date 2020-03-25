/*
 *  The game setup window.
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

#include <gtkmm/messagedialog.h>

#include "setup_game_win.hh"


setup_game_win::setup_game_win() : setup_game_win_glade()
{
}


void setup_game_win::setup(unsigned int num, bool lj, bool ho, bool so,
						   GameServer::GameStatus status)
{
	num_players->set_value(num);
	long_jumps->set_active(lj);
	hop_others->set_active(ho);
	stop_others->set_active(so);
	_status = status;
}


void setup_game_win::on_cmd_game_turn(unsigned int posn,
									  GameServer::GameStatus status,
									  unsigned int move_count)
{
	_status = status;
}


void setup_game_win::on_disconnected()
{
	hide();
}


void setup_game_win::on_num_players_activate()
{
	on_ok_button_clicked();
}


void setup_game_win::on_cancel_button_clicked()
{
	hide();
}


void setup_game_win::on_ok_button_clicked()
{
	unsigned int num = (unsigned int)num_players->get_value();
	bool lj = long_jumps->get_active();
	bool ho = hop_others->get_active();
	bool so = stop_others->get_active();

	signal_restart_game(num, lj, ho, so);
}
