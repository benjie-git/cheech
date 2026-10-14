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

#include <gdk/gdk.h>
#include "click_counter.hh"
#include <gtkmm/eventcontrollerkey.h>
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
	_center = GamePoint();
	_board_center = GamePoint();
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

	_board_center = GamePoint((int)lround((min_x + max_x) / 2.0),
							   (int)lround((min_y + max_y) / 2.0));
	_base_width = (int)lround(max_x - min_x + 2.0 * pad);
	_base_height = (int)lround(max_y - min_y + 2.0 * pad);

	set_size_request(_base_width, _base_height);

	set_focusable(true);
	grab_focus();

	// Clicks (any button; double-click commits a move) and key presses.
	// ClickCounter counts successive clicks itself: GTK's built-in
	// counter does not always restart when the pointer moves between
	// clicks.
	auto click = ClickCounter::create();
	click->signal_counted().connect(sigc::mem_fun(*this,
		&game_view::on_view_clicked));
	add_controller(click);

	auto key = Gtk::EventControllerKey::create();
	key->signal_key_pressed().connect(sigc::mem_fun(*this,
		&game_view::on_view_key_pressed), false);
	add_controller(key);

	set_draw_func(sigc::mem_fun(*this, &game_view::on_draw));

	signal_resize().connect(sigc::mem_fun(*this, &game_view::on_resize));
}


void game_view::on_realize()
{
	// We need to call the base on_realize()
	Gtk::DrawingArea::on_realize();

	create_holes();
}


void game_view::on_resize(int width, int height)
{
	_center = GamePoint(width/2, height/2);

	double sx = (double)width / _base_width;
	double sy = (double)height / _base_height;
	_scale = (sx < sy) ? sx : sy;

	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->set_scale(_scale);

	queue_draw();

	evt_board_resized.emit();
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
			GamePoint offset((int)(((i % GameBoard::SIZE_X)
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


// Any button press clears/rebuilds the selection; double-click on the last
// hole of a path commits the move.  The click count comes from ClickCounter
// (see click_counter.hh).
void game_view::on_view_clicked(unsigned int clicks, double x, double y)
{
	grab_focus();
	evt_user_action.emit();

	if (_locked)
		return;

	for (unsigned int i = 0; i < _holes.size(); i++)
	{
		if (_holes[i] && _holes[i]->in_bounds((int)x, (int)y))
		{
			if (clicks == 1)
			{
				read_move(i);
			}
			else if (clicks == 2)
			{
				if (_move_list.size() > 0 && _move_list.back() != i)
					read_move(i);
				if (_move_list.size() >= 2 && i == _move_list.back())
					write_move();
			}

			return;
		}
	}
}


bool game_view::on_view_key_pressed(guint keyval, guint keycode,
	Gdk::ModifierType state)
{
	evt_user_action.emit();

	switch (keyval)
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
			if (state == Gdk::ModifierType(0) ||
				state == Gdk::ModifierType(GDK_SHIFT_MASK))
			{
				evt_unhandled_key.emit(keyval);
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


void game_view::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int, int)
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

	// Draw the lattice connecting adjacent holes (same edge rule as the
	// iOS board: each hole's forward neighbours, guarded by distance so
	// row boundaries don't wrap).
	double unit_x = GameImages::get_peg_size().get_x() * MULTIPLIER;
	cr->set_source_rgb(0.49, 0.34, 0.20);
	cr->set_line_width(std::max(1.0, unit_x * 0.06));
	cr->begin_new_path();
	for (unsigned int i = 0; i < _holes.size(); i++)
	{
		if (!_holes[i])
			continue;

		unsigned int row = i / GameBoard::SIZE_X;
		unsigned int candidates[3];
		unsigned int ncand;
		if (util::even(row))
		{
			candidates[0] = i + 1;
			candidates[1] = i + GameBoard::SIZE_X + 1;
			candidates[2] = i + GameBoard::SIZE_X;
			ncand = 3;
		}
		else
		{
			candidates[0] = i + 1;
			candidates[1] = i + GameBoard::SIZE_X;
			candidates[2] = i + GameBoard::SIZE_X - 1;
			ncand = 3;
		}

		GamePoint a = _holes[i]->get_offset();
		for (unsigned int c = 0; c < ncand; c++)
		{
			if (candidates[c] >= _holes.size() || !_holes[candidates[c]])
				continue;

			GamePoint b = _holes[candidates[c]]->get_offset();
			double dx = b.get_x() - a.get_x();
			double dy = b.get_y() - a.get_y();
			if (sqrt(dx * dx + dy * dy) >= unit_x * 1.1)
				continue;

			cr->move_to(a.get_x(), a.get_y());
			cr->line_to(b.get_x(), b.get_y());
		}
	}
	cr->stroke();

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

	return;
}


void game_view::draw_move_arc(const Cairo::RefPtr<Cairo::Context>& cr,
							  GamePoint from, GamePoint to)
{
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(4);
	cr->move_to(from.get_x(), from.get_y());
	cr->line_to(to.get_x(), to.get_y());
	cr->stroke();
}


void game_view::rotate_to_local_player(unsigned int posn)
{
	// The caller normally guarantees a board, but a stray early call would
	// previously have crashed here.
	if (_board)
		rotate_to_seat(_board->get_num_players(), posn);
}


void game_view::rotate_to_seat(unsigned int num_players, unsigned int posn)
{
	g_assert(posn >= 1 && posn <= 6);

	double theta = 0;

	for (int i = 1; i <= 6; i++)
	{
		if (GameBoard::START_MAP[num_players][i] == posn)
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
