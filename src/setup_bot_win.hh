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
#include "game_client.hh"

#include <gtkmm/checkbutton.h>
#include <vector>


class setup_bot_win : public setup_bot_win_glade
{
	public:
		setup_bot_win();
		~setup_bot_win();

		void setup(GameServer::GameStatus status, GameClient *client = NULL);

		// Rebuilds the player checkboxes from the current client state.  Call
		// this when the player list changes (a player joins or leaves).
		void refresh_focus();

		// Shows the window and gives it keyboard focus.
		void present_focused();

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
		void rebuild_focus();
		void grab_dialog_focus();

		GameServer::GameStatus _status;
		BotBase *_bot;
		GameClient *_client;
		// _focus_checks is parallel to player numbers 1..N; _focus_widgets
		// holds the grid cells (label and checkbox row) for cleanup.
		std::vector<Gtk::CheckButton*> _focus_checks;
		std::vector<Gtk::Widget*> _focus_widgets;
		// Persists the user's checkbox selection across rebuilds (adding a
		// player, re-selecting the bot type, ...).  ALL_PLAYERS means "every
		// player, including ones that join later".
		unsigned int _focus_mask;
};
#endif
