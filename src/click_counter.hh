//
//    ClickCounter: a Gtk::GestureClick that counts successive clicks
//    itself.
//
//    GtkGestureClick does not always restart its click count when the
//    pointer has moved between clicks (e.g. select a peg, then immediately
//    double click a nearby hole turns the double click into a triple
//    click), so the count is tracked here: it restarts when the pointer
//    moved further than the double click distance, or more than the double
//    click time has passed since the previous click.  Triple clicks count
//    as double clicks.  All buttons are recognized.
//

#ifndef _CLICK_COUNTER_HH
#define _CLICK_COUNTER_HH

#include <gtkmm/gestureclick.h>
#include <gtkmm/settings.h>


class ClickCounter : public Gtk::GestureClick
{
  public:
	static Glib::RefPtr<ClickCounter> create()
	{
		return Glib::make_refptr_for_instance<ClickCounter>(
			new ClickCounter());
	}

	// Emitted for every press: number of successive clicks (capped at
	// two), and the press position.
	sigc::signal<void(unsigned int, double, double)> &signal_counted()
		{ return _signal_counted; }


  protected:
	ClickCounter()
	{
		set_button(0);
		signal_pressed().connect(sigc::mem_fun(*this,
			&ClickCounter::on_pressed));
	}

	void on_pressed(int, double x, double y)
	{
		gint64 now = g_get_monotonic_time();
		int double_time = 400, double_dist = 5;
		Glib::RefPtr<Gtk::Settings> settings = Gtk::Settings::get_default();
		if (settings)
		{
			settings->get_property("gtk-double-click-time", double_time);
			settings->get_property("gtk-double-click-distance",
				double_dist);
		}

		if (_click_count > 0 &&
			now - _last_click_time <= (gint64)double_time * 1000 &&
			fabs(x - _last_click_x) <= double_dist &&
			fabs(y - _last_click_y) <= double_dist)
			_click_count++;
		else
			_click_count = 1;
		_last_click_time = now;
		_last_click_x = x;
		_last_click_y = y;

		_signal_counted.emit(_click_count < 2 ? _click_count : 2, x, y);
	}

	gint64 _last_click_time;
	double _last_click_x;
	double _last_click_y;
	unsigned int _click_count;
	sigc::signal<void(unsigned int, double, double)> _signal_counted;
};


#endif
