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

#ifndef INCL_GAME_HOLE_HH
#define INCL_GAME_HOLE_HH

#include <glib.h>
#include <sigc++/sigc++.h>


class GameHole : public sigc::trackable
{
public:
	GameHole(unsigned int id, unsigned int start_player = 0,
			 unsigned int end_player = 0);
	GameHole(const GameHole& hole);
	GameHole& operator=(const GameHole& hole);
	virtual ~GameHole();

	sigc::signal<void, int> evt_player_changed;

	unsigned int get_id() const;
	unsigned int get_start_player() const;
	unsigned int get_end_player() const;
	unsigned int get_current_player() const;
	unsigned int get_peg_list_index() const;
	GameHole* get_neighbor(unsigned int dir, unsigned int dist = 1) const;
	double get_location_x() const;
	double get_location_y() const;
	double get_distance_to(GameHole *to) const;

	void set_current_player(unsigned int current_player);
	void set_peg_list_index(unsigned int index);
	void set_start_player(unsigned int start_player);
	void set_end_player(unsigned int end_player);
	void set_neighbor(unsigned int i, GameHole* neighbor);
	void set_location(double x, double y);

private:
	unsigned int		_id;
	unsigned int		_start_player;
	unsigned int		_end_player;
	unsigned int		_current_player;
	unsigned int		_peg_list_index;
	GameHole*			_neighbor[6];
	double				_location_x;
	double				_location_y;
};

#endif   // #ifndef INCL_GAME_HOLE_HH
