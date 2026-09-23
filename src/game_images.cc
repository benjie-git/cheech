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

#include "game_images.hh"

using namespace std;

Gdk::Point GameImages::_peg_size;

Gdk::RGBA GameImages::_peg_fill[GameImages::NUM_COLORS+1];
Gdk::RGBA GameImages::_peg_edge[GameImages::NUM_COLORS+1];

namespace {
struct Rgb { double r, g, b; };

const unsigned int kNumColors = 8;

// Fill and rim colours for the vector pegs (id 0 is an empty hole).
const Rgb fill_rgb[kNumColors+1] = {
	{0.40,      0.26,      0.15},      // 0 empty hole (iOS board colour)
	{226/255.0, 0/255.0,   0/255.0},   // 1 red
	{252/255.0, 142/255.0, 0/255.0},   // 2 orange
	{246/255.0, 243/255.0, 0/255.0},   // 3 yellow
	{0/255.0,   214/255.0, 0/255.0},   // 4 green
	{0/255.0,   149/255.0, 226/255.0}, // 5 blue
	{181/255.0, 0/255.0,   226/255.0}, // 6 purple
	{32/255.0,  32/255.0,  32/255.0},  // 7 black
	{247/255.0, 247/255.0, 247/255.0}  // 8 white
};

const Rgb edge_rgb[kNumColors+1] = {
	{0.40,      0.26,      0.15},      // 0 empty hole
	{112/255.0, 35/255.0,  35/255.0},  // 1 red
	{182/255.0, 103/255.0, 0/255.0},   // 2 orange
	{135/255.0, 134/255.0, 39/255.0},  // 3 yellow
	{0/255.0,   140/255.0, 0/255.0},   // 4 green
	{35/255.0,  86/255.0,  113/255.0}, // 5 blue
	{118/255.0, 0/255.0,   148/255.0}, // 6 purple
	{26/255.0,  26/255.0,  26/255.0},  // 7 black
	{191/255.0, 191/255.0, 191/255.0}  // 8 white
};

const int peg_size = 19;
}

const char* GameImages::color_names[GameImages::NUM_COLORS] =
	{"red", "orange", "yellow", "green", "blue", "purple", "black", "white"};

void GameImages::init()
{
	for (unsigned int i = 0; i <= NUM_COLORS; i++)
	{
		_peg_fill[i].set_rgba(fill_rgb[i].r, fill_rgb[i].g, fill_rgb[i].b);
		_peg_edge[i].set_rgba(edge_rgb[i].r, edge_rgb[i].g, edge_rgb[i].b);
	}

	_peg_size = Gdk::Point(peg_size, peg_size);
}


Gdk::Point GameImages::get_peg_size()
{
	return _peg_size;
}


Gdk::RGBA GameImages::get_peg_fill(unsigned int id)
{
	g_assert(id <= NUM_COLORS);
	return _peg_fill[id];
}


Gdk::RGBA GameImages::get_peg_edge(unsigned int id)
{
	g_assert(id <= NUM_COLORS);
	return _peg_edge[id];
}


unsigned int GameImages::get_num_colors()
{
	return NUM_COLORS;
}


const char *GameImages::get_color_name(unsigned int color)
{
	g_assert(color >= 1 && color <= NUM_COLORS);
	return color_names[color-1];
}
