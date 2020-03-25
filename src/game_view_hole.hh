/*
 *  A hole in the GUI Chinese Checkers Board.
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

#ifndef _GAME_VIEW_HOLE_HH
#define _GAME_VIEW_HOLE_HH

#include <gtkmm/drawingarea.h>
#include <gdkmm/colormap.h>
#include <gdkmm/window.h>
#include <gdkmm/pixbuf.h>

#include "game_hole.hh"
#include "game_client.hh"


class GameViewHole : public sigc::trackable
{
	public:
		GameViewHole(Gdk::Point *view_center, Gdk::Point *orig_offset);
		~GameViewHole();

		void setup(GameClient *cl, GameHole *h);
		Gdk::Point get_location();
		bool get_hilighted();
		void set_offset(Gdk::Point p);
		void set_hilighted(bool h);
		void recenter(Gdk::Point view_center);
		void draw(Glib::RefPtr<Gdk::Window> window, Glib::RefPtr<Gdk::GC> gc);
		bool in_bounds(int x, int y);
		void rotate(double theta);

	private:
		GameClient *_client;
		GameHole *_hole;
		Gdk::Point *_center; // Center of game_view
		Gdk::Point _orig_offset;  // original location with middle peg as 0,0
		Gdk::Point _offset;  // location with middle peg as 0,0
		bool _hilighted;
};

#endif // _GAME_VIEW_HOLE_HH
