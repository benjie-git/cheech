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

#include <gdk/gdkkeysyms.h>
#include <gtkmm/main.h>
#include <sstream>
#include <cmath>

#include "game_view.hh"
#include "game_images.hh"
#include "utility.hh"
#include "bot_lookahead.hh"

#define PI 3.14159265359
#define MULTIPLIER 1.5


game_view::~game_view()
{
	destroy_holes();
}


game_view::game_view() : Gtk::DrawingArea()
{
	_client = NULL;
	_board = NULL;
	_locked = true;
	_center = Gdk::Point();
	_board_center = Gdk::Point();
	_scale = 1.0;

	double peg_x = GameImages::get_peg_size().get_x();
	double peg_y = GameImages::get_peg_size().get_y();
	double min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9;

	GameBoard b(1, false, false, false);
	for (unsigned int i = 0; i < b.get_size(); i++)
	{
		if (!b[i])
			continue;

		double x_offset = (util::even(i / GameBoard::SIZE_X) ? 0.5 : 0.0) + 1.0;
		double rx = ((i % GameBoard::SIZE_X) + x_offset) * peg_x * MULTIPLIER;
		double ry = (i / GameBoard::SIZE_X) * peg_y * MULTIPLIER
					* (sqrt(3.0) / 2.0);

		if (rx < min_x) min_x = rx;
		if (rx > max_x) max_x = rx;
		if (ry < min_y) min_y = ry;
		if (ry > max_y) max_y = ry;
	}

	// Pad the board bounding box so the background hugs the lattice (keeping
	// it close to square) instead of filling the whole (tall) widget.
	double pad = peg_x * MULTIPLIER * 0.8;

	_board_center = Gdk::Point((int)lround((min_x + max_x) / 2.0),
							   (int)lround((min_y + max_y) / 2.0));
	_base_width = (int)lround(max_x - min_x + 2.0 * pad);
	_base_height = (int)lround(max_y - min_y + 2.0 * pad);

	set_size_request(_base_width, _base_height);

	grab_focus();
}


void game_view::on_realize()
{
	// We need to call the base on_realize()
	Gtk::DrawingArea::on_realize();

	create_holes();
}


bool game_view::on_configure_event(GdkEventConfigure* event)
{
	_center = Gdk::Point(event->width/2, event->height/2);

	double sx = (double)event->width / _base_width;
	double sy = (double)event->height / _base_height;
	_scale = (sx < sy) ? sx : sy;

	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->set_scale(_scale);

	queue_draw();

	evt_board_resized.emit();

	return true;
}


void game_view::create_holes()
{
	GameBoard b(1, false, false, false);

	_holes.resize(b.get_size());

	for (unsigned int i=0; i < b.get_size(); i++)
	{
		if (b[i])
		{
			double x = GameImages::get_peg_size().get_x();
			double y = GameImages::get_peg_size().get_y();
			double x_offset = (util::even(i / GameBoard::SIZE_X) ? 0.5 : 0.0)
								+ 1.0;
			double y_offset = 0;
			Gdk::Point offset((int)(((i % GameBoard::SIZE_X)
				+ x_offset) * x * MULTIPLIER)-_board_center.get_x(),
				(int)(((i / GameBoard::SIZE_X) + y_offset) * y * MULTIPLIER 
				* (sqrt(3.0) / 2.0))-_board_center.get_y());
			
			_holes[i] = new GameViewHole(&_center, &offset);
			_holes[i]->set_scale(_scale);
		}
		else
			_holes[i] = NULL;
	}

	rebuild_board();
}


void game_view::destroy_holes()
{
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			delete *hole;
	_holes.resize(0);
}


void game_view::set_client(GameClient *c)
{
	_client = c;
}


void game_view::rebuild_board()
{
	if (_client && _client->get_board())
	{
		_board = _client->get_board();
	
		for (unsigned int i=0; i < _board->get_size(); i++)
			if (_holes[i])
				_holes[i]->setup(_client, (*_board)[i]);
	}
	else
	{
		for (unsigned int i=0; i < _holes.size(); i++)
			if (_holes[i])
				_holes[i]->setup(NULL, NULL);

		_board = NULL;
	}
}


void game_view::set_locked(bool l)
{
	_locked = l;
}


bool game_view::on_button_press_event(GdkEventButton *ev)
{
	grab_focus();
	evt_user_action();

	if (_locked)
		return false;

	if (ev->button == 1)
	{
		for (unsigned int i = 0; i < _holes.size(); i++)
		{
			if (_holes[i] && _holes[i]->in_bounds((int)ev->x, (int)ev->y))
			{
				if (ev->type == GDK_BUTTON_PRESS)
				{
					read_move(i);
				}
				else if (ev->type == GDK_2BUTTON_PRESS)
				{
					if (_move_list.size() > 0 && _move_list.back() != i)
						read_move(i);
					if (_move_list.size() >= 2 && i == _move_list.back())
						write_move();
				}
				else
					return false;
			
				return true;
			}
		}
	}
	else
	{
		erase_move();
		return true;
	}

	return false;
}


bool game_view::on_key_press_event(GdkEventKey *ev)
{
	evt_user_action();

	switch (ev->keyval)
	{
		case GDK_KEY_Escape:
			if (!_locked)
			{
				erase_move();
				return true;
			}
		case GDK_KEY_Return:
		case GDK_KEY_space:
			if (!_locked)
			{
				write_move();
				return true;
			}
		default:
			if (ev->state == 0 || ev->state == GDK_SHIFT_MASK)
			{
				evt_unhandled_key(ev->keyval);
				return true;
			}
			break;
	}
	return false;
}


void game_view::read_move(unsigned int i)
{
	if (_locked)
		return;
	
	if (!move_list_contains(i))
	{
		// Clicking another of the player's pegs clears any path built so
		// far and starts a fresh selection from that peg.
		if (_move_list.size() > 0 &&
			(*_board)[i]->get_current_player() ==
			_client->get_my_player_number())
		{
			erase_move();
			_move_list.push_back(i);
			_client->show_move(&_move_list);
			return;
		}

		_move_list.push_back(i);
		if (!((_move_list.size() == 1 &&
			(*_board)[i]->get_current_player() ==
			_client->get_my_player_number()) ||
			_board->valid_move_list(_move_list, false)))
		{
			_move_list.pop_back();
		}
		else
		{
			_client->show_move(&_move_list);
		}
	} else {
		if (_move_list.back() == i)
		{
			_holes[_move_list.back()]->set_hilighted(false);
			_move_list.pop_back();
		}
		else
		{
			while (_move_list.size() > 0 && _move_list.back() != i)
			{
				_holes[_move_list.back()]->set_hilighted(false);
				_move_list.pop_back();
			}
		}
		if (_move_list.size() > 0 )
			_client->show_move(&_move_list);
		else
			_client->hide_move();
	}
}


void game_view::write_move()
{
	if (_locked)
		return;

	if (_move_list.size() > 1 )
	{
		_client->make_move(&_move_list);
		erase_move();
	}
}


void game_view::erase_move()
{
	if (_locked)
		return;

	while (!_move_list.empty())
	{
		_holes[_move_list.back()]->set_hilighted(false);
		_move_list.pop_back();
	}

	_client->hide_move();
}


void game_view::show_move(MoveList *list)
{
	hide_move();

	for (MoveList::iterator move = list->begin();
		move < list->end(); move++)
	{
		_holes[(*move)]->set_hilighted(true);
		_move_list.push_back((*move));
	}
}


void game_view::hide_move()
{
	while (!_move_list.empty())
	{
		_holes[_move_list.back()]->set_hilighted(false);
		_move_list.pop_back();
	}
}


bool game_view::move_list_contains(unsigned int i)
{
	for (MoveList::iterator move = _move_list.begin();
		move < _move_list.end(); move++)
			if (*move == i)
				return true;

	return false;
}


bool game_view::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	// Fill the widget with black (the letterbox area around the board),
	// then paint the board background as a rounded rectangle that hugs the
	// lattice and is centered in the available space.
	double w = get_width();
	double h = get_height();

	cr->set_source_rgb(0, 0, 0);
	cr->paint();

	double bw = _base_width * _scale;
	double bh = _base_height * _scale;
	double bx = _center.get_x() - bw / 2.0;
	double by = _center.get_y() - bh / 2.0;
	double r = 24.0;
	if (r > bw / 2.0) r = bw / 2.0;
	if (r > bh / 2.0) r = bh / 2.0;

	cr->set_source_rgb(0.55, 0.40, 0.24);
	cr->begin_new_path();
	cr->arc(bx + r, by + r, r, PI, 1.5 * PI);
	cr->arc(bx + bw - r, by + r, r, 1.5 * PI, 2 * PI);
	cr->arc(bx + bw - r, by + bh - r, r, 0, 0.5 * PI);
	cr->arc(bx + r, by + bh - r, r, 0.5 * PI, PI);
	cr->close_path();
	cr->fill();

	cr->save();
	cr->translate(_center.get_x(), _center.get_y());
	cr->scale(_scale, _scale);

	// Draw all the holes
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->draw(cr);

	if (_move_list.size() >=2)
	{
		// Draw the arcs beteen holes on the movelist
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end()-1; move++)
				draw_move_arc(cr, _holes[*move]->get_offset(),
					_holes[*(move+1)]->get_offset());
	
		// Then redraw the holes on the move_list on top of the arcs
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end(); move++)
				_holes[*move]->draw(cr);
	}

	cr->restore();

	return true;
}


void game_view::draw_move_arc(const Cairo::RefPtr<Cairo::Context>& cr,
							  Gdk::Point from, Gdk::Point to)
{
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(4);
	cr->move_to(from.get_x(), from.get_y());
	cr->line_to(to.get_x(), to.get_y());
	cr->stroke();
}


void game_view::rotate_to_local_player(unsigned int posn)
{
	g_assert(posn >= 1 && posn <= 6);

	double theta = 0;

	for (int i = 1; i <= 6; i++)
	{
		if (GameBoard::START_MAP[_board->get_num_players()][i] == posn)
		{
			theta = (1 - i) * PI / 3;
			break;
		}
	}

	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole != NULL)
				(*hole)->rotate(theta);
}
