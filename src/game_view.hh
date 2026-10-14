/*
 *  The GUI Chinese Checkers Board.
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

#ifndef _GAME_VIEW_HH
#  include <gtkmm/drawingarea.h>
#  define _GAME_VIEW_HH

#include <gtkmm/drawingarea.h>
#include <gtkmm/stylecontext.h>
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
		void set_locked(bool l);
		void rotate_to_local_player(unsigned int posn);
		// Rotates without needing a joined client's board: the caller
		// supplies the player count (used when hosting, before any seat
		// joins).
		void rotate_to_seat(unsigned int num_players, unsigned int posn);
		void show_move(MoveList *list);
		void hide_move();

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

		std::vector<GameViewHole*>  _holes;
		MoveList _move_list;

		GameClient *_client;
		GameBoard *_board;
		bool _locked;
		GamePoint _center;
		GamePoint _board_center;
		double _scale;
		int _base_width;
		int _base_height;
};

#endif // _GAME_VIEW_HH
