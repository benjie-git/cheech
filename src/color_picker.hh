//
//    A row of clickable peg-colored circles, matching the iOS color picker:
//    the selected color draws at full opacity with a bold ring, the rest are
//    dimmed with a faint ring.
//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//

#ifndef _COLOR_PICKER_HH
#  define _COLOR_PICKER_HH

#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <cairomm/context.h>
#include <sigc++/sigc++.h>
#include "game_images.hh"


class ColorSwatch : public Gtk::DrawingArea
{
	public:
		ColorSwatch(unsigned int color) : _color(color), _selected(false)
		{
			set_size_request(24, 24);
			// GTK4 has no on_draw vfunc; drawing goes through set_draw_func.
			set_draw_func(sigc::mem_fun(*this, &ColorSwatch::on_draw));
			Glib::RefPtr<Gtk::GestureClick> gesture = Gtk::GestureClick::create();
			gesture->set_button(0);
			gesture->signal_pressed().connect(sigc::mem_fun(*this,
				&ColorSwatch::on_swatch_pressed));
			add_controller(gesture);
		}

		unsigned int color() const { return _color; }

		void set_selected(bool selected)
		{
			_selected = selected;
			queue_draw();
		}

		sigc::signal<void(unsigned int)> signal_clicked;

	protected:
		void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int, int);

	private:
		void on_swatch_pressed(int, double, double)
		{
			signal_clicked.emit(_color);
		}

		unsigned int _color;
		bool _selected;
};


inline void ColorSwatch::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
	int, int)
{
	static const double CP_PI = 3.14159265358979;

	double w = get_width();
	double h = get_height();
	double r = (w < h ? w : h) / 2.0 - 2.5;
	if (r <= 0)
		return;
	double cx = w / 2.0;
	double cy = h / 2.0;

	const GameColor fill = GameImages::get_peg_fill(_color);
	Gdk::RGBA fg = get_style_context()->get_color();

	cr->set_source_rgba(fill.r, fill.g, fill.b,
		_selected ? 1.0 : 0.7);
	cr->arc(cx, cy, r, 0, 2 * CP_PI);
	cr->fill();

	cr->set_source_rgba(fg.get_red(), fg.get_green(), fg.get_blue(),
		_selected ? 0.9 : 0.5);
	cr->set_line_width(_selected ? 2.5 : 1.0);
	cr->arc(cx, cy, r, 0, 2 * CP_PI);
	cr->stroke();
}


class ColorPicker : public Gtk::Box
{
	public:
		ColorPicker() : Gtk::Box(Gtk::Orientation::HORIZONTAL, 5), _selected(1)
		{
			for (unsigned int c = 1; c <= 8; c++)
			{
				ColorSwatch *swatch = Gtk::make_managed<ColorSwatch>(c);
				swatch->signal_clicked.connect(
					sigc::mem_fun(*this, &ColorPicker::on_swatch_clicked));
				append(*swatch);
				_swatches.push_back(swatch);
			}
			set_selected(_selected);
		}

		unsigned int get_selected() const { return _selected; }

		// Updates the visuals only.  A user click additionally emits
		// signal_changed.
		void set_selected(unsigned int color)
		{
			_selected = color;
			for (size_t i = 0; i < _swatches.size(); i++)
				_swatches[i]->set_selected(_swatches[i]->color() == color);
		}

		sigc::signal<void(unsigned int)> signal_changed;

	private:
		void on_swatch_clicked(unsigned int color)
		{
			set_selected(color);
			signal_changed(color);
		}

		std::vector<ColorSwatch *> _swatches;
		unsigned int _selected;
};


#endif
