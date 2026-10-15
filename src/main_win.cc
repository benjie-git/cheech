/*
 *  The main Chinese Checkers Game Window.
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

#include "config.h"
#include <glibmm/i18n.h>
#include <glibmm/main.h>

#include <gtkmm/textview.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/application.h>
#include <gdkmm/display.h>
#include <gdkmm/clipboard.h>

#include <map>

#include "main_win.hh"

#include "bot_catalog.hh"
#include "cheech_move_gate.hh"
#include "utility.hh"


// Gives the window keyboard focus on the next main-loop iteration.  Doing it
// immediately fails when the request comes from a menu popup that still holds
// the keyboard grab, so the window manager would hand focus back to the parent.
static void grab_window_focus(Gtk::Window *window)
{
	if (!window->get_focus())
		window->grab_focus();
}


static void present_and_focus(Gtk::Window *window)
{
	// show() first so a re-opened dialog takes the normal map path;
	// present() alone only re-presents an already-mapped surface.
	if (!window->get_visible())
		window->show();
	window->present();
	Glib::signal_timeout().connect_once(
		sigc::bind(sigc::ptr_fun(&grab_window_focus), window), 1);
}


main_win::main_win() : main_win_glade()
{
	_server = NULL;
	_ajax_server = NULL;
	_client = NULL;
	_current_player = 0;
	_move_count = 1;
	_game_status = GameServer::End;
	_leaving_game = false;
	_next_seat = 0;
	_seat_join_ticks = 0;
	_hosted_port = 0;
	_active_input_client = NULL;
	_confirm_quit_dialog = NULL;
	_anim_step_ms = 0;
	_label_player = 0;

	game_view->sig_anim_started.connect(
		sigc::mem_fun(*this, &main_win::on_anim_started));
	game_view->sig_anim_idle.connect(
		sigc::mem_fun(*this, &main_win::on_anim_idle));

	// One-time connections from the dialogs, forwarded to the current
	// primary client (see on_dialog_change_color).
	_color_win.signal_change_color.connect(sigc::mem_fun(*this,
		&main_win::on_dialog_change_color));
	_name_win.signal_change_name.connect(sigc::mem_fun(*this,
		&main_win::on_dialog_change_name));
	_setup_game_win.signal_restart_game.connect(sigc::mem_fun(*this,
		&main_win::on_game_setup));

	_player_name[0]=player_name1;
	_player_name[1]=player_name2;
	_player_name[2]=player_name3;
	_player_name[3]=player_name4;
	_player_name[4]=player_name5;
	_player_name[5]=player_name6;

	_player_peg[0]=player_peg1;
	_player_peg[1]=player_peg2;
	_player_peg[2]=player_peg3;
	_player_peg[3]=player_peg4;
	_player_peg[4]=player_peg5;
	_player_peg[5]=player_peg6;

	for (int i=0; i<6; i++)
		_finished_in_moves[i] = 0;

	game_view->evt_unhandled_key.connect(sigc::mem_fun(*this,
		&main_win::append_to_chat_entry));

	// The win.* actions referenced by the Gio::Menu model built in
	// main_win_glade.
	_win_actions = Gio::SimpleActionGroup::create();
	_win_actions->add_action("new_game", sigc::mem_fun(*this,
		&main_win::on_new_game_activate));
	_end_game_action = _win_actions->add_action("end_game", sigc::mem_fun(*this,
		&main_win::on_end_game_activate));
	_leave_game_action = _win_actions->add_action("leave_game",
		sigc::mem_fun(*this, &main_win::on_leave_game_activate));
	_game_settings_action = _win_actions->add_action("game_settings",
		sigc::mem_fun(*this, &main_win::on_game_settings_activate));
	_rotate_action = _win_actions->add_action("rotate", sigc::mem_fun(*this,
		&main_win::on_rotate_activate));
	_shuffle_action = _win_actions->add_action("shuffle", sigc::mem_fun(*this,
		&main_win::on_shuffle_activate));
	_restart_game_action = _win_actions->add_action("restart_game",
		sigc::mem_fun(*this, &main_win::on_restart_game_activate));
	_win_actions->add_action("quit", sigc::mem_fun(*this,
		&main_win::on_quit_activate));
	_undo_action = _win_actions->add_action("undo", sigc::mem_fun(*this,
		&main_win::on_undo_activate));
	_redo_action = _win_actions->add_action("redo", sigc::mem_fun(*this,
		&main_win::on_redo_activate));
	_show_last_move_action = _win_actions->add_action("show_last_move",
		sigc::mem_fun(*this, &main_win::on_show_last_move_activate));
	_win_actions->add_action("cut", sigc::mem_fun(*this,
		&main_win::on_cut_activate));
	_win_actions->add_action("copy", sigc::mem_fun(*this,
		&main_win::on_copy_activate));
	_win_actions->add_action("paste", sigc::mem_fun(*this,
		&main_win::on_paste_activate));
	_win_actions->add_action("delete", sigc::mem_fun(*this,
		&main_win::on_delete_activate));
	_change_name_action = _win_actions->add_action("change_name",
		sigc::mem_fun(*this, &main_win::on_change_name_activate));
	_change_color_action = _win_actions->add_action("change_color",
		sigc::mem_fun(*this, &main_win::on_change_color_activate));
	_add_computer_player_action = _win_actions->add_action(
		"add_computer_player", sigc::mem_fun(*this,
		&main_win::on_add_computer_player_activate));
	_remove_computer_players_action = _win_actions->add_action(
		"remove_computer_players", sigc::mem_fun(*this,
		&main_win::on_remove_computer_players_activate));
	_setup_computer_player_action = _win_actions->add_action(
		"setup_computer_player", sigc::mem_fun(*this,
		&main_win::on_setup_computer_player_activate));
	_win_actions->add_action("show_chat", sigc::mem_fun(*this,
		&main_win::on_show_chat_activate));
	_win_actions->add_action("how_to_play", sigc::mem_fun(*this,
		&main_win::on_how_to_play_activate));
	_win_actions->add_action("about", sigc::mem_fun(*this,
		&main_win::on_about_activate));
	insert_action_group("win", _win_actions);

	// The window is only ever hidden on close (see on_close_request).
	signal_close_request().connect(sigc::mem_fun(*this,
		&main_win::on_close_request), false);

	update_menus();

	_new_game_win.set_transient_for(*this);
	_new_game_win.signal_start_server.connect(sigc::mem_fun(*this,
		&main_win::start_server));
	_new_game_win.signal_start_client.connect(sigc::mem_fun(*this,
		&main_win::start_client));

	_setup_bot_win.set_transient_for(*this);
	_setup_bot_win.signal_add_bot.connect(sigc::mem_fun(*this,
		&main_win::add_bot));
	_setup_bot_win.signal_remove_bots.connect(sigc::mem_fun(*this,
		&main_win::on_remove_computer_players_activate));

	_color_win.set_transient_for(*this);
	_name_win.set_transient_for(*this);
	_about_win.set_transient_for(*this);
	_help_win.set_transient_for(*this);
	_setup_game_win.set_transient_for(*this);

	// These dialogs live for the whole app run; the window close button
	// must hide them rather than destroy them (GTK4's default), otherwise
	// the widget surface is torn down and the next open comes up empty.
	_new_game_win.set_hide_on_close(true);
	_setup_bot_win.set_hide_on_close(true);
	_color_win.set_hide_on_close(true);
	_name_win.set_hide_on_close(true);
	_about_win.set_hide_on_close(true);
	_help_win.set_hide_on_close(true);
	_setup_game_win.set_hide_on_close(true);

	_new_game_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_setup_bot_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_color_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_name_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_about_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_help_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
	_setup_game_win.signal_hide().connect(sigc::mem_fun(*this,
		&main_win::on_dialog_hidden));
}


void main_win::on_dialog_hidden()
{
	if (get_visible())
		present();
}


main_win::~main_win()
{
	delete _confirm_quit_dialog;
	if (_server)
		stop_server();
	if (_client)
		on_leave_game_activate();
}


void main_win::on_realize()
{
	// The keyboard accelerators are registered by cheech.cc in its
	// application startup handler; on_realize() runs before the window has
	// been added to the application, so there is no application here yet.
	update_menus();
}


bool main_win::start_server(unsigned int port, unsigned int cheechweb_port,
	bool long_jumps, bool hop_others, bool stop_others,
	std::vector<NewGameSeat> seats)
{
	if (_server)
	{
		// Ending the current game needs confirmation; the response handler
		// stops it and starts the new game if the user answers yes.
		confirm_then([=]()
			{
				stop_server();
				do_start_server(port, cheechweb_port, long_jumps, hop_others,
					stop_others, seats);
			});
		return false;
	}

	return do_start_server(port, cheechweb_port, long_jumps, hop_others,
		stop_others, seats);
}


// Creates the server and its ajax bridge, then joins the hosted seats.
// Assumes any previous game has already been confirmed and stopped.
bool main_win::do_start_server(unsigned int port, unsigned int cheechweb_port,
	bool long_jumps, bool hop_others, bool stop_others,
	std::vector<NewGameSeat> seats)
{
	// Hosting replaces any game this window is joined to (its client would
	// otherwise be overwritten and leaked here).
	if (_client)
		on_leave_game_activate();

	_server = new GameServer(port, seats.size(), long_jumps,
							 hop_others, stop_others);
	_server->evt_message.connect(sigc::mem_fun(*this, &main_win::show_message));
	_server->new_game();

	if (!_server->ready())
	{
		delete _server;
		_server = NULL;
		update_menus();
		return false;
	}

	if (cheechweb_port)
	{
		_ajax_server = new AjaxServer();
		if (!_ajax_server->listen(cheechweb_port, "localhost", port))
		{
			delete _ajax_server;
			_ajax_server = NULL;
		}
	}

	start_hosted_seats(seats, port);

	update_menus();

	return true;
}


// Wires a client to the window's game-state handlers.  The primary client
// (the one driving menus, chat and the change name/color dialogs) also gets
// the conflict dialogs; secondary seat clients resolve conflicts themselves.
void main_win::connect_client(GameClient *client, bool primary)
{
	// The replay's per-hop pace, as saved by the New Game dialog
	// (0 disables the replay).  Prefs can only change between games, so
	// one read per connection covers every bot-joining path.
	{
		Prefs prefs;
		prefs.read();
		_anim_step_ms = prefs.move_delay;
	}

	client->evt_disconnected.connect(sigc::mem_fun(*this,
		&main_win::on_leave_game_activate));
	client->evt_cancelled.connect(sigc::mem_fun(*this,
		&main_win::on_leave_game_activate));
	client->evt_message.connect(sigc::mem_fun(*this,
		&main_win::show_message));
	client->cmd_player_add.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_add));
	client->cmd_player_remove.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_remove));
	client->cmd_player_finish.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_finish));
	client->cmd_game_resync.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_resync));
	if (primary)
	{
		client->cmd_choose_new_name.connect(sigc::mem_fun(*this,
			&main_win::on_cmd_choose_new_name));
		client->cmd_choose_new_color.connect(sigc::mem_fun(*this,
			&main_win::on_cmd_choose_new_color));
	}
	client->cmd_game_turn.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_turn));
	client->cmd_game_undo_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_undo_move));
	client->cmd_game_show_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_show_move));
	client->cmd_game_hide_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_hide_move));
	client->cmd_game_make_move.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::on_cmd_game_make_move), client));
}


// Hooks the name/color/game-settings dialogs to a client.  Only used for the
// primary client; repeated connections match the existing start_client
// behavior.
void main_win::connect_client_dialogs(GameClient *client)
{
	client->evt_disconnected.connect(sigc::mem_fun(_color_win,
		&color_win::on_disconnected));
	client->evt_disconnected.connect(sigc::mem_fun(_name_win,
		&name_win::on_disconnected));

	client->cmd_game_turn.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_cmd_game_turn));
	client->evt_disconnected.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_disconnected));
	_setup_game_win.setup(1, false, false, false, _game_status);

	_setup_bot_win.setup(_game_status, client);
	client->cmd_game_turn.connect(sigc::mem_fun(_setup_bot_win,
		&setup_bot_win::on_cmd_game_turn));
}


// The dialog signals are connected once (in the constructor) and forwarded
// to the current primary client, so starting new games can't stack up
// duplicate connections on the dialogs.
void main_win::on_dialog_change_color(int color)
{
	if (_client)
		_client->change_color(color);
}


void main_win::on_dialog_change_name(Glib::ustring name)
{
	if (_client)
		_client->change_name(name);
}


void main_win::start_client(Glib::ustring host, unsigned int port,
	Glib::ustring name, unsigned int color, bool spectator)
{
	if (_client)
		on_leave_game_activate();

	_client = new GameClient();
	_client->change_name(name);
	_client->change_color((spectator) ? 0 : color);

	connect_client(_client, true);
	_client->evt_connected.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::add_join_hostname), host));

	game_view->set_client(_client);
	_active_input_client = _client;

	_client->join_game(host, port, spectator);

	update_menus();

	connect_client_dialogs(_client);
}


// Joins the seats of a freshly hosted game, one at a time so the player
// numbers match the seat order the user arranged in the New Game dialog.
void main_win::start_hosted_seats(std::vector<NewGameSeat> seats,
	unsigned int port)
{
	clear_seats();
	_hosted_port = port;

	// Rotate to the first human seat's start position before anyone joins,
	// so the board doesn't visibly reorient as seats appear.  Player numbers
	// are assigned by join order and seats join in order, so the first
	// human's number is one more than the computer seats ahead of it.
	unsigned int computers = 0;
	for (size_t i = 0; i < seats.size(); i++)
	{
		if (seats[i].kind == HostSeat::SEAT_HUMAN)
		{
			game_view->rotate_to_seat(seats.size(), 1 + computers);
			break;
		}
		else if (seats[i].kind == HostSeat::SEAT_COMPUTER)
			computers++;
	}

	// Map the dialog's seat ids to seat indices for focus lists.
	std::map<unsigned int, unsigned int> index_of_id;
	for (size_t i = 0; i < seats.size(); i++)
		index_of_id[seats[i].id] = i;

	for (size_t i = 0; i < seats.size(); i++)
	{
		HostSeat seat;
		seat.kind = seats[i].kind;
		seat.name = seats[i].name;
		seat.color = seats[i].color;
		seat.family = seats[i].family;
		seat.level = seats[i].level;
		seat.depth = seats[i].depth;
		seat.focus_all = seats[i].focus_all;
		seat.client = NULL;
		seat.bot = NULL;
		seat.player_number = 0;
		for (size_t j = 0; j < seats[i].focus_ids.size(); j++)
		{
			std::map<unsigned int, unsigned int>::iterator f =
				index_of_id.find(seats[i].focus_ids[j]);
			if (f != index_of_id.end())
				seat.focus_ids.push_back(f->second);
		}
		_seats.push_back(seat);
	}

	// A seat that used to be remote carries no color; players need a real
	// one or the server refuses the join (a bot has no one to answer its
	// request for a new color and would never appear).
	for (size_t i = 0; i < _seats.size(); i++)
		if (_seats[i].kind != HostSeat::SEAT_REMOTE && !_seats[i].color)
			for (unsigned int c = 1; c <= 8; c++)
			{
				bool used = false;
				for (size_t j = 0; j < _seats.size(); j++)
					if (_seats[j].color == c)
						used = true;
				if (!used)
				{
					_seats[i].color = c;
					break;
				}
			}

	// The first human seat's client drives menus, chat and dialogs; with no
	// human seats at all, a spectator client mirrors the board instead.
	for (size_t i = 0; i < _seats.size(); i++)
	{
		if (_seats[i].kind == HostSeat::SEAT_HUMAN)
		{
			_client = new GameClient();
			_client->change_name(_seats[i].name);
			_client->change_color(_seats[i].color);
			_seats[i].client = _client;
			connect_client(_client, true);
			_client->cmd_set_player_number.connect(sigc::bind(
				sigc::mem_fun(*this, &main_win::on_seat_player_number),
				_client));
			game_view->set_client(_client);
			_active_input_client = _client;
			connect_client_dialogs(_client);
			break;
		}
	}
	if (!_client)
	{
		_client = new GameClient();
		_client->change_name("Display");
		_client->change_color(0);
		connect_client(_client, true);
		game_view->set_client(_client);
		_active_input_client = _client;
		connect_client_dialogs(_client);
		_client->join_game("localhost", port, true);
	}

	// Join the remaining seats in order, polling until each has a player
	// number before starting the next.
	_next_seat = 0;
	_seat_join_ticks = 0;
	join_next_seat();
	// Poll fast like the iOS host (10ms): seats join one at a time, waiting
	// for each player number, but a 10ms tick makes that imperceptible.
	_seat_poll = Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&main_win::poll_seat_join), 10);
}


void main_win::join_next_seat()
{
	// Remote seats never join; skip them.
	while (_next_seat < _seats.size() &&
		_seats[_next_seat].kind == HostSeat::SEAT_REMOTE)
		_next_seat++;

	if (_next_seat >= _seats.size())
	{
		apply_seat_focus();
		return;
	}

	HostSeat &seat = _seats[_next_seat];

	if (seat.kind == HostSeat::SEAT_HUMAN)
	{
		if (!seat.client)
		{
			seat.client = new GameClient();
			seat.client->change_name(seat.name);
			seat.client->change_color(seat.color);
			connect_client(seat.client, false);
			hook_seat_client(seat.client, _next_seat);
		}
		seat.client->join_game("localhost", _hosted_port, false);
	}
	else if (seat.kind == HostSeat::SEAT_COMPUTER)
	{
		// "Everyone" from the new bot's point of view: every other seat.
		unsigned int friend_count = seat.focus_all
			? _seats.size() - 1 : seat.focus_ids.size();
		Glib::ustring type = BotUI::bot_type(seat.family, seat.depth,
			friend_count);

		seat.bot = BotBase::new_bot_of_type(type);
		if (!seat.bot)
		{
			type = BotUI::bot_type(BotUI::NEUTRAL, 0, friend_count);
			seat.bot = BotBase::new_bot_of_type(type);
		}
		if (!seat.bot)
		{
			show_message("Cannot create a computer player of type "
				+ type);
			return;
		}
		seat.bot->set_name(seat.name);
		seat.bot->set_color(seat.color);
		seat.bot->set_smarts(BotUI::skill_smarts(seat.level));
		// The engine's pacing now comes from the UI's move replay (the
		// animation gate holds each commit until the previous move has
		// finished animating), so the bot itself commits without delay.
		// The replay's pace was picked up in connect_client.
		seat.bot->set_move_delay(0);
		_bots.push_back(seat.bot);
		seat.bot->evt_disconnected.connect(sigc::bind(sigc::mem_fun(*this,
			&main_win::bot_disconnected), seat.bot));
		GameClient *bot_client = seat.bot->get_game_client();
		bot_client->cmd_set_player_number.connect(sigc::bind(
			sigc::mem_fun(*this, &main_win::on_seat_player_number),
			bot_client));
		seat.bot->join_game("localhost", _hosted_port);
	}

	// SEAT_REMOTE: leave the seat open for a joiner.
}


// Waits for the seat that is currently joining to receive its player number
// (the server assigns numbers by connection-arrival order), then starts the
// next seat.  Gives up on a seat after 5 seconds (500 ticks at 10ms) so a
// stalled join cannot block the rest.
bool main_win::poll_seat_join()
{
	if (_seats.empty())
		return false;

	if (_next_seat >= _seats.size())
	{
		apply_seat_focus();
		return false;
	}

	HostSeat &seat = _seats[_next_seat];
	GameClient *client = seat.client ? seat.client
		: (seat.bot ? seat.bot->get_game_client() : NULL);
	unsigned int num = client ? client->get_my_player_number() : 0;

	if (num)
	{
		seat.player_number = num;
		_next_seat++;
		_seat_join_ticks = 0;
		join_next_seat();
		return _next_seat < _seats.size();
	}

	if (++_seat_join_ticks >= 500)
	{
		_seat_join_ticks = 0;
		_next_seat++;
		join_next_seat();
		return _next_seat < _seats.size();
	}

	return true;
}


// Applies each computer seat's focus list now that every seat has its player
// number.  Focus entries are seat indices; the mask is over player numbers.
void main_win::apply_seat_focus()
{
	for (size_t i = 0; i < _seats.size(); i++)
	{
		HostSeat &seat = _seats[i];
		if (seat.kind != HostSeat::SEAT_COMPUTER || !seat.bot || seat.focus_all)
			continue;

		unsigned int mask = 0;
		bool complete = true;
		for (size_t j = 0; j < seat.focus_ids.size(); j++)
		{
			if (seat.focus_ids[j] >= _seats.size() ||
				!_seats[seat.focus_ids[j]].player_number)
			{
				complete = false;
				break;
			}
			mask |= 1 << (_seats[seat.focus_ids[j]].player_number - 1);
		}
		if (!complete)
			continue;

		Glib::ustring family = seat.bot->get_type_name();
		if (family == BotUI::FRIENDLY)
			seat.bot->set_friends(mask);
		else if (family == BotUI::MEAN)
			seat.bot->set_enemies(mask);
	}
}


// Non-primary human seats resolve name/color clashes themselves (mirroring
// the iOS host) instead of popping the change dialogs.
void main_win::hook_seat_client(GameClient *client, size_t index)
{
	client->cmd_choose_new_name.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::on_seat_choose_new_name), client, (size_t) index));
	client->cmd_choose_new_color.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::on_seat_choose_new_color), client));
	client->cmd_set_player_number.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::on_seat_player_number), client));
}


void main_win::on_seat_choose_new_name(Glib::ustring name,
	GameClient *client, size_t index)
{
	if (index < _seats.size() && _seats[index].client == client)
		client->change_name(_seats[index].name + " " +
			util::to_str(index + 1));
}


void main_win::on_seat_choose_new_color(Glib::ustring name, int color,
	GameClient *client)
{
	bool used[9] = {false};
	unsigned int my = client->get_my_player_number();
	if (client->get_board())
		for (unsigned int p = 1;
			p <= client->get_board()->get_num_players(); p++)
		{
			if (p == my)
				continue;
			unsigned int c = client->get_player_color(p);
			if (c >= 1 && c <= 8)
				used[c] = true;
		}
	for (unsigned int c = 1; c <= 8; c++)
		if (!used[c])
		{
			client->change_color(c);
			return;
		}
}


// A rotate/shuffle renumbers every player (SET_PLAYER_NUMBER per client).
// Keep the seat mapping in step so the board stays oriented to the first
// human seat and turns route to the right seat, and re-apply bot focus
// lists, which are masks over player numbers.
void main_win::on_seat_player_number(unsigned int n, GameClient *client)
{
	for (size_t i = 0; i < _seats.size(); i++)
	{
		GameClient *seat_client = _seats[i].client ? _seats[i].client
			: (_seats[i].bot ? _seats[i].bot->get_game_client() : NULL);
		if (seat_client == client)
		{
			_seats[i].player_number = n;
			break;
		}
	}
	apply_seat_focus();
}


// In a hosted hot-seat game, the human seat whose turn it is drives the
// board; otherwise the primary client just watches.
main_win::HostSeat *main_win::active_seat()
{
	if (_seats.empty() || !_current_player)
		return NULL;

	for (size_t i = 0; i < _seats.size(); i++)
		if (_seats[i].kind == HostSeat::SEAT_HUMAN &&
			_seats[i].player_number == _current_player)
			return &_seats[i];

	return NULL;
}


// Points game_view at the client that may move this turn.  The board itself
// stays put: the first human seat keeps the bottom (set on resync).
void main_win::route_input()
{
	if (_seats.empty())
		return;

	HostSeat *seat = active_seat();
	GameClient *input = seat ? seat->client : _client;

	if (input != _active_input_client)
	{
		_active_input_client = input;
		// Re-point the input without replacing the board: all seat clients
		// hold the same board, so a running or parked replay keeps playing
		// across the handoff instead of being cancelled mid-flight.
		game_view->set_input_client(input);
	}

	game_view->set_locked(seat == NULL);
}


void main_win::clear_seats()
{
	// A new hosted game is about to rebuild the seats; stop the old poll
	// timer before it can drive the new seat vector.
	_seat_poll.disconnect();

	// Seat bots were already removed from _bots (and disposed) by
	// on_remove_computer_players_activate(); the seat clients are
	// disconnected here and freed on a later main-loop iteration, once
	// their disconnect processing has finished.
	for (size_t i = 0; i < _seats.size(); i++)
		if (_seats[i].client && _seats[i].client != _client)
		{
			GameClient *client = _seats[i].client;
			client->leave_game();
			Glib::signal_timeout().connect(sigc::bind_return(
				sigc::mem_fun(*client, &GameClient::dispose), false), 0);
		}
	_seats.clear();
	_next_seat = 0;
	_seat_join_ticks = 0;
}


void main_win::add_join_hostname(Glib::ustring host)
{
	Prefs p;

	p.read();
	p.set_join_hostname(host);
	p.write();
}


void main_win::on_new_game_activate()
{
	// The dialog's notebook remembers the last-used page (New Game or Join).
	present_and_focus(&_new_game_win);
}


void main_win::on_restart_game_activate()
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				confirm_then(sigc::mem_fun(*_bots[0]->get_game_client(),
					&GameClient::restart_game));
		}
		else
			confirm_then(sigc::mem_fun(*_client, &GameClient::restart_game));
	}
}


void main_win::on_rotate_activate()
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				confirm_then(sigc::mem_fun(*_bots[0]->get_game_client(),
					&GameClient::rotate_players));
		}
		else
			confirm_then(sigc::mem_fun(*_client, &GameClient::rotate_players));
	}
}


void main_win::on_shuffle_activate()
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				confirm_then(sigc::mem_fun(*_bots[0]->get_game_client(),
					&GameClient::shuffle_players));
		}
		else
			confirm_then(sigc::mem_fun(*_client, &GameClient::shuffle_players));
	}
}


void main_win::on_game_setup(unsigned int num_players, bool long_jumps,
							 bool hop_others, bool stop_others)
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
			{
				if (_game_status != GameServer::Playing)
					_bots[0]->get_game_client()->reconfigure_game(
						num_players, long_jumps, hop_others, stop_others);
				else
					confirm_then(sigc::bind(sigc::mem_fun(
						*_bots[0]->get_game_client(),
						&GameClient::reconfigure_game),
						num_players, long_jumps, hop_others, stop_others));
			}
		}
		else
		{
			if (_game_status != GameServer::Playing)
				_client->reconfigure_game(
						num_players, long_jumps, hop_others, stop_others);
			else
				confirm_then(sigc::bind(sigc::mem_fun(*_client,
					&GameClient::reconfigure_game),
					num_players, long_jumps, hop_others, stop_others));
		}
	}
	_setup_game_win.hide();
}


void main_win::on_end_game_activate()
{
	if (_server)
		confirm_then(sigc::mem_fun(*this, &main_win::stop_server));
}


void main_win::stop_server()
{
	_seat_poll.disconnect();

	if (_ajax_server)
	{
		_ajax_server->close();
		Glib::signal_timeout().connect(sigc::bind_return(
			sigc::mem_fun(*_ajax_server, &AjaxServer::dispose), false), 0);
		_ajax_server = NULL;
	}

	if (_server)
	{
		on_leave_game_activate();

		util::delay_ms(100);
		_server->end_game();
		Glib::signal_timeout().connect(sigc::bind_return(
			sigc::mem_fun(*_server, &GameServer::dispose), false), 0);
		_server = NULL;

		update_menus();
	}
}


void main_win::on_leave_game_activate()
{
	// Disconnecting seat clients re-emits evt_disconnected (and
	// evt_cancelled) into this handler while it is still running; a second
	// entry would complete the teardown and leave this run dereferencing a
	// NULL _client and a cleared _seats vector.
	if (!_client || _leaving_game)
		return;

	_leaving_game = true;

	if (_client)
	{
		_label_player = 0;
		game_view->hide_move();
		game_view->queue_draw();
		game_view->set_client(NULL);
		game_view->rebuild_board();
		game_view->set_locked(true);
		on_remove_computer_players_activate();
		clear_seats();
		_active_input_client = NULL;
		_client->leave_game();
		_current_player = 0;
		_game_status = GameServer::End;
		// The setup-bot dialog keeps the client pointer between games; hand
		// it a NULL one so it cannot reach the client being disposed below.
		_setup_bot_win.setup(_game_status, NULL);
		_move_count = 1;
		move_counter->set_text("");
		_last_move.clear();

		// Free the client on a later main-loop iteration: this handler may
		// be running from the client's own evt_disconnected emission, and
		// its slots must stay valid until the emission returns.
		GameClient *client = _client;
		Glib::signal_timeout().connect(sigc::bind_return(
			sigc::mem_fun(*client, &GameClient::dispose), false), 0);
		_client = NULL;

		update_menus();
	}

	_leaving_game = false;
}


void main_win::update_menus()
{
	_end_game_action->set_enabled(_server != NULL);

	_leave_game_action->set_enabled(_client != NULL);
	_game_settings_action->set_enabled(_client != NULL);
	_restart_game_action->set_enabled(_client != NULL);
	_rotate_action->set_enabled(_client != NULL);
	_shuffle_action->set_enabled(_client != NULL);
	_change_name_action->set_enabled(_client != NULL);
	_change_color_action->set_enabled(_client != NULL);
	_add_computer_player_action->set_enabled(_client != NULL &&
		_game_status == GameServer::WaitingForPlayers);
	_remove_computer_players_action->set_enabled(_client != NULL &&
		_bots.size() > 0);
	_show_last_move_action->set_enabled(_client != NULL);
	_undo_action->set_enabled(_client != NULL);
	_redo_action->set_enabled(_client != NULL);
	// Only meaningful once a game is underway.  Note the entire first
	// turn reports Start, not Playing (get_game_status()).
	_setup_computer_player_action->set_enabled(_client != NULL &&
		_game_status != GameServer::WaitingForPlayers &&
		_game_status != GameServer::End);

	if (_client)
	{
		bool active_member = (!_client->is_spectator()) || (_bots.size() > 0);
		_game_settings_action->set_enabled(active_member);
		_restart_game_action->set_enabled(active_member);
		_rotate_action->set_enabled(active_member);
		_shuffle_action->set_enabled(active_member);
	}
}


void main_win::on_change_color_activate()
{
	if (_client)
	{
		_color_win.set_force_info(NULL, 0);
		present_and_focus(&_color_win);
	}
}


void main_win::on_change_name_activate()
{
	if (_client)
	{
		Glib::ustring name = _client->get_my_name();
		_name_win.set_name(&name);
		present_and_focus(&_name_win);

	}
}


void main_win::on_resync_activate()
{
	if (_client)
		_client->resync_game();
}


void main_win::on_show_last_move_activate()
{
	if (_client && _client->get_board() &&
		_last_move.size() > 0)
	{
		game_view->show_move(&_last_move);
		game_view->queue_draw();
		util::delay_ms(1500);
		game_view->hide_move();
		game_view->queue_draw();
	}
}


void main_win::on_add_computer_player_activate()
{
	if (_client)
	{
		Prefs prefs;
		prefs.read();

		// "Everyone" from the new bot's point of view: every player already
		// on the board (it joins after them).
		unsigned int friend_count = _client->get_board()
			? _client->get_board()->get_num_players() : 0;
		Glib::ustring type = BotUI::bot_type(prefs.bot_type,
			prefs.bot_depth, friend_count);

		BotBase *bot = BotBase::new_bot_of_type(type);
		if (!bot)
		{
			prefs.bot_type = BotUI::NEUTRAL;
			prefs.bot_level = 1;
			prefs.bot_depth = 0;
			prefs.write();
			bot = BotBase::new_bot_of_type(BotUI::bot_type(prefs.bot_type,
				prefs.bot_depth, friend_count));
		}
		if (!bot)
		{
			show_message("Cannot create a computer player of type " + type);
			return;
		}
		// See the seat-joining path: bots commit without delay and the UI
		// paces them with the move replay.
		bot->set_move_delay(0);
		bot->set_think_delay(prefs.think_delay);
		bot->set_smarts(BotUI::skill_smarts(prefs.bot_level));
		// A bot without a name is never added: the server rejects the empty
		// name and nothing answers its request for a new one.
		bot->set_name(bot->get_default_name());

		add_bot(bot);
	}
}


void main_win::on_game_settings_activate()
{
	if (_client && _client->get_board())
	{
		_setup_game_win.setup(
			_client->get_board()->get_num_players(),
			_client->get_board()->get_long_jumps_allowed(),
			_client->get_board()->get_hop_others_allowed(),
			_client->get_board()->get_stop_others_allowed(),
			_game_status);
		present_and_focus(&_setup_game_win);
	}
}


void main_win::on_setup_computer_player_activate()
{
	_setup_bot_win.refresh_focus();
	_setup_bot_win.present_focused();
}


void main_win::on_show_chat_activate()
{
	if (chat_area->get_visible())
	{
		chat_area->hide();
		show_chat_item->set_label(_("Show Chat"));
	}
	else
	{
		chat_area->show();
		show_chat_item->set_label(_("Hide Chat"));
	}
}


void main_win::on_remove_computer_players_activate()
{
	BotBase *bot;

	while (_bots.size() > 0)
	{
		bot = _bots.back();
		_bots.pop_back();
		bot->leave_game();
		// Free the bot on a later main-loop iteration: leave_game() may be
		// emitting the bot's own evt_disconnected right now.  The bot is no
		// longer in _bots, so bot_disconnected() will not dispose it too.
		Glib::signal_timeout().connect(sigc::bind_return(
			sigc::mem_fun(*bot, &BotBase::dispose), false), 0);
	}
}


void main_win::add_bot(BotBase *bot)
{
	if (_client)
	{
		_bots.push_back(bot);
		bot->join_game(_client->get_host_name(), _client->get_port());
		bot->evt_disconnected.connect(sigc::bind(sigc::mem_fun(*this,
			&main_win::bot_disconnected), bot));
	}
}


void main_win::bot_disconnected(BotBase *bot)
{
	for (std::vector<BotBase*>::iterator b = _bots.begin();
		b < _bots.end(); b++)
		if (*b == bot)
		{
			_bots.erase(b);
			// Free the bot on a later main-loop iteration: this handler may
			// be running from the bot's own evt_disconnected emission.
			Glib::signal_timeout().connect(sigc::bind_return(
				sigc::mem_fun(*bot, &BotBase::dispose), false), 0);
			break;
		}
}


void main_win::on_undo_activate()
{
	if (_client)
	{
		_client->undo_move();
		_last_move.clear();
	}
}


void main_win::on_redo_activate()
{
	if (_client)
		_client->redo_move();
}


void main_win::on_cut_activate()
{
	Glib::RefPtr<Gdk::Clipboard> clipboard = get_display()->get_clipboard();
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->cut_clipboard(clipboard, true);
	else if (focused == chat_entry)
	{
		// GTK4 dropped the entry clipboard helpers; copy the selection and
		// delete it.
		int start = 0, end = 0;
		if (chat_entry->get_selection_bounds(start, end))
		{
			clipboard->set_text(chat_entry->get_chars(start, end));
			chat_entry->delete_selection();
		}
	}
}


void main_win::on_copy_activate()
{
	Glib::RefPtr<Gdk::Clipboard> clipboard = get_display()->get_clipboard();
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->copy_clipboard(clipboard);
	else if (focused == chat_entry)
	{
		int start = 0, end = 0;
		if (chat_entry->get_selection_bounds(start, end))
			clipboard->set_text(chat_entry->get_chars(start, end));
	}
}


void main_win::on_paste_activate()
{
	Glib::RefPtr<Gdk::Clipboard> clipboard = get_display()->get_clipboard();
	// GTK4 reads clipboard text asynchronously.
	clipboard->read_text_async(
		[this, clipboard](Glib::RefPtr<Gio::AsyncResult>& result)
		{
			Glib::ustring text = clipboard->read_text_finish(result);
			if (!text.empty())
			{
				int pos = chat_entry->get_position();
				chat_entry->insert_text(text, text.size(), pos);
				chat_entry->set_position(pos);
				chat_entry->grab_focus();
			}
		});
}


void main_win::on_delete_activate()
{
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->erase_selection();
	else if (focused == chat_entry)
		chat_entry->delete_selection();
}


// Closing the window hides it and tears the game down cleanly; returning
// true blocks GTK's default destroy.
bool main_win::on_close_request()
{
	on_quit_activate();

	return true;
}


void main_win::on_quit_activate()
{
	if (!_server)
		quit_now();
	else
		confirm_then(sigc::mem_fun(*this, &main_win::quit_now));
}


void main_win::quit_now()
{
	hide();
	// Gtk::Main::run() used to return when the window was hidden;
	// Gtk::Application::run() keeps running, so end it explicitly.
	if (auto app = get_application())
		app->quit();
}


// Runs action right away when no confirmation is needed.  Otherwise the
// confirmation alert is shown asynchronously (no nested main-context
// iteration) and the action runs from its response handler if the user
// answers Yes.  The dialog is kept around so it can simply be re-presented
// if another confirmation is requested while it is up.
void main_win::confirm_then(sigc::slot<void()> action)
{
	if (_game_status == GameServer::Start || _game_status == GameServer::End)
	{
		action();
		return;
	}

	if (_move_count == 1 && (_current_player == 1 || _current_player == 0))
	{
		action();
		return;
	}

	_pending_confirm.push_back(action);

	if (!_confirm_quit_dialog)
	{
		_confirm_quit_dialog = new Gtk::MessageDialog(
			"This will end your current game!  Is that okay?",
			false, Gtk::MessageType::WARNING, Gtk::ButtonsType::YES_NO, true);
		_confirm_quit_dialog->set_transient_for(*this);
		_confirm_quit_dialog->signal_response().connect(sigc::mem_fun(*this,
			&main_win::on_confirm_quit_response));
	}

	_confirm_quit_dialog->show();
}


void main_win::on_confirm_quit_response(int response)
{
	_confirm_quit_dialog->hide();

	if (response == (int)Gtk::ResponseType::YES && !_pending_confirm.empty())
	{
		sigc::slot<void()> action = _pending_confirm.front();
		_pending_confirm.pop_front();
		action();
	}
	else
		_pending_confirm.clear();

	// A request that arrived while the dialog was up is answered next.
	if (!_pending_confirm.empty())
		_confirm_quit_dialog->show();
}


void main_win::on_about_activate()
{
	present_and_focus(&_about_win);
}


void main_win::on_how_to_play_activate()
{
	present_and_focus(&_help_win);
}


void main_win::on_chat_entry_activate()
{
	if (_client)
	{
		if (chat_entry->get_text().size() > 0)
		{
			_client->chat(chat_entry->get_text());
			chat_entry->set_text("");
		}
	}
}


void main_win::append_to_chat_entry(unsigned int keyval)
{
	if (keyval < 0x8000)
	{
		chat_entry->set_text(chat_entry->get_text() + keyval);
		chat_entry->grab_focus();
		chat_entry->set_position(-1);
	}
}


void main_win::show_message(Glib::ustring message)
{
	Gtk::TextIter iter;

	iter = message_view->get_buffer()->end();
	message_view->get_buffer()->insert(iter, message + "\n");

	iter = message_view->get_buffer()->end();
	message_view->scroll_to(iter);
}


void main_win::set_player_label(unsigned int posn, bool bold)
{
	if (!_client || !_client->get_board() || posn < 1 || posn > 6)
		return;

	Glib::ustring num_str = "";
	if (_client->get_board()->player_finished(posn) &&
		_finished_in_moves[posn-1])
	{
		num_str = util::to_str(_finished_in_moves[posn-1]);
		num_str = " (" + num_str + ")";
	}

	if (bold)
	{
		_player_name[posn-1]->set_markup("<b>"
			+ _client->get_player_name(posn) + num_str + "</b>");
	}
	else
	{
		_player_name[posn-1]->set_text(
			_client->get_player_name(posn) + num_str);
	}
}


// Moves the bold "current turn" label to `posn`, un-bolding the one that
// had it before (tracked separately, since the label can lag the server's
// turn while a move replay runs).
void main_win::set_turn_label(unsigned int posn)
{
	if (_label_player && _label_player != posn)
		set_player_label(_label_player, false);
	if (posn)
		set_player_label(posn, true);
	_label_player = posn;
}


// A move replay has started (fresh or chained onto a finished one): show
// the mover until it ends.
void main_win::on_anim_started()
{
	set_turn_label(game_view->anim_player());
}


// The board is idle again: catch the label up to the server's turn.
void main_win::on_anim_idle()
{
	set_turn_label(_current_player);
}


void main_win::on_cmd_player_add(unsigned int posn,
								 Glib::ustring name, int color)
{
	if (posn < 1 || posn > 6)
		return;

	// A rejoin reuses the slot's color; queued replays of the previous
	// occupant's moves would animate in the wrong color.  Snap instead.
	game_view->rebuild_board();

	if (posn == _current_player && game_view->anim_player())
	{
		// The turn label lags behind while a replay runs; don't steal it.
		set_player_label(posn, false);
	}
	else
		set_player_label(posn, _current_player == posn);

	_player_peg[posn-1]->set_color(color);
	game_view->queue_draw();

	update_menus();

	_setup_bot_win.refresh_focus();
}


void main_win::on_cmd_player_remove(unsigned int posn)
{
	if (posn < 1 || posn > 6)
		return;

	// A departing player's color is gone; replays of their moves would
	// animate as colorless ghosts.  Snap the view board to the truth.
	game_view->rebuild_board();

	_player_name[posn-1]->set_text("");
	_player_peg[posn-1]->clear();

	game_view->queue_draw();

	update_menus();

	_setup_bot_win.refresh_focus();
}


void main_win::on_cmd_player_finish(unsigned int posn, unsigned int move_count)
{
	if (posn < 1 || posn > 6)
		return;

	_finished_in_moves[posn-1] = move_count;
	set_player_label(posn, false);
}


void main_win::on_cmd_game_resync()
{
	game_view->rebuild_board();

	if (!_seats.empty())
	{
		// Resync arrives on every seat client.  The board keeps the first
		// human seat at the bottom for the whole game; only the input client
		// changes with the turn.
		HostSeat *input_seat = active_seat();
		GameClient *input = input_seat ? input_seat->client : _client;
		if (_active_input_client != input)
		{
			_active_input_client = input;
			game_view->set_client(input);
			game_view->rebuild_board();
		}

		HostSeat *view_seat = NULL;
		for (size_t i = 0; i < _seats.size(); i++)
			if (_seats[i].kind == HostSeat::SEAT_HUMAN &&
				_seats[i].player_number)
			{
				view_seat = &_seats[i];
				break;
			}
		if (view_seat)
			game_view->rotate_to_local_player(view_seat->player_number);

		game_view->set_locked(input_seat == NULL);
	}
	else
	{
		if (_client->get_my_player_number() != 0)
			game_view->rotate_to_local_player(_client->get_my_player_number());

		game_view->set_locked(_current_player != _client->get_my_player_number());
	}

	game_view->queue_draw();

	set_turn_label(_current_player);
}


void main_win::on_cmd_choose_new_name(Glib::ustring name)
{
	_name_win.set_name(&name, true);
	present_and_focus(&_name_win);
}


void main_win::on_cmd_choose_new_color(Glib::ustring name, int color)
{
	_color_win.set_force_info(&name, color);
	present_and_focus(&_color_win);
}


void main_win::on_cmd_game_turn(unsigned int posn,
								GameServer::GameStatus status,
								unsigned int move_count)
{
	// While a move replay runs, the label keeps showing the player who
	// made that move (like the iOS app); it catches up when the replay
	// and any queued replays end.
	if (!game_view->anim_player())
		set_turn_label(posn);

	_current_player = posn;
	_game_status = status;
	_move_count = move_count;

	if (posn == 1 && move_count == 1)
		for (int i=0; i<6; i++)
			_finished_in_moves[i] = 0;

	move_counter->set_markup("<b>" + util::to_str(_move_count) + "</b>");

	game_view->hide_move();
	if (!_seats.empty())
	{
		// Hot-seat: the seat whose turn it is drives the board.
		route_input();
	}
	else if (posn > 0 && posn == _client->get_my_player_number())
	{
		game_view->set_locked(false);
	}
	else
	{
		game_view->set_locked(true);
	}
	game_view->queue_draw();
	update_menus();
}


void main_win::on_cmd_game_undo_move(unsigned int from, unsigned int to)
{
	// The client board has already been reverted: re-copy it into the
	// view (dropping any queued replays, which would otherwise re-apply
	// the undone move).
	game_view->rebuild_board();
	game_view->queue_draw();
}


void main_win::on_cmd_game_show_move(MoveList *list)
{
	game_view->show_move(list);
	game_view->queue_draw();
}


void main_win::on_cmd_game_hide_move()
{
	game_view->hide_move();
	game_view->queue_draw();
}


void main_win::on_cmd_game_make_move(MoveList *list, GameClient *from)
{
	_last_move = *list;

	// Replay other players' moves as an animation so the next player (a
	// human selecting pegs or a bot searching) can plan while it plays;
	// the view board applies each move only when its replay lands.  The
	// local player's own move snaps into place, like the iOS app.
	if (from && from == game_view->get_client() && from->get_board() &&
		list->size() > 1)
	{
		unsigned int mover =
			(*from->get_board())[list->back()]->get_current_player();

		if (mover && mover == from->get_my_player_number())
		{
			// The local player committed while a replay was still running:
			// snap everything to its final position and release the
			// animation gate so the next bot is not held back by a window
			// that no longer exists.
			game_view->apply_move_now(*list);
			cheech::open_move_gate();
		}
		else if (mover)
		{
			// Drop the rest delay when this move hands the turn to a
			// human: the peg lands and they can play at once (like the
			// iOS app).
			unsigned int next =
				from->get_board()->get_next_player(mover);
			bool human_next = false;
			if (!_seats.empty())
			{
				for (size_t i = 0; i < _seats.size() && !human_next; i++)
					if (_seats[i].kind == HostSeat::SEAT_HUMAN &&
						_seats[i].player_number == next)
							human_next = true;
			}
			else
				human_next = (next != 0 &&
					next == _client->get_my_player_number());

			if (_anim_step_ms > 0 &&
				game_view->shown_move() != *list)
			{
				game_view->replay_move(*list, _anim_step_ms,
					human_next ? 0 : 2 * _anim_step_ms);
			}
			else
			{
				// The mover telegraphed this exact path, so we already
				// watched it being planned: snap instead of replaying it,
				// and release the gate like a local commit.
				game_view->apply_move_now(*list);
				cheech::open_move_gate();
			}
		}
		else
		{
			// The mover's peg has already moved on: just catch the view
			// board up.
			game_view->apply_move_now(*list);
		}
	}

	game_view->queue_draw();
}
