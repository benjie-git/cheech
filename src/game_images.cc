/*
 *   Contains all the static images.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include "game_images.hh"

using namespace std;

GamePoint GameImages::_peg_size;

GameColor GameImages::_peg_fill[GameImages::NUM_COLORS+1];
GameColor GameImages::_peg_edge[GameImages::NUM_COLORS+1];

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
	{110/255.0, 100/255.0, 30/255.0},  // 3 yellow
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
		_peg_fill[i] = GameColor(fill_rgb[i].r, fill_rgb[i].g, fill_rgb[i].b);
		_peg_edge[i] = GameColor(edge_rgb[i].r, edge_rgb[i].g, edge_rgb[i].b);
	}

	_peg_size = GamePoint(peg_size, peg_size);
}


GamePoint GameImages::get_peg_size()
{
	return _peg_size;
}


GameColor GameImages::get_peg_fill(unsigned int id)
{
	g_assert(id <= NUM_COLORS);
	return _peg_fill[id];
}


GameColor GameImages::get_peg_edge(unsigned int id)
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
