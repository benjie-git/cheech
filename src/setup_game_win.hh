/*
 *   The game setup window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _SETUP_GAME_WIN_HH
#  include "setup_game_win_glade.hh"
#  define _SETUP_GAME_WIN_HH

#include "game_client.hh"
#include "prefs.hh"


class setup_game_win : public setup_game_win_glade
{
	public:
		setup_game_win();

		void setup(unsigned int num, bool lj, bool ho, bool so,
				   GameServer::GameStatus status);

		sigc::signal<void(unsigned int, bool, bool, bool)> signal_restart_game;

		void on_cmd_game_turn(unsigned int posn,
							  GameServer::GameStatus status,
							  unsigned int move_number);
		void on_disconnected();

	protected:
        void on_num_players_activate();
        void on_cancel_button_clicked();
        void on_ok_button_clicked();

		GameServer::GameStatus _status;
};
#endif
