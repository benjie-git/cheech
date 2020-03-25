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
#include <gdkmm/colormap.h>
#include <gdkmm/window.h>
#include <gdkmm/pixbuf.h>
#include <vector>

#include "game_view_hole.hh"
#include "game_board.hh"


class game_view : public Gtk::DrawingArea
{
	public:
		game_view();
		~game_view();

		sigc::signal<void, unsigned int> evt_unhandled_key;
		sigc::signal<void> evt_user_action;  // e.g. For turning off urgency

		void set_client(GameClient *c);
		void rebuild_board();
		void set_locked(bool l);
		void rotate_to_local_player(unsigned int posn);
		void show_move(MoveList *list);
		void hide_move();

	protected:
		void create_holes();
		void destroy_holes();

		//Override default signal handlers:
		void on_realize();
		bool on_expose_event(GdkEventExpose* event);
		bool on_configure_event(GdkEventConfigure* event);
		bool on_button_press_event(GdkEventButton *ev);
		bool on_key_press_event(GdkEventKey *ev);

		void read_move(unsigned int i);
		void write_move();
		void erase_move();

		bool move_list_contains(unsigned int i);
		void draw_move_arc(Glib::RefPtr<Gdk::Window> window,
						   Gdk::Point from, Gdk::Point to);

		std::vector<GameViewHole*>  _holes;
		MoveList _move_list;

		GameClient *_client;
		GameBoard *_board;
		Glib::RefPtr<Gdk::Window> _window;
		Glib::RefPtr<Gdk::GC> _gc;
		bool _locked;
		Gdk::Point _center;
};

#endif // _GAME_VIEW_HH
