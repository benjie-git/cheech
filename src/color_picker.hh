//
//    A row of clickable peg-colored circles, matching the iOS color picker:
//    the selected color draws at full opacity with a bold ring, the rest are
//    dimmed with a faint ring.
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef _COLOR_PICKER_HH
#  define _COLOR_PICKER_HH

#include <gtkmm/drawingarea.h>
#include <gtkmm/box.h>
#include <cairomm/context.h>
#include <sigc++/sigc++.h>
#include "game_images.hh"


class ColorSwatch : public Gtk::DrawingArea
{
	public:
		ColorSwatch(unsigned int color) : _color(color), _selected(false)
		{
			set_size_request(24, 24);
			add_events(Gdk::BUTTON_PRESS_MASK);
		}

		unsigned int color() const { return _color; }

		void set_selected(bool selected)
		{
			_selected = selected;
			queue_draw();
		}

		sigc::signal<void, unsigned int> signal_clicked;

	protected:
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
		virtual bool on_button_press_event(GdkEventButton* event);

	private:
		unsigned int _color;
		bool _selected;
};


inline bool ColorSwatch::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	static const double CP_PI = 3.14159265358979;

	double w = get_width();
	double h = get_height();
	double r = (w < h ? w : h) / 2.0 - 2.5;
	if (r <= 0)
		return true;
	double cx = w / 2.0;
	double cy = h / 2.0;

	Gdk::RGBA fill = GameImages::get_peg_fill(_color);
	Gdk::RGBA fg;
	get_style_context()->get_color(Gtk::STATE_FLAG_NORMAL);

	cr->set_source_rgba(fill.get_red(), fill.get_green(), fill.get_blue(),
		_selected ? 1.0 : 0.7);
	cr->arc(cx, cy, r, 0, 2 * CP_PI);
	cr->fill();

	cr->set_source_rgba(fg.get_red(), fg.get_green(), fg.get_blue(),
		_selected ? 0.9 : 0.35);
	cr->set_line_width(_selected ? 2.5 : 1.0);
	cr->arc(cx, cy, r, 0, 2 * CP_PI);
	cr->stroke();

	return true;
}


inline bool ColorSwatch::on_button_press_event(GdkEventButton*)
{
	signal_clicked(_color);
	return true;
}


class ColorPicker : public Gtk::HBox
{
	public:
		ColorPicker() : Gtk::HBox(false, 5), _selected(1)
		{
			for (unsigned int c = 1; c <= 8; c++)
			{
				ColorSwatch *swatch = Gtk::manage(new class ColorSwatch(c));
				swatch->signal_clicked.connect(
					sigc::mem_fun(*this, &ColorPicker::on_swatch_clicked));
				pack_start(*swatch, Gtk::PACK_SHRINK);
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

		sigc::signal<void, unsigned int> signal_changed;

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
