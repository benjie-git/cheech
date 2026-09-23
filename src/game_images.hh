/*
 *  Contains all the static images.
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


#ifndef INCL_GAME_IMAGES_HH
#define INCL_GAME_IMAGES_HH

#include <gdkmm/types.h>
#include <gdkmm/rgba.h>


class GameImages 
{
private:
	static const unsigned int NUM_COLORS=8;

	static Gdk::Point _peg_size;

	static Gdk::RGBA _peg_fill[NUM_COLORS+1];
	static Gdk::RGBA _peg_edge[NUM_COLORS+1];

	static const char* color_names[NUM_COLORS];

	GameImages();

public:
	static void init();

	static Gdk::Point get_peg_size();
	static Gdk::RGBA get_peg_fill(unsigned int id);
	static Gdk::RGBA get_peg_edge(unsigned int id);
	static unsigned int get_num_colors();
	static const char* get_color_name(unsigned int color);
};


#endif   // #ifndef INCL_GAME_IMAGES_HH
