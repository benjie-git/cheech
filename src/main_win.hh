/*
 *   The main Chinese Checkers Game Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _MAIN_WIN_HH
#  include "main_win_glade.hh"
#  define _MAIN_WIN_HH

#include "about_win.hh"
#include "help_win.hh"
#include "new_game_win.hh"
#include "name_win.hh"
#include "setup_bot_win.hh"
#include "setup_game_win.hh"
#include "color_win.hh"

#include "game_server.hh"
#include "ajax_server.hh"
#include "game_client.hh"
#include "bot_base.hh"

#include <giomm/simpleactiongroup.h>
#include <giomm/menuitem.h>
#include <gtkmm/messagedialog.h>

#include <deque>


class main_win : public main_win_glade
{
	public:
		main_win();
		~main_win();

		bool start_server(unsigned int port, unsigned int cheechweb_port,
			bool long_jumps, bool hop_others, bool stop_others,
			std::vector<NewGameSeat> seats);
		bool do_start_server(unsigned int port, unsigned int cheechweb_port,
			bool long_jumps, bool hop_others, bool stop_others,
			std::vector<NewGameSeat> seats);
		void start_client(Glib::ustring host, unsigned int port,
			Glib::ustring name, unsigned int color, bool spectator);

	private:
		// One seat of a hosted hot-seat game: a local human (its own client),
		// a computer (a bot), or a remote seat left open for a joiner.
		struct HostSeat
		{
			enum { SEAT_HUMAN, SEAT_COMPUTER, SEAT_REMOTE };

			int kind;
			Glib::ustring name;
			unsigned int color;
			Glib::ustring family;
			unsigned int level;
			unsigned int depth;
			bool focus_all;
			std::vector<unsigned int> focus_ids;	// seat indices
			GameClient	*client;
			BotBase		*bot;
			unsigned int player_number;
		};

		void connect_client(GameClient *client, bool primary);
		void connect_client_dialogs(GameClient *client);
		void start_hosted_seats(std::vector<NewGameSeat> seats,
			unsigned int port);
		void join_next_seat();
		bool poll_seat_join();
		void apply_seat_focus();
		void hook_seat_client(GameClient *client, size_t index);
		void on_seat_choose_new_name(Glib::ustring name,
			GameClient *client, size_t index);
		void on_seat_choose_new_color(Glib::ustring name, int color,
			GameClient *client);
		void on_seat_player_number(unsigned int n, GameClient *client);
		HostSeat *active_seat();
		void route_input();
		void clear_seats();

		void on_realize();
		void on_new_game_activate();
		void on_restart_game_activate();
		void on_rotate_activate();
		void on_shuffle_activate();
		void on_game_setup(unsigned int num_players, bool long_jumps,
						   bool hop_others, bool stop_others);
		void on_dialog_change_color(int color);
		void on_dialog_change_name(Glib::ustring name);
		void on_end_game_activate();
		void on_leave_game_activate();
        void on_change_name_activate();
		void on_change_color_activate();
		void on_resync_activate();
		bool on_close_request();
		void on_quit_activate();
		void confirm_then(sigc::slot<void()> action);
		void on_confirm_quit_response(int response);
		void quit_now();
		void on_dialog_hidden();
		void on_game_settings_activate();

		void on_add_computer_player_activate();
        void on_setup_computer_player_activate();
		void on_remove_computer_players_activate();
		void on_show_chat_activate();

		void on_undo_activate();
		void on_redo_activate();
		void on_cut_activate();
		void on_copy_activate();
		void on_paste_activate();
		void on_delete_activate();
		void on_show_last_move_activate();

		void add_join_hostname(Glib::ustring host);
		void stop_server();
		void add_bot(BotBase *bot);
		void bot_disconnected(BotBase *bot);
		void update_menus();

		void on_about_activate();
		void on_how_to_play_activate();

		void on_chat_entry_activate();
		void append_to_chat_entry(unsigned int keyval);

		void show_message(Glib::ustring message);
		void on_chat_vadjustment_changed();
		void set_player_label(unsigned int posn, bool bold);
		// Moves the bold "current turn" label to `posn`, un-bolding the
		// previously bolded one; keeps track across lagged turn display.
		void set_turn_label(unsigned int posn);
		void on_anim_started();
		void on_anim_idle();

		void on_cmd_player_add(unsigned int posn,
			Glib::ustring name, int color);
		void on_cmd_player_remove(unsigned int posn);
		void on_cmd_player_finish(unsigned int posn, unsigned int move_count);
		void on_cmd_game_resync();
		void on_cmd_choose_new_name(Glib::ustring name);
		void on_cmd_choose_new_color(Glib::ustring name, int color);
		void on_cmd_game_turn(unsigned int posn,
							  GameServer::GameStatus status,
							  unsigned int move_count);
		void on_cmd_game_undo_move(unsigned int from, unsigned int to);
		void on_cmd_game_show_move(MoveList *list);
		void on_cmd_game_hide_move();
		// `from` is the client whose signal fired (bound in
		// connect_client); only the client the board is viewed through
		// starts a replay, so hot-seat broadcasts are not triple-counted.
		void on_cmd_game_make_move(MoveList *list, GameClient *from);

		unsigned int	_current_player;
		unsigned int	_move_count;
		GameServer::GameStatus	_game_status;
		GameServer		*_server;
		AjaxServer		*_ajax_server;
		GameClient		*_client;
		Gtk::Label		*_player_name[6];
		PegColorSwatch	*_player_peg[6];
		MoveList		_last_move;
		unsigned int	_finished_in_moves[6];
		// Milliseconds per hop of the move replay, picked up in
		// connect_client (0 disables the replay).
		int				_anim_step_ms;
		// The player whose label is currently bolded; lags the server's
		// turn while a move replay runs (like the iOS app).
		unsigned int	_label_player;

		about_win		_about_win;
		help_win		_help_win;
		new_game_win	_new_game_win;
		name_win		_name_win;
		setup_bot_win	_setup_bot_win;
		setup_game_win	_setup_game_win;
		color_win		_color_win;

		std::vector<BotBase*>	_bots;

		std::vector<HostSeat>	_seats;
		size_t					_next_seat;
		unsigned int			_seat_join_ticks;
		unsigned int			_hosted_port;
		// Held so a second hosted game can cancel the previous poll timer
		// before it repopulates the seats.
		sigc::connection		_seat_poll;
		GameClient				*_active_input_client;
		// Set while on_leave_game_activate tears a game down: disconnecting
		// seat clients re-emits evt_disconnected into the same handler, and a
		// second entry would finish the teardown and leave the outer run
		// using freed state (the quit-midgame crash).
		bool					_leaving_game;

		// The win.* actions the Gio::Menu model in main_win_glade refers to.
		Glib::RefPtr<Gio::SimpleActionGroup>	_win_actions;

		Gtk::MessageDialog * _confirm_quit_dialog;
		// Set by show_message, consumed when the chat view's adjustment
		// reports its range grew (the new line has been laid out).
		bool _chat_scroll_pending;
		// The actions a Yes on the confirm dialog should perform, in the
		// order they were requested (a second request while the dialog is
		// up queues instead of replacing the first).
		std::deque<sigc::slot<void()>> _pending_confirm;
		// Held so update_menus() can enable/disable menu entries.
		Glib::RefPtr<Gio::SimpleAction> _end_game_action;
		Glib::RefPtr<Gio::SimpleAction> _leave_game_action;
		Glib::RefPtr<Gio::SimpleAction> _game_settings_action;
		Glib::RefPtr<Gio::SimpleAction> _restart_game_action;
		Glib::RefPtr<Gio::SimpleAction> _rotate_action;
		Glib::RefPtr<Gio::SimpleAction> _shuffle_action;
		Glib::RefPtr<Gio::SimpleAction> _change_name_action;
		Glib::RefPtr<Gio::SimpleAction> _change_color_action;
		Glib::RefPtr<Gio::SimpleAction> _add_computer_player_action;
		Glib::RefPtr<Gio::SimpleAction> _remove_computer_players_action;

		Glib::RefPtr<Gio::SimpleAction> _setup_computer_player_action;
		Glib::RefPtr<Gio::SimpleAction> _show_last_move_action;
		Glib::RefPtr<Gio::SimpleAction> _undo_action;
		Glib::RefPtr<Gio::SimpleAction> _redo_action;
		// The Show Chat item label is toggled at runtime.
		Glib::RefPtr<Gio::MenuItem>		_show_chat_item;
};

#endif
