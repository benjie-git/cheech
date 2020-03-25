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

		sigc::signal<void, unsigned int, bool, bool, bool> signal_restart_game;

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
