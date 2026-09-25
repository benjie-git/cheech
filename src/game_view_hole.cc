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
	:_client(NULL),
	 _hole(NULL),
	 _center(view_center),
	 _orig_offset(*orig_offset),
	 _offset(*orig_offset),
	 _scale(1.0),
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
	return Gdk::Point((int)lround(_center->get_x() + _offset.get_x() * _scale),
					  (int)lround(_center->get_y() + _offset.get_y() * _scale));
}


Gdk::Point GameViewHole::get_offset()
{
	return _offset;
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
	_offset = Gdk::Point(p.get_x(), p.get_y());
}


void GameViewHole::set_scale(double s)
{
	_scale = s;
}


void GameViewHole::draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	const double PI = 3.14159265359;

	double r = GameImages::get_peg_size().get_x() / 2.0;
	double cx = _offset.get_x();
	double cy = _offset.get_y();

	unsigned int id = (!_hole) ? 0 :
		_client->get_player_color(_hole->get_current_player());

	Gdk::RGBA fill = GameImages::get_peg_fill(id);
	Gdk::RGBA edge = GameImages::get_peg_edge(id);

	cr->set_source_rgba(edge.get_red(), edge.get_green(), edge.get_blue(),
						edge.get_alpha());
	cr->arc(cx, cy, r, 0, 2.0 * PI);
	cr->fill();

	cr->set_source_rgba(fill.get_red(), fill.get_green(), fill.get_blue(),
						fill.get_alpha());
	cr->arc(cx, cy, r * 0.88, 0, 2.0 * PI);
	cr->fill();

	if (_hole && _hole->get_current_player())
	{
		cr->set_source_rgba(1.0, 1.0, 1.0, 0.35);
		cr->arc(cx - r * 0.28, cy - r * 0.34, r * 0.30, 0, 2.0 * PI);
		cr->fill();
	}

	if (_hilighted)
	{
		cr->set_source_rgba(1.0, 0.9, 0.0, 1.0);
		cr->set_line_width(r * 0.28);
		cr->arc(cx, cy, r * 1.25, 0, 2.0 * PI);
		cr->stroke();
	}
}


bool GameViewHole::in_bounds(int x, int y)
{
	double half_x = GameImages::get_peg_size().get_x() / 2.0 * _scale;
	double half_y = GameImages::get_peg_size().get_y() / 2.0 * _scale;
	Gdk::Point location = get_location();

	return (
		x >= location.get_x() - half_x &&
		x <= location.get_x() + half_x &&
		y >= location.get_y() - half_y &&
		y <= location.get_y() + half_y);
}


void GameViewHole::rotate(double theta)
{
	double new_x = _orig_offset.get_x() * cos (theta) 
		- _orig_offset.get_y() * sin (theta);
	double new_y = _orig_offset.get_x() * sin (theta) 
		+ _orig_offset.get_y() * cos (theta);

	_offset = Gdk::Point((int)(new_x+0.5), (int)(new_y+0.5));
}
