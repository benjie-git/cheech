/*
 *  The bot setup window.
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

#ifndef _BOT_SETUP_WIN_HH
#  include "setup_bot_win_glade.hh"
#  define _BOT_SETUP_WIN_HH

#include "bot_base.hh"


class setup_bot_win : public setup_bot_win_glade
{
	public:
		setup_bot_win();
		~setup_bot_win();

		void setup(GameServer::GameStatus status);

		sigc::signal<void, BotBase*> signal_add_bot;
		sigc::signal<void> signal_remove_bots;

		void on_cmd_game_turn(unsigned int posn,
							  GameServer::GameStatus status,
							  unsigned int move_count);

	private:
        void on_name_entry_activate();
        void on_add_button_activate();
        void on_remove_button_activate();
        void on_ok_button_activate();
        void on_defaults_activate();
		void on_bot_type_changed();

		GameServer::GameStatus _status;
		BotBase *_bot;
};
#endif
