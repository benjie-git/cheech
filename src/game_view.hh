/*
 *   The GUI Chinese Checkers Board.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _GAME_VIEW_HH
#  include <gtkmm/drawingarea.h>
#  define _GAME_VIEW_HH

#include <gtkmm/drawingarea.h>
#include <gtkmm/stylecontext.h>
#include <gdkmm/frameclock.h>
#include <gdkmm/pixbuf.h>
#include <cairomm/context.h>
#include <vector>

#include "game_view_hole.hh"
#include "game_board.hh"


class game_view : public Gtk::DrawingArea
{
	public:
		game_view();
		~game_view();

		sigc::signal<void(unsigned int)> evt_unhandled_key;
		sigc::signal<void()> evt_user_action;  // e.g. For turning off urgency
		sigc::signal<void()> evt_board_resized;

		void set_client(GameClient *c);
		void rebuild_board();
		// Switches the input client without replacing the board (hot-seat
		// turn handoff): every seat client holds the same board, so the
		// lagged view board and any running or parked replay stay valid.
		void set_input_client(GameClient *c);
		void set_locked(bool l);
		void rotate_to_local_player(unsigned int posn);
		// Rotates without needing a joined client's board: the caller
		// supplies the player count (used when hosting, before any seat
		// joins).
		void rotate_to_seat(unsigned int num_players, unsigned int posn);
		void show_move(MoveList *list);
		void hide_move();
		// The provisional path currently displayed (the local selection or
		// another player's telegraph); empty when nothing is shown.
		const MoveList &shown_move() const { return _move_list; }

		// Replays a move the server has already applied.  The view keeps
		// its own lagged copy of the board: each move is applied to it only
		// when its replay finishes, so a replay always plays over the exact
		// state the move was made in.  Like the iOS app, a replay waits for
		// the running one to finish, and at most one move waits: a newer
		// arrival replaces it (the superseded move is applied silently,
		// since it will never be animated).
		void replay_move(const MoveList &list, int step_ms, int done_ms);

		// Fast-forwards the view board to the true board, snapping any
		// running or queued replays (resync/undo/leave, and the local
		// player committing mid-replay).
		void flush_replays();

		// flush_replays() plus the move itself, applied at once: the local
		// player's own move never animates.
		void apply_move_now(const MoveList &list);
		GameClient *get_client() const { return _client; }

		// 0 while no replay is running, else the mover whose move is being
		// replayed.
		unsigned int anim_player() const
			{ return _anim_tick_id ? _anim_player : 0; }

		// A replay started (also fired when a queued replay chains onto a
		// finished one) / the board is idle again after the last replay.
		sigc::signal<void()> sig_anim_started;
		sigc::signal<void()> sig_anim_idle;

		GamePoint get_view_center() const { return _center; }
		double get_scale() const { return _scale; }
		int get_base_width() const { return _base_width; }
		int get_base_height() const { return _base_height; }

	protected:
		void create_holes();
		void destroy_holes();

		void on_realize();

		//Input handlers (wired to controllers in the constructor):
		void on_view_clicked(unsigned int clicks, double x, double y);
		bool on_view_key_pressed(guint keyval, guint keycode,
			Gdk::ModifierType state);
		void on_resize(int width, int height);

		void read_move(unsigned int i);
		void write_move();
		void erase_move();

		bool move_list_contains(unsigned int i);
		void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width,
			int height);
		void draw_move_arc(const Cairo::RefPtr<Cairo::Context>& cr,
						   GamePoint from, GamePoint to);

		bool on_anim_tick(const Glib::RefPtr<Gdk::FrameClock>& clock);
		bool start_replay(const MoveList &list, int step_ms, int done_ms);
		bool stop_anim();
		void finish_anim();
		GamePoint anim_point(double progress);

		std::vector<GameViewHole*>  _holes;
		MoveList _move_list;

		GameClient *_client;
		GameBoard *_board;        // the client's live board (input checks)
		GameBoard *_view_board;   // the view's lagged copy (what is drawn)
		bool _locked;
		GamePoint _center;
		GamePoint _board_center;
		double _scale;
		int _base_width;
		int _base_height;

		MoveList _anim_path;      // move currently being replayed

		// The next move waiting for its replay, if any.  One slot, not a
		// queue, like the iOS app: replays play one after another, and a
		// move arriving while one already waits replaces it.
		struct PendingReplay
		{
			MoveList list;
			int step_ms;
			int done_ms;
		};
		bool _anim_pending_valid;
		PendingReplay _anim_pending;
		int _anim_step_ms;        // milliseconds per hop
		int _anim_done_ms;        // rest at the destination, milliseconds
		gint64 _anim_start_us;    // monotonic start time, microseconds
		unsigned int _anim_player;  // the mover (destination peg owner)
		double _anim_x, _anim_y;  // in-flight peg position, board space
		guint _anim_tick_id;
};

#endif // _GAME_VIEW_HH
