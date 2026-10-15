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
#include <gdkmm/pixbuf.h>
#include <cairomm/context.h>

#include "game_hole.hh"
#include "game_images.hh"
#include "game_client.hh"


class GameViewHole : public sigc::trackable
{
	public:
		GameViewHole(GamePoint *view_center, GamePoint *orig_offset);
		~GameViewHole();

		void setup(GameClient *cl, GameHole *h);
		GamePoint get_location();
		GamePoint get_offset();
		bool get_hilighted();
		void set_offset(GamePoint p);
		void set_scale(double s);
		void set_hilighted(bool h);
		// Draws the hole as empty while a replayed move is in flight to
		// this hole (the moving peg is drawn on top by game_view).
		void set_suppress_peg(bool s);
		void recenter(GamePoint view_center);
		void draw(const Cairo::RefPtr<Cairo::Context>& cr);
		static void draw_peg_at(const Cairo::RefPtr<Cairo::Context>& cr,
			double cx, double cy, unsigned int id);
		bool in_bounds(int x, int y);
		void rotate(double theta);

	private:
		GameClient *_client;
		GameHole *_hole;
		GamePoint *_center; // Center of game_view
		GamePoint _orig_offset;  // original location with middle peg as 0,0
		GamePoint _offset;  // location with middle peg as 0,0
		double _scale;  // board scale factor applied by game_view
		bool _hilighted;
		bool _suppress_peg;
};

#endif // _GAME_VIEW_HOLE_HH
