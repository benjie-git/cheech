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
#include <gdkmm/window.h>

#include <map>

#ifdef MACOS_APP
#include <gtkosxapplication.h>
#endif

#include "main_win.hh"

#include "bot_catalog.hh"
#include "utility.hh"


// Gives the window keyboard focus on the next main-loop iteration.  Doing it
// immediately fails when the request comes from a menu popup that still holds
// the keyboard grab, so the window manager would hand focus back to the parent.
static void grab_window_focus(Gtk::Window *window)
{
	Glib::RefPtr<Gdk::Window> gdk_window = window->get_window();

	if (gdk_window)
		gdk_window->focus(GDK_CURRENT_TIME);

	if (!window->get_focus())
		window->child_focus(Gtk::DIR_TAB_FORWARD);
}


static void present_and_focus(Gtk::Window *window)
{
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
	game_view->evt_user_action.connect(sigc::bind(sigc::mem_fun(*this,
		&Gtk::Window::set_urgency_hint), false));

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

#ifdef MACOS_APP
	setup_macos_menubar();
#endif
}


#ifdef MACOS_APP
void main_win::setup_macos_menubar()
{
	GtkosxApplication *osx = gtkosx_application_get();

	// The menu bar is rendered by the system, so hide the in-window copy.
	menubar1->set_no_show_all(true);
	menubar1->hide();

	gtkosx_application_set_menu_bar(osx, GTK_MENU_SHELL(menubar1->gobj()));
	gtkosx_application_set_about_item(osx, GTK_WIDGET(about->gobj()));
	gtkosx_application_set_help_menu(osx, GTK_MENU_ITEM(help_menu->gobj()));
	gtkosx_application_set_use_quartz_accelerators(osx, TRUE);

	g_signal_connect(osx, "NSApplicationBlockTermination",
		G_CALLBACK(main_win::on_macos_block_termination), this);

	gtkosx_application_ready(osx);
}


gboolean main_win::on_macos_block_termination(gpointer, gpointer data)
{
	main_win *win = static_cast<main_win *>(data);

	if (win->confirm_end_game())
		gtk_main_quit();

	// Always block Cocoa's own termination and quit through the main loop
	// so that the game is torn down cleanly.
	return TRUE;
}
#endif


void main_win::on_dialog_hidden()
{
	if (get_visible())
		present();
}


main_win::~main_win()
{
	if (_server)
		stop_server();
	if (_client)
		on_leave_game_activate();
}


void main_win::on_realize()
{
	update_menus();
}


bool main_win::start_server(unsigned int port, unsigned int cheechweb_port,
	bool long_jumps, bool hop_others, bool stop_others,
	std::vector<NewGameSeat> seats)
{
	if (_server)
		on_end_game_activate();

	if (_server)
		return false;

	_server = new GameServer(port, seats.size(), long_jumps,
							 hop_others, stop_others);
	_server->evt_message.connect(sigc::mem_fun(*this, &main_win::show_message));
	_server->new_game();

	if (!_server->ready())
	{
		// delete _server;
		_server = NULL;
		update_menus();
		return false;
	}

	if (cheechweb_port)
	{
		_ajax_server = new AjaxServer();
		if (!_ajax_server->listen(cheechweb_port, "localhost", port))
		{
			//delete _ajax_server;
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
	client->cmd_game_make_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_make_move));
}


// Hooks the name/color/game-settings dialogs to a client.  Only used for the
// primary client; repeated connections match the existing start_client
// behavior.
void main_win::connect_client_dialogs(GameClient *client)
{
	client->evt_disconnected.connect(sigc::mem_fun(_color_win,
		&color_win::on_disconnected));
	_color_win.signal_change_color.connect(sigc::mem_fun(*client,
		&GameClient::change_color));

	client->evt_disconnected.connect(sigc::mem_fun(_name_win,
		&name_win::on_disconnected));
	_name_win.signal_change_name.connect(sigc::mem_fun(*client,
		&GameClient::change_name));

	client->cmd_game_turn.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_cmd_game_turn));
	client->evt_disconnected.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_disconnected));
	_setup_game_win.signal_restart_game.connect(sigc::mem_fun(this,
		&main_win::on_game_setup));
	_setup_game_win.setup(1, false, false, false, _game_status);

	_setup_bot_win.setup(_game_status, client);
	client->cmd_game_turn.connect(sigc::mem_fun(_setup_bot_win,
		&setup_bot_win::on_cmd_game_turn));
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
	Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&main_win::poll_seat_join), 250);
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
		// Match the Add Computer Player dialog's saved default (or its
		// built-in default when nothing has been saved yet).  Think delay
		// stays at 0.
		{
			Prefs prefs;
			prefs.read();
			seat.bot->set_move_delay(prefs.move_delay,
				2 * prefs.move_delay);
		}
		_bots.push_back(seat.bot);
		seat.bot->evt_disconnected.connect(sigc::bind(sigc::mem_fun(*this,
			&main_win::bot_disconnected), seat.bot));
		seat.bot->join_game("localhost", _hosted_port);
	}

	// SEAT_REMOTE: leave the seat open for a joiner.
}


// Waits for the seat that is currently joining to receive its player number
// (the server assigns numbers by connection-arrival order), then starts the
// next seat.  Gives up on a seat after 5 seconds so a stalled join cannot
// block the rest.
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

	if (++_seat_join_ticks >= 20)
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
		game_view->set_client(input);
		game_view->rebuild_board();
	}

	game_view->set_locked(seat == NULL);
}


void main_win::clear_seats()
{
	// Seat bots were already removed from _bots by
	// on_remove_computer_players_activate(); the clients stay allocated like
	// the primary client does.
	for (size_t i = 0; i < _seats.size(); i++)
		if (_seats[i].client && _seats[i].client != _client)
			_seats[i].client->leave_game();
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
				if (confirm_end_game())
					_bots[0]->get_game_client()->restart_game();
		}
		else
		{
			if (confirm_end_game())
				_client->restart_game();
		}
	}
}


void main_win::on_rotate_activate()
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				if (confirm_end_game())
					_bots[0]->get_game_client()->rotate_players();
		}
		else
		{
			if (confirm_end_game())
				_client->rotate_players();
		}
	}
}


void main_win::on_shuffle_activate()
{
	if (_client)
	{
		if (_client->is_spectator())
		{
			if (_bots.size() > 0 && _bots[0]->get_game_client())
				if (confirm_end_game())
					_bots[0]->get_game_client()->shuffle_players();
		}
		else
		{
			if (confirm_end_game())
				_client->shuffle_players();
		}
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
				if (_game_status != GameServer::Playing || confirm_end_game())
					_bots[0]->get_game_client()->reconfigure_game(
						num_players, long_jumps, hop_others, stop_others);
		}
		else
		{
			if (_game_status != GameServer::Playing || confirm_end_game())
				_client->reconfigure_game(
						num_players, long_jumps, hop_others, stop_others);

		}
	}
	_setup_game_win.hide();
}


void main_win::on_end_game_activate()
{
	if (_server && confirm_end_game())
		stop_server();
}


void main_win::stop_server()
{
	if (_ajax_server)
	{
		_ajax_server->close();
		//delete _ajax_server;
		_ajax_server = NULL;
	}

	if (_server)
	{
		on_leave_game_activate();

		util::delay_ms(100);
		_server->end_game();
		// delete _server;
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
		_setup_bot_win.setup(_game_status, _client);
		_move_count = 1;
		move_counter->set_text("");
		_last_move.clear();

		// delete _client;
		_client = NULL;

		update_menus();
	}

	_leaving_game = false;
}


void main_win::update_menus()
{
	if (_server)
	{
		end_game->set_sensitive(true);
	}
	else
	{
		end_game->set_sensitive(false);
	}

	if (_client)
	{
		leave_game->set_sensitive(true);

		bool active_member = (!_client->is_spectator()) || (_bots.size() > 0);
		game_settings->set_sensitive(active_member);
		restart_submenu->set_sensitive(active_member);
		restart_game->set_sensitive(active_member);
		rotate->set_sensitive(active_member);
		shuffle->set_sensitive(active_member);

		change_name->set_sensitive(true);
		change_color->set_sensitive(true);
		add_computer_player->set_sensitive(_game_status == GameServer::WaitingForPlayers);
		remove_computer_players->set_sensitive(_bots.size() > 0);
		show_last_move->set_sensitive(true);
		undo->set_sensitive(true);
		redo->set_sensitive(true);
	}
	else
	{
		leave_game->set_sensitive(false);

		game_settings->set_sensitive(false);
		restart_submenu->set_sensitive(false);
		restart_game->set_sensitive(false);
		rotate->set_sensitive(false);
		shuffle->set_sensitive(false);

		change_name->set_sensitive(false);
		change_color->set_sensitive(false);
		add_computer_player->set_sensitive(false);
		remove_computer_players->set_sensitive(false);
		show_last_move->set_sensitive(false);
		undo->set_sensitive(false);
		redo->set_sensitive(false);
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
		bot->set_move_delay(prefs.move_delay, 2 * prefs.move_delay);
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
		show_chat->set_label(_("Show _Chat"));
	}
	else
	{
		chat_area->show_all();
		show_chat->set_label(_("Hide _Chat"));
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
				_bots.erase(b);

	// delete bot;
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
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->cut_clipboard(Gtk::Clipboard::get());
	else if (focused == chat_entry)
		chat_entry->cut_clipboard();
}


void main_win::on_copy_activate()
{
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->copy_clipboard(Gtk::Clipboard::get());
	else if (focused == chat_entry)
		chat_entry->copy_clipboard();
}


void main_win::on_paste_activate()
{
	chat_entry->paste_clipboard();
	chat_entry->grab_focus();
}


void main_win::on_delete_activate()
{
	Gtk::Widget *focused = get_focus();
	if (focused == message_view)
		message_view->get_buffer()->erase_selection();
	else if (focused == chat_entry)
		chat_entry->delete_selection();
}


bool main_win::on_delete_event(GdkEventAny* event)
{
	on_quit_activate();

	return true;
}


void main_win::on_quit_activate()
{
	if (!_server || confirm_end_game())
		hide();
}


bool main_win::confirm_end_game()
{
	if (_game_status == GameServer::Start || _game_status == GameServer::End)
		return true;

	if (_move_count == 1 && (_current_player == 1 || _current_player == 0))
		return true;

	Gtk::MessageDialog dlg("This will end your current game!  Is that okay?",
		false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
	if (dlg.run() == Gtk::RESPONSE_YES)
		return true;

	return false;
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


void main_win::on_cmd_player_add(unsigned int posn,
								 Glib::ustring name, int color)
{
	set_player_label(posn, _current_player == posn);

	_player_peg[posn-1]->set_color(color);
	game_view->queue_draw();

	update_menus();

	_setup_bot_win.refresh_focus();
}


void main_win::on_cmd_player_remove(unsigned int posn)
{
	_player_name[posn-1]->set_text("");
	_player_peg[posn-1]->clear();

	game_view->queue_draw();

	update_menus();

	_setup_bot_win.refresh_focus();
}


void main_win::on_cmd_player_finish(unsigned int posn, unsigned int move_count)
{
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

	if (_current_player)
	{
		set_player_label(_current_player, true);
	}
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
	if (_current_player)
		set_player_label(_current_player, false);
	if (posn)
		set_player_label(posn, true);

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
		set_urgency_hint(active_seat() != NULL);
	}
	else if (posn > 0 && posn == _client->get_my_player_number())
	{
		game_view->set_locked(false);
		set_urgency_hint(true);
	}
	else
	{
		game_view->set_locked(true);
		set_urgency_hint(false);
	}
	game_view->queue_draw();
	update_menus();
}


void main_win::on_cmd_game_undo_move(unsigned int from, unsigned int to)
{
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


void main_win::on_cmd_game_make_move(MoveList *list)
{
		_last_move = *list;
}
