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

	set_size_request((int)(GameImages::get_peg_size().get_x()
		* (GameBoard::SIZE_X + 1) * MULTIPLIER), 
		(int)(GameImages::get_peg_size().get_y() * (GameBoard::SIZE_Y - 1)
		* MULTIPLIER * sqrt(3.0) / 2.0));

	grab_focus();
}


void game_view::on_realize()
{
	// We need to call the base on_realize()
	Gtk::DrawingArea::on_realize();

	// Now we can allocate any additional resources we need
	_window = get_window();
	_gc = Gdk::GC::create(_window);
	_gc->set_line_attributes(4, Gdk::LINE_SOLID, Gdk::CAP_BUTT,
		Gdk::JOIN_MITER);
	_window->clear();

	create_holes();
}


bool game_view::on_configure_event(GdkEventConfigure* event)
{
	_center = Gdk::Point(event->width/2, event->height/2);

	return true;
}


void game_view::create_holes()
{
	GameBoard b(1, false, false, false);

	Gdk::Point board_center((int)(GameImages::get_peg_size().get_x()
		* (GameBoard::SIZE_X + 1) * MULTIPLIER/2.0), 
		(int)(GameImages::get_peg_size().get_y() * (GameBoard::SIZE_Y - 1)
		* MULTIPLIER * sqrt(3.0) / 4.0));

	_holes.resize(_board->get_size());

	for (unsigned int i=0; i < _board->get_size(); i++)
	{
		if (b[i])
		{
			double x = GameImages::get_peg_size().get_x();
			double y = GameImages::get_peg_size().get_y();
			double x_offset = (util::even(i / GameBoard::SIZE_X) ? 0.5 : 0.0)
								+ 1.0;
			double y_offset = 0;
			Gdk::Point offset((int)(((i % GameBoard::SIZE_X)
				+ x_offset) * x * MULTIPLIER)-board_center.get_x(),
				(int)(((i / GameBoard::SIZE_X) + y_offset) * y * MULTIPLIER 
				* (sqrt(3.0) / 2.0))-board_center.get_y());
			
			_holes[i] = new GameViewHole(&_center, &offset);
		}
		else
			_holes[i] = NULL;
	}
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
		for (unsigned int i=0; i < _board->get_size(); i++)
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
		case GDK_Escape:
			if (!_locked)
			{
				erase_move();
				return true;
			}
		case GDK_Return:
		case GDK_space:
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


bool game_view::on_expose_event(GdkEventExpose* event)
{
	Glib::RefPtr<Gdk::Window> window = get_window();
	_window->clear();

	// Don't draw the board unless we're connected to one
	if (!_client)
		return true;

	// Draw all the holes
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->draw(_window, _gc);

	if (_move_list.size() >=2)
	{
		// Draw the arcs beteen holes on the movelist
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end()-1; move++)
				draw_move_arc(_window, _holes[*move]->get_location(),
					_holes[*(move+1)]->get_location());
	
		// Then redraw the holes on the move_list on top of the arcs
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end(); move++)
				_holes[*move]->draw(_window, _gc);
	}

	return true;
}


void game_view::draw_move_arc(Glib::RefPtr<Gdk::Window> window,
							  Gdk::Point from, Gdk::Point to)
{
	_window->draw_line(_gc, from.get_x(), from.get_y(), to.get_x(), to.get_y());
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
