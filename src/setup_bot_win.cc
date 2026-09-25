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

#include <glibmm/i18n.h>
#include <gtkmm/label.h>
#include <glibmm/main.h>
#include <gdkmm/window.h>


static unsigned int focus_mask_from_checks(
	const std::vector<Gtk::CheckButton*> &checks);


setup_bot_win::setup_bot_win()
	: setup_bot_win_glade()
{
	_bot = NULL;
	_client = NULL;
	_status = GameServer::WaitingForPlayers;
	_focus_mask = BotBase::ALL_PLAYERS;

	type_box->signal_changed().connect(sigc::mem_fun(*this,
		&setup_bot_win::on_bot_type_changed));

	type_box->append("LookAhead(2)");
	type_box->append("LookAhead(3)");
	type_box->append("LookAhead(4)");
	type_box->append("LookAhead(5)");
	type_box->append("Mean(2)");
	type_box->append("Mean(3)");
	type_box->append("Mean(4)");
	type_box->append("Mean(5)");
	type_box->append("Friendly(2)");
	type_box->append("Friendly(3)");
	type_box->append("Friendly(4)");

	Prefs prefs;

	prefs.read();

	type_box->set_active_text(prefs.bot_type);
	done_scale->set_value(prefs.done_delay * 0.0010001);
	move_scale->set_value(prefs.move_delay * 0.0010001);
	think_scale->set_value(prefs.think_delay * 0.0010001);	
	smarts_scale->set_value(prefs.smarts * 0.01);
}


setup_bot_win::~setup_bot_win()
{
	if (_bot)
		delete _bot;
}


void setup_bot_win::setup(GameServer::GameStatus status, GameClient *client)
{
	_status = status;
	_client = client;

	add_button->set_sensitive(status == GameServer::WaitingForPlayers);
	rebuild_focus();
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
		prefs.smarts = (unsigned int)(smarts_scale->get_value()*100 + 0.5);

		_bot->set_name(name_entry->get_text());
		_bot->set_move_delay(prefs.move_delay, prefs.done_delay);
		_bot->set_think_delay(prefs.think_delay);
		_bot->set_smarts(prefs.smarts);

		// The checkboxes are parallel to player numbers 1..N; only the family's
		// own list is meaningful, but setting both is harmless.
		_focus_mask = focus_mask_from_checks(_focus_checks);
		if (_bot->get_type_name() == "Friendly")
			_bot->set_friends(_focus_mask);
		else if (_bot->get_type_name() == "Mean")
			_bot->set_enemies(_focus_mask);

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
	prefs.smarts = (unsigned int)(smarts_scale->get_value()*100 + 0.5);

	prefs.write();

	hide();
}


void setup_bot_win::on_defaults_activate()
{
	done_scale->set_value(0.6);
	move_scale->set_value(0.4);
	think_scale->set_value(0.0);
	smarts_scale->set_value(1.0);

	_focus_mask = BotBase::ALL_PLAYERS;
	for (size_t i = 0; i < _focus_checks.size(); i++)
		_focus_checks[i]->set_active(true);
}


void setup_bot_win::on_bot_type_changed()
{
	if (_bot)
		delete _bot;

	_bot = BotBase::new_bot_of_type(type_box->get_active_text());

	name_entry->set_text(_bot->get_default_name());
	rebuild_focus();
}


// Reads the current checkbox state into a focus mask.  Returns ALL_PLAYERS
// when every box is ticked, so that players joining later are still included.
static unsigned int focus_mask_from_checks(
	const std::vector<Gtk::CheckButton*> &checks)
{
	if (checks.empty())
		return BotBase::ALL_PLAYERS;

	unsigned int mask = 0;
	bool all = true;

	for (size_t i = 0; i < checks.size(); i++)
	{
		if (checks[i]->get_active())
			mask |= (1u << i);
		else
			all = false;
	}

	return all ? BotBase::ALL_PLAYERS : mask;
}


// Rebuilds the friend/enemy checkbox row for the active bot type.  Friendly
// bots get a "Friendly to" list, Mean bots a "Mean to" list, and other types
// get none.  The row is added to the Configuration grid so its label lines up
// with the other config labels, and the user's selection is preserved across
// rebuilds via _focus_mask.
void setup_bot_win::rebuild_focus()
{
	// Preserve whatever the user had ticked before tearing the widgets down.
	if (!_focus_checks.empty())
		_focus_mask = focus_mask_from_checks(_focus_checks);

	for (size_t i = 0; i < _focus_widgets.size(); i++)
		config_table->remove(*_focus_widgets[i]);
	for (size_t i = 0; i < _focus_checks.size(); i++)
		delete _focus_checks[i];
	for (size_t i = 0; i < _focus_widgets.size(); i++)
		delete _focus_widgets[i];
	_focus_widgets.clear();
	_focus_checks.clear();

	config_table->resize(4, 2);

	if (!_bot || !_client || !_client->get_board())
		return;

	Glib::ustring family = _bot->get_type_name();
	bool friendly = (family == "Friendly");
	bool mean = (family == "Mean");
	if (!friendly && !mean)
		return;

	unsigned int num = _client->get_board()->get_num_players();
	if (num < 1 || num > 6)
		return;

	config_table->resize(5, 2);

	Gtk::Label *label = new Gtk::Label(
		friendly ? _("Friendly to:") : _("Mean to:"));
	label->set_alignment(0.5, 0.5);
	label->set_padding(0, 0);
	label->set_justify(Gtk::JUSTIFY_RIGHT);
	label->set_line_wrap(false);
	label->set_use_markup(false);
	label->set_selectable(false);
	config_table->attach(*label, 0, 1, 4, 5, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	label->show();
	_focus_widgets.push_back(label);

	// Lay the checkboxes out horizontally on one line beside the label.
	Gtk::HBox *hbox = new Gtk::HBox(false, 6);
	config_table->attach(*hbox, 1, 2, 4, 5, Gtk::EXPAND|Gtk::FILL,
		Gtk::FILL, 0, 0);
	hbox->show();

	for (unsigned int player = 1; player <= num; player++)
	{
		Glib::ustring name = _client->get_player_name(player);
		Glib::ustring text = name.empty()
			? Glib::ustring::compose(_("Player %1"), player)
			: Glib::ustring::compose("%1: %2", player, name);
		Gtk::CheckButton *check = new Gtk::CheckButton(text);
		check->set_active((_focus_mask == BotBase::ALL_PLAYERS)
			|| (_focus_mask & (1u << (player - 1))));
		hbox->pack_start(*check, Gtk::PACK_SHRINK);
		check->show();
		_focus_checks.push_back(check);
	}

	_focus_widgets.push_back(hbox);
}


void setup_bot_win::refresh_focus()
{
	rebuild_focus();
}


void setup_bot_win::present_focused()
{
	present();

	// Defer the focus request: if we grab focus while the menu popup still
	// holds the keyboard grab, the request is dropped and focus bounces back
	// to the parent window.  Doing it on the next main-loop iteration lets the
	// popup finish first.
	Glib::signal_timeout().connect_once(
		sigc::mem_fun(*this, &setup_bot_win::grab_dialog_focus), 1);
}


void setup_bot_win::grab_dialog_focus()
{
	Glib::RefPtr<Gdk::Window> window = get_window();

	if (window)
		window->focus(GDK_CURRENT_TIME);

	set_focus(*type_box);
	type_box->grab_focus();
}
