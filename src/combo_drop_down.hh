//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//
//    ComboDropDown: a Gtk::DropDown wearing Gtk::ComboBoxText's API.
//
//    The selection widgets were migrated from Gtk::ComboBoxText to
//    Gtk::DropDown; this wrapper keeps the old append/set_active_text/
//    get_active_text code unchanged.  The join page's editable hostname
//    combo stays a Gtk::ComboBoxText, since Gtk::DropDown cannot accept
//    arbitrary typed text.
//

#ifndef _COMBO_DROP_DOWN_HH
#define _COMBO_DROP_DOWN_HH

#include <gtkmm/dropdown.h>
#include <gtkmm/stringlist.h>


class ComboDropDown : public Gtk::DropDown
{
  public:
	ComboDropDown()
	{
		_model = Gtk::StringList::create();
		set_model(_model);
		property_selected().signal_changed().connect(
			sigc::mem_fun(*this, &ComboDropDown::on_selection_changed));
	}

	void append(const Glib::ustring &text)
		{ _model->append(text); }

	void set_active(int index)
	{
		set_selected(index >= 0 ? (unsigned int) index
			: GTK_INVALID_LIST_POSITION);
	}

	int get_active_row_number() const
	{
		return get_selected() == GTK_INVALID_LIST_POSITION ? -1 :
			(int) get_selected();
	}

	void set_active_text(const Glib::ustring &text)
	{
		for (unsigned int i = 0; i < _model->get_n_items(); i++)
			if (_model->get_string(i) == text)
				{ set_selected(i); return; }
		set_active(-1);
	}

	Glib::ustring get_active_text() const
	{
		return get_selected() == GTK_INVALID_LIST_POSITION ?
			Glib::ustring() : _model->get_string(get_selected());
	}

	sigc::signal<void()> &signal_changed()
		{ return _signal_changed; }


  protected:
	void on_selection_changed()
		{ _signal_changed.emit(); }

	Glib::RefPtr<Gtk::StringList> _model;
	sigc::signal<void()> _signal_changed;
};


#endif
