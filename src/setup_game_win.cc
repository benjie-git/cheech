/*
 *   The game setup window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
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
