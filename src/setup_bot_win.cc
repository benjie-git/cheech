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

#include "prefs.hh"
#include "setup_bot_win.hh"


setup_bot_win::setup_bot_win()
	: setup_bot_win_glade()
{
	_bot = NULL;
	_status = GameServer::WaitingForPlayers;

	type_box->signal_changed().connect(sigc::mem_fun(*this,
		&setup_bot_win::on_bot_type_changed));

	type_box->append_text("Random");
	type_box->append_text("Simple(1)");
	type_box->append_text("LookAhead(2)");
	type_box->append_text("LookAhead(3)");
	type_box->append_text("LookAhead(4)");
//	type_box->append_text("LookAhead(5)");
	type_box->append_text("Friendly(2)");
	type_box->append_text("Friendly(3)");
	type_box->append_text("Friendly(4)");
//	type_box->append_text("Friendly(5)");
	type_box->append_text("Mean(2)");
	type_box->append_text("Mean(3)");
	type_box->append_text("Mean(4)");
//	type_box->append_text("Mean(5)");

	Prefs prefs;

	prefs.read();

	type_box->set_active_text(prefs.bot_type);
	done_scale->set_value(prefs.done_delay * 0.0010001);
	move_scale->set_value(prefs.move_delay * 0.0010001);
	think_scale->set_value(prefs.think_delay * 0.0010001);	
}


setup_bot_win::~setup_bot_win()
{
	if (_bot)
		delete _bot;
}


void setup_bot_win::setup(GameServer::GameStatus status)
{
	_status = status;

	add_button->set_sensitive(status == GameServer::WaitingForPlayers);
}


void setup_bot_win::on_add_button_activate()
{
	if (_bot)
	{
		Prefs prefs;

		prefs.read();

		prefs.bot_type = type_box->get_active_text();
		prefs.move_delay = (int)(move_scale->get_value()*1000 + 0.5);
		prefs.done_delay = (int)(done_scale->get_value()*1000 + 0.5);
		prefs.think_delay = (int)(think_scale->get_value()*1000 + 0.5);

		_bot->set_name(name_entry->get_text());
		_bot->set_move_delay(prefs.move_delay, prefs.done_delay);
		_bot->set_think_delay(prefs.think_delay);

		prefs.write();

		signal_add_bot(_bot);
		_bot = NULL;

		on_bot_type_changed();
	}
}


void setup_bot_win::on_remove_button_activate()
{
	signal_remove_bots();
}


void setup_bot_win::on_cmd_game_turn(unsigned int posn,
									 GameServer::GameStatus status,
									 unsigned int move_count)
{
	add_button->set_sensitive(status == GameServer::WaitingForPlayers);
}


void setup_bot_win::on_ok_button_activate()
{
	Prefs prefs;

	prefs.read();

	prefs.bot_type = type_box->get_active_text();
	prefs.move_delay = (int)(move_scale->get_value()*1000 + 0.5);
	prefs.done_delay = (int)(done_scale->get_value()*1000 + 0.5);
	prefs.think_delay = (int)(think_scale->get_value()*1000 + 0.5);

	prefs.write();

	hide();
}


void setup_bot_win::on_defaults_activate()
{
	done_scale->set_value(0.6);
	move_scale->set_value(0.4);
	think_scale->set_value(0.0);
}


void setup_bot_win::on_bot_type_changed()
{
	if (_bot)
		delete _bot;

	_bot = BotBase::new_bot_of_type(type_box->get_active_text());

	name_entry->set_text(_bot->get_default_name());
}
