/*
 *  A hole in the Chinese Checkers board.
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


unsigned int GameHole::get_id() const
{ 
	return _id;
}


unsigned int GameHole::get_start_player() const
{
	return _start_player;
}


unsigned int GameHole::get_end_player() const
{
	return _end_player;
}


unsigned int GameHole::get_current_player() const
{
	return _current_player;
}


unsigned int GameHole::get_peg_list_index() const
{
	return _peg_list_index;
}


GameHole* GameHole::get_neighbor(unsigned int dir, unsigned int dist) const 
{
	if (dist == 1 || _neighbor[dir] == NULL)
		return _neighbor[dir];

	return _neighbor[dir]->get_neighbor(dir, dist-1);
}


double GameHole::get_location_x() const
{
	return _location_x;
}


double GameHole::get_location_y() const
{
	return _location_y;
}


void GameHole::set_current_player(unsigned int current_player) 
{
	_current_player = current_player; 
	evt_player_changed(_current_player);
}


void GameHole::set_start_player(unsigned int start_player) 
{
	_start_player = start_player; 
}


void GameHole::set_end_player(unsigned int end_player) 
{
	_end_player = end_player; 
}


void GameHole::set_peg_list_index(unsigned int index) 
{
	_peg_list_index = index; 
}


void GameHole::set_neighbor(unsigned int i, GameHole* neighbor) 
{
	if (neighbor && get_distance_to(neighbor) < 2)
		_neighbor[i] = neighbor;
}


void GameHole::set_location(double x, double y)
{
	_location_x = x;
	_location_y = y;
}


double GameHole::get_distance_to(GameHole *to) const
{
	return (sqrt(pow(to->get_location_x() - get_location_x(), 2.0)
		+ pow(to->get_location_y() - get_location_y(), 2.0)));
}
