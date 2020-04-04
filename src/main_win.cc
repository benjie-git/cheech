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

#include <gtkmm/textview.h>
#include <gtkmm/messagedialog.h> 

#include "main_win.hh"

#include "game_images.hh"
#include "utility.hh"


main_win::main_win() : main_win_glade()
{
	_server = NULL;
	_ajax_server = NULL;
	_client = NULL;
	_current_player = 0;
	_move_count = 1;
	_game_status = GameServer::End;

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

	logo->set(GameImages::get_logo(false));
	logo->set_size_request(GameImages::get_logo(false)->get_width(),
						   GameImages::get_logo(false)->get_height());

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
	unsigned int num_players, bool long_jumps,
	bool hop_others, bool stop_others)
{
	if (_server)
		on_end_game_activate();

	if (_server)
		return false;

	_server = new GameServer(port, num_players, long_jumps,
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

	update_menus();

	return true;
}


void main_win::start_client(Glib::ustring host, unsigned int port,
	Glib::ustring name, unsigned int color, bool spectator)
{
	if (_client)
		on_leave_game_activate();

	_client = new GameClient();
	_client->change_name(name);
	_client->change_color((spectator) ? 0 : color);

	_client->evt_connected.connect(sigc::bind(sigc::mem_fun(*this,
		&main_win::add_join_hostname), host));
	_client->evt_disconnected.connect(sigc::mem_fun(*this,
		&main_win::on_leave_game_activate));
	_client->evt_cancelled.connect(sigc::mem_fun(*this,
		&main_win::on_leave_game_activate));
	_client->evt_message.connect(sigc::mem_fun(*this,
		&main_win::show_message));
	_client->cmd_player_add.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_add));
	_client->cmd_player_remove.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_remove));
	_client->cmd_player_finish.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_player_finish));
	_client->cmd_game_resync.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_resync));
	_client->cmd_choose_new_name.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_choose_new_name));
	_client->cmd_choose_new_color.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_choose_new_color));
	_client->cmd_game_turn.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_turn));
	_client->cmd_game_undo_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_undo_move));
	_client->cmd_game_show_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_show_move));
	_client->cmd_game_hide_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_hide_move));
	_client->cmd_game_make_move.connect(sigc::mem_fun(*this,
		&main_win::on_cmd_game_make_move));

	game_view->set_client(_client);

	_client->join_game(host, port, spectator);

	update_menus();

	_client->evt_disconnected.connect(sigc::mem_fun(_color_win,
		&color_win::on_disconnected));
	_color_win.signal_change_color.connect(sigc::mem_fun(*_client,
		&GameClient::change_color));

	_client->evt_disconnected.connect(sigc::mem_fun(_name_win,
		&name_win::on_disconnected));
	_name_win.signal_change_name.connect(sigc::mem_fun(*_client,
		&GameClient::change_name));

	_client->cmd_game_turn.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_cmd_game_turn));
	_client->evt_disconnected.connect(sigc::mem_fun(_setup_game_win,
		&setup_game_win::on_disconnected));
	_setup_game_win.signal_restart_game.connect(sigc::mem_fun(this,
		&main_win::on_game_setup));
	_setup_game_win.setup(1, false, false, false, _game_status);

	_setup_bot_win.setup(_game_status);
	_client->cmd_game_turn.connect(sigc::mem_fun(_setup_bot_win,
		&setup_bot_win::on_cmd_game_turn));
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
	_new_game_win.show_join_page(false);
	_new_game_win.present();
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


void main_win::on_join_game_activate()
{
	_new_game_win.present();
	_new_game_win.show_join_page(true);
	game_view->set_locked(true);
}


void main_win::on_leave_game_activate()
{
	if (_client)
	{
		rule_label->set_label("");
		game_view->hide_move();
		game_view->queue_draw();
		game_view->set_client(NULL);
		game_view->rebuild_board();
		game_view->set_locked(true);
		on_remove_computer_players_activate();
		_client->leave_game();
		_current_player = 0;
		_game_status = GameServer::End;
		_setup_bot_win.setup(_game_status);
		_move_count = 1;
		move_counter->set_text("");
		_last_move.clear();

		// delete _client;
		_client = NULL;

		update_menus();
	}
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
		join_game->set_sensitive(false);
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
		join_game->set_sensitive(true);
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
		_color_win.present();
	}
}


void main_win::on_change_name_activate()
{
	if (_client)
	{
		Glib::ustring name = _client->get_my_name();
		_name_win.set_name(&name);
		_name_win.present();

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

		BotBase *bot = BotBase::new_bot_of_type(prefs.bot_type);
		if (!bot)
		{
			prefs.bot_type = "LookAhead(3)";
			prefs.write();
			bot = BotBase::new_bot_of_type(prefs.bot_type);
		}
		bot->set_move_delay(prefs.move_delay, prefs.done_delay);
		bot->set_think_delay(prefs.think_delay);

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
		_setup_game_win.present();
	}
}


void main_win::on_setup_computer_player_activate()
{
	_setup_bot_win.present();
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
	_about_win.present();
}


void main_win::on_how_to_play_activate()
{
	_help_win.present();
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

	_player_peg[posn-1]->set(GameImages::get_peg(color));
	_player_peg[posn-1]->set_size_request(
		GameImages::get_peg(color)->get_width(),
		GameImages::get_peg(color)->get_height());

	game_view->queue_draw();

	update_menus();
}


void main_win::on_cmd_player_remove(unsigned int posn)
{
	_player_name[posn-1]->set_text("");
	_player_peg[posn-1]->clear();
	_player_peg[posn-1]->queue_draw();

	game_view->queue_draw();

	update_menus();
}


void main_win::on_cmd_player_finish(unsigned int posn, unsigned int move_count)
{
	_finished_in_moves[posn-1] = move_count;
	set_player_label(posn, false);
}


void main_win::on_cmd_game_resync()
{
	game_view->rebuild_board();

	if (_client->get_my_player_number() != 0)
		game_view->rotate_to_local_player(_client->get_my_player_number());

	game_view->queue_draw();

	rule_label->set_markup("<b>" +
		util::to_str<unsigned int>(_client->get_board()->get_num_players()) +
		" Players</b>" +
		((_client->get_board()->get_long_jumps_allowed())
			? "\nLong Jumps" : "\nNo Long Jumps") +
		((!_client->get_board()->get_hop_others_allowed())
			? "\nNo Throughs" : "") +
		((!_client->get_board()->get_stop_others_allowed())
			? "\nNo Stops" : ""));

	if (_current_player)
	{
		set_player_label(_current_player, true);
		game_view->set_locked(_current_player != _client->get_my_player_number());
	}
}


void main_win::on_cmd_choose_new_name(Glib::ustring name)
{
	_name_win.set_name(&name, true);
	_name_win.present();
}


void main_win::on_cmd_choose_new_color(Glib::ustring name, int color)
{
	_color_win.set_force_info(&name, color);
	_color_win.present();
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
	if (posn > 0 && posn == _client->get_my_player_number())
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
