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

#include <cmath>

#include "game_view_hole.hh"
#include "game_images.hh"


GameViewHole::GameViewHole(Gdk::Point *view_center, Gdk::Point *orig_offset)
	:_hole(NULL),
	 _center(view_center),
	 _orig_offset(*orig_offset),
	 _offset(*orig_offset),
	 _hilighted(false)
{
}


GameViewHole::~GameViewHole()
{
	
}


void GameViewHole::setup(GameClient *cl, GameHole *h)
{
	_client = cl;
	_hole = h;
}


Gdk::Point GameViewHole::get_location()
{
	return Gdk::Point(_center->get_x() + _offset.get_x(),
					  _center->get_y() + _offset.get_y());
}


bool GameViewHole::get_hilighted()
{
	return _hilighted;
}


void GameViewHole::set_hilighted(bool h)
{
	_hilighted = h;
}


void GameViewHole::set_offset(Gdk::Point p)
{
	_offset = p;
}


void GameViewHole::draw(Glib::RefPtr<Gdk::Window> window,
						  Glib::RefPtr<Gdk::GC> gc)
{
	int off_x, off_y;

	off_x = GameImages::get_peg_size().get_x()/2;
	off_y = GameImages::get_peg_size().get_y()/2;

	Glib::RefPtr<Gdk::Pixbuf> peg = GameImages::get_peg(!_hole ? 0 :
		_client->get_player_color(_hole->get_current_player()));
	
	if (peg) {
		peg->render_to_drawable(window, gc, 0, 0,
								get_location().get_x() - off_x,
								get_location().get_y() - off_y,
								-1, -1, Gdk::RGB_DITHER_NONE, 0, 0);
	}
	
	if (_hilighted)
	{
		off_x = GameImages::get_highlight_size().get_x()/2;
		off_y = GameImages::get_highlight_size().get_y()/2;
		Glib::RefPtr<Gdk::Pixbuf> hl = GameImages::get_highlight();
		if (hl) {
			hl->render_to_drawable(window, gc, 0, 0,
								   get_location().get_x() - off_x,
								   get_location().get_y() - off_y,
								   -1, -1, Gdk::RGB_DITHER_NONE, 0, 0);
		}
	}
}


bool GameViewHole::in_bounds(int x, int y)
{
	return (
		x >= get_location().get_x() - GameImages::get_peg_size().get_x()/2 &&
		x <= get_location().get_x() + GameImages::get_peg_size().get_x()/2 &&
		y >= get_location().get_y() - GameImages::get_peg_size().get_y()/2 &&
		y <= get_location().get_y() + GameImages::get_peg_size().get_y()/2);
}


void GameViewHole::rotate(double theta)
{
	double new_x = _orig_offset.get_x() * cos (theta) 
		- _orig_offset.get_y() * sin (theta);
	double new_y = _orig_offset.get_x() * sin (theta) 
		+ _orig_offset.get_y() * cos (theta);

	_offset = Gdk::Point((int)(new_x+0.5), (int)(new_y+0.5));
}
