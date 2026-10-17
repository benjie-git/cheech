/*
 *   A hole in the Chinese Checkers board.
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
 *  Gnome Chinese Checkers
 *  Copyright (C) 1999-2003 Loban Rahman
 *  Copyright (C) 2005-2026 Ben Levitt
 */

#include <math.h>

#include "game_hole.hh"
#include "utility.hh"


GameHole::GameHole(unsigned int id, unsigned int start_player,
				   unsigned int end_player)
	:_id(id),
	 _start_player(start_player),
	 _end_player(end_player),
	 _current_player(0),
	 _peg_list_index(0)
{
	_location_x = 0;
	_location_y = 0;

	for (int i = 0; i < 6; i++)
		_neighbor[i] = NULL;
}


GameHole::GameHole(const GameHole& hole)
{
	*this = hole;
}


GameHole& GameHole::operator=(const GameHole& hole)
{
	if (this == &hole)
		return *this;

	_id = hole._id;
	_start_player = hole._start_player;
	_end_player = hole._end_player;
	_current_player = hole._current_player;
	_peg_list_index = hole._peg_list_index;
	_location_x = hole._location_x;
	_location_y = hole._location_y;

	for (int i = 0; i < 6; i++)
		_neighbor[i] = hole._neighbor[i];

	return *this;
}


GameHole::~GameHole()
{
}


void GameHole::set_neighbor(unsigned int i, GameHole* neighbor) 
{
	if (neighbor && get_distance_to(neighbor) < 2)
		_neighbor[i] = neighbor;
}


double GameHole::get_distance_to(GameHole *to) const
{
	return (sqrt(pow(to->get_location_x() - get_location_x(), 2.0)
		+ pow(to->get_location_y() - get_location_y(), 2.0)));
}
