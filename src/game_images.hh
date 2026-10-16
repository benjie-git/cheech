/*
 *   Contains all the static images.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */


#ifndef INCL_GAME_IMAGES_HH
#define INCL_GAME_IMAGES_HH

#include <glib.h>


// Gdk::Point was removed in gtkmm4; this simple replacement keeps the
// get_x()/get_y() accessors used throughout the game view code.
struct GamePoint
{
	int x;
	int y;
	GamePoint() : x(0), y(0) {}
	GamePoint(int x_, int y_) : x(x_), y(y_) {}
	int get_x() const { return x; }
	int get_y() const { return y; }
};

// Simple RGBA color, independent of gdkmm so that the headless
// binaries (cheechd/cheechbot/cheechwebd) do not need GTK.
struct GameColor
{
	double r, g, b, a;
	GameColor() : r(0), g(0), b(0), a(1.0) {}
	GameColor(double r_, double g_, double b_, double a_ = 1.0)
		: r(r_), g(g_), b(b_), a(a_) {}
};


class GameImages
{
private:
	static const unsigned int NUM_COLORS=8;

	static GamePoint _peg_size;

	static GameColor _peg_fill[NUM_COLORS+1];
	static GameColor _peg_edge[NUM_COLORS+1];

	static const char* color_names[NUM_COLORS];

	GameImages();

public:
	static void init();

	static GamePoint get_peg_size();
	static GameColor get_peg_fill(unsigned int id);
	static GameColor get_peg_edge(unsigned int id);
	static unsigned int get_num_colors();
	static const char* get_color_name(unsigned int color);
};


#endif   // #ifndef INCL_GAME_IMAGES_HH
