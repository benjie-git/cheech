/*
 *   The bot setup window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "prefs.hh"
#include "bot_catalog.hh"
#include "setup_bot_win.hh"

#include <glibmm/i18n.h>
#include <gtkmm/label.h>
#include <glibmm/main.h>


static unsigned int focus_mask_from_checks(
	const std::vector<Gtk::CheckButton*> &checks);

static unsigned int count_bits(unsigned int mask);

// Maps combo text back onto its value; unknown text falls back to the
// default (Default depth, Best smarts).
static unsigned int depth_from_label(const Glib::ustring &text);
static unsigned int level_from_label(const Glib::ustring &text);


setup_bot_win::setup_bot_win()
	: setup_bot_win_glade()
{
	_bot = NULL;
	_client = NULL;
	_status = GameServer::WaitingForPlayers;
	_focus_mask = BotBase::ALL_PLAYERS;

	type_box->signal_changed().connect(sigc::mem_fun(*this,
		&setup_bot_win::on_bot_type_changed));
	depth_box->signal_changed().connect(sigc::mem_fun(*this,
		&setup_bot_win::on_bot_type_changed));

	std::vector<Glib::ustring> families;
	families.push_back(BotUI::FRIENDLY);
	families.push_back(BotUI::NEUTRAL);
	families.push_back(BotUI::MEAN);
	for (size_t i = 0; i < families.size(); i++)
		type_box->append(families[i]);

	std::vector<unsigned int> depths = BotUI::depth_values();
	for (size_t i = 0; i < depths.size(); i++)
		depth_box->append(BotUI::depth_label(depths[i]));

	std::vector<unsigned int> levels = BotUI::skill_levels();
	for (size_t i = 0; i < levels.size(); i++)
		smarts_box->append(BotUI::skill_label(levels[i]));

	Prefs prefs;

	prefs.read();

	type_box->set_active_text(prefs.bot_type);
	depth_box->set_active_text(BotUI::depth_label(prefs.bot_depth));
	smarts_box->set_active_text(BotUI::skill_label(prefs.bot_level));
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
	if (!_bot)
		return;

	Prefs prefs;

	prefs.read();

	Glib::ustring family = type_box->get_active_text();
	unsigned int depth = depth_from_label(depth_box->get_active_text());
	unsigned int level = level_from_label(smarts_box->get_active_text());

	// The checkboxes are parallel to player numbers 1..N; only the family's
	// own list is meaningful, but setting both is harmless.
	_focus_mask = focus_mask_from_checks(_focus_checks);

	// The single-friend Friendly depth cap counts the players the bot will
	// help.  All boxes ticked means "everyone", i.e. every player currently
	// on the board (the new bot itself joins after them).
	unsigned int friend_count;
	if (_focus_mask == BotBase::ALL_PLAYERS && _client
		&& _client->get_board())
		friend_count = _client->get_board()->get_num_players();
	else
		friend_count = count_bits(_focus_mask);

	Glib::ustring type = BotUI::bot_type(family, depth, friend_count);

	delete _bot;
	_bot = BotBase::new_bot_of_type(type);
	if (!_bot)
		return;

	prefs.bot_type = family;
	prefs.bot_level = level;
	prefs.bot_depth = depth;
	prefs.smarts = BotUI::skill_smarts(level);

	// Use the engine's cute default (Rosie, Earl, Jimbo, ...), matching the
	// iOS setup.  An empty name would be rejected by the server and the bot
	// would never be added.
	_bot->set_name(_bot->get_default_name());
	// Bots commit without delay; the UI paces them with the move replay
	// (see main_win's seat-joining path).
	_bot->set_move_delay(0);
	_bot->set_think_delay(prefs.think_delay);
	_bot->set_smarts(prefs.smarts);

	if (_bot->get_type_name() == "Friendly")
		_bot->set_friends(_focus_mask);
	else if (_bot->get_type_name() == "Mean")
		_bot->set_enemies(_focus_mask);

	prefs.write();

	signal_add_bot(_bot);
	_bot = NULL;

	on_bot_type_changed();
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

	Glib::ustring family = type_box->get_active_text();

	if (!family.empty())
		prefs.bot_type = family;
	prefs.bot_level = level_from_label(smarts_box->get_active_text());
	prefs.bot_depth = depth_from_label(depth_box->get_active_text());
	prefs.smarts = BotUI::skill_smarts(prefs.bot_level);

	prefs.write();

	hide();
}


void setup_bot_win::on_defaults_activate()
{
	depth_box->set_active_text(BotUI::depth_label(0));
	smarts_box->set_active_text(BotUI::skill_label(1));

	_focus_mask = BotBase::ALL_PLAYERS;
	for (size_t i = 0; i < _focus_checks.size(); i++)
		_focus_checks[i]->set_active(true);
}


void setup_bot_win::on_bot_type_changed()
{
	if (_bot)
		delete _bot;

	Glib::ustring family = type_box->get_active_text();

	if (family.empty())
	{
		_bot = NULL;
		rebuild_focus();
		return;
	}

	unsigned int depth = depth_from_label(depth_box->get_active_text());
	Glib::ustring type = BotUI::bot_type(family, depth, 0);

	_bot = BotBase::new_bot_of_type(type);

	rebuild_focus();
}


static unsigned int count_bits(unsigned int mask)
{
	unsigned int count = 0;

	while (mask)
	{
		count += mask & 1;
		mask >>= 1;
	}
	return count;
}


static unsigned int depth_from_label(const Glib::ustring &text)
{
	std::vector<unsigned int> values = BotUI::depth_values();

	for (size_t i = 0; i < values.size(); i++)
		if (BotUI::depth_label(values[i]) == text)
			return values[i];
	return 0;
}


static unsigned int level_from_label(const Glib::ustring &text)
{
	std::vector<unsigned int> levels = BotUI::skill_levels();

	for (size_t i = 0; i < levels.size(); i++)
		if (BotUI::skill_label(levels[i]) == text)
			return levels[i];
	return 1;
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

	// Gtk::Grid grows its rows automatically; nothing to resize here.

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

	Gtk::Label *label = new Gtk::Label(
		friendly ? _("Friendly to:") : _("Mean to:"));
	label->set_xalign(0.5);
	label->set_yalign(0.5);
	label->set_justify(Gtk::Justification::RIGHT);
	label->set_wrap(false);
	label->set_use_markup(false);
	label->set_selectable(false);
	config_table->attach(*label, 0, 3, 1, 1);
	label->show();
	_focus_widgets.push_back(label);

	// Lay the checkboxes out horizontally on one line beside the label.
	Gtk::Box *hbox = new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6);
	hbox->set_hexpand(true);
	config_table->attach(*hbox, 1, 3, 1, 1);
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
		hbox->append(*check);
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
	set_focus(*type_box);
	type_box->grab_focus();
}
