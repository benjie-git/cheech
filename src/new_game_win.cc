/*
 *  The New/Join Game Window.
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

#include <iostream>
#include "config.h"
#include "new_game_win.hh"
#include "bot_catalog.hh"
#include <gtkmm/eventcontrollerkey.h>
#include <gdkmm/event.h>
#include <glibmm/i18n.h>
#include "utility.hh"


namespace
{
	// Split on unescaped '|' characters ('\\' escapes the next character).
	std::vector<Glib::ustring> split_fields(const Glib::ustring &line)
	{
		std::vector<Glib::ustring> fields;
		Glib::ustring field;
		bool escaped = false;

		for (Glib::ustring::const_iterator i = line.begin();
			 i != line.end(); i++)
		{
			gunichar ch = *i;
			if (escaped)
			{
				field += ch;
				escaped = false;
			}
			else if (ch == '\\')
				escaped = true;
			else if (ch == '|')
			{
				fields.push_back(field);
				field.clear();
			}
			else
				field += ch;
		}
		fields.push_back(field);
		return fields;
	}

	std::vector<unsigned int> split_indices(const Glib::ustring &list)
	{
		std::vector<unsigned int> indices;
		Glib::ustring::size_type start = 0;

		while (start <= list.length())
		{
			Glib::ustring::size_type comma = list.find(',', start);
			Glib::ustring item = (comma == Glib::ustring::npos) ?
				list.substr(start) : list.substr(start, comma - start);
			if (!item.empty())
				indices.push_back(util::from_str<unsigned int>(item));
			if (comma == Glib::ustring::npos)
				break;
			start = comma + 1;
		}
		return indices;
	}

	Glib::ustring escape_field(Glib::ustring field)
	{
		Glib::ustring::size_type pos = 0;
		while ((pos = field.find('\\', pos)) != Glib::ustring::npos)
		{
			field.replace(pos, 1, "\\\\");
			pos += 2;
		}
		pos = 0;
		while ((pos = field.find('|', pos)) != Glib::ustring::npos)
		{
			field.replace(pos, 1, "\\|");
			pos += 2;
		}
		return field;
	}

	Glib::ustring unescape_field(Glib::ustring field)
	{
		Glib::ustring::size_type pos = 0;
		while ((pos = field.find("\\|", pos)) != Glib::ustring::npos)
		{
			field.replace(pos, 2, "|");
			pos += 1;
		}
		pos = 0;
		while ((pos = field.find("\\\\", pos)) != Glib::ustring::npos)
		{
			field.replace(pos, 2, "\\");
			pos += 1;
		}
		return field;
	}
}


new_game_win::new_game_win() : new_game_win_glade()
{
	Prefs prefs;

	prefs.read();

	_player_name = prefs.name;
	_next_seat_id = 1;
	_loading_seat = false;

	name_entry->set_text(prefs.name);
	set_selected_color(prefs.color);

	load_seats();

	host_port_entry->set_text(util::to_str(prefs.host_port));
	cheechweb->set_active(prefs.start_cheechweb);
	cheechweb_port_entry->set_text(util::to_str(prefs.cheechweb_port));
	cheechweb_port_entry->set_sensitive(cheechweb->get_active());

	long_jumps->set_active(prefs.long_jumps);
	hop_others->set_active(prefs.hop_others);
	stop_others->set_active(prefs.stop_others);

	refresh_join_hostnames();
	join_host_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_button_clicked));
	join_host_list->signal_row_activated().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_host_picked));
	join_host_entry->signal_icon_press().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_host_icon_pressed));

	// Delete/backspace removes the selected host from the saved history
	// and the popup list (so accidentally added rows can be pruned).
	auto controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_host_key_pressed), false);
	join_host_list->add_controller(controller);

	join_port_entry->set_text(util::to_str<unsigned int>(prefs.join_port));
	spectator->set_active(prefs.spectator);

	// Remember the seat layout across games and app runs, even when the
	// dialog is closed without starting a game.
	signal_hide().connect(sigc::mem_fun(*this, &new_game_win::save_seats));

	// Focus the Start Game button when the dialog is opened (also on
	// re-show, since the window is only hidden on close), and refresh
	// the join hostname list (hostnames learned during a session show
	// up the next time the dialog is opened).
	signal_map().connect([this]() {
		refresh_join_hostnames();
		host_button->grab_focus();
	});
}


void new_game_win::refresh_join_hostnames()
{
	Prefs prefs;
	prefs.read();

	join_host_list->remove_all();

	for (std::vector<Glib::ustring>::iterator i = prefs.join_hostnames.begin();
		 i != prefs.join_hostnames.end(); i++) {
		Gtk::Label *host_row = Gtk::make_managed<Gtk::Label>(*i);
		host_row->set_halign(Gtk::Align::START);
		host_row->set_margin(6);
		join_host_list->append(*host_row);
	}

	join_host_entry->set_text(prefs.join_hostnames.size() > 0 ?
		prefs.join_hostnames.front() : "");
}


void new_game_win::show_join_page(bool joining)
{
	if (joining)
	{
		role_notebook->set_current_page(1);
		set_default_widget(*join_button);
	}
	else
	{
		role_notebook->set_current_page(0);
		set_default_widget(*host_button);
	}
}


// Seat rows -----------------------------------------------------------------


void new_game_win::add_seat(const NewGameSeat &seat)
{
	SeatRow *row = new SeatRow;

	row->id = _next_seat_id++;
	row->color = seat.color;
	row->focus_pending = !seat.focus_all;
	row->focus_pending_ids = seat.focus_ids;
	row->sep = NULL;
	if (!_rows.empty())
	{
		// A thin line between the seats.
		row->sep = new class Gtk::Separator(Gtk::Orientation::HORIZONTAL);
		seats_box->append(*row->sep);
		row->sep->set_margin_top(2);
		row->sep->show();
	}

	row->box = new Gtk::Box(Gtk::Orientation::VERTICAL, 3);
	row->box->set_margin_top(3);
	row->box->set_margin_bottom(3);
	row->box->set_margin_start(3);
	row->box->set_margin_end(3);

	row->kind_box = Gtk::make_managed<ComboDropDown>();
	row->kind_box->append(_("Human"));
	row->kind_box->append(_("Computer"));
	row->kind_box->append(_("Remote"));
	row->kind_box->set_active(seat.kind);

	row->family_box = Gtk::make_managed<ComboDropDown>();
	row->family_box->append(BotUI::FRIENDLY);
	row->family_box->append(BotUI::NEUTRAL);
	row->family_box->append(BotUI::MEAN);

	row->name_entry = Gtk::make_managed<class Gtk::Entry>();
	row->name_entry->set_text(seat.name);
	row->name_entry->set_max_width_chars(12);

	row->color_picker = Gtk::make_managed<class ColorPicker>();
	row->color_picker->set_selected(row->color >= 1 && row->color <= 8 ?
		row->color : 1);

	Gtk::Box *top_box = Gtk::make_managed<class Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	top_box->append(*row->kind_box);
	top_box->append(*row->family_box);
	top_box->append(*row->name_entry);

	Gtk::Box *depth_box = Gtk::make_managed<class Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	depth_box->append(*Gtk::make_managed<class Gtk::Label>(_("Depth:")));
	row->depth_box = Gtk::make_managed<ComboDropDown>();
	for (size_t i = 0; i < BotUI::depth_values().size(); i++)
		row->depth_box->append(
			BotUI::depth_label(BotUI::depth_values()[i]));
	depth_box->append(*row->depth_box);

	Gtk::Box *smarts_box = Gtk::make_managed<class Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	smarts_box->append(*Gtk::make_managed<class Gtk::Label>(_("Smarts:")));
	row->smarts_box = Gtk::make_managed<ComboDropDown>();
	for (size_t i = 0; i < BotUI::skill_levels().size(); i++)
		row->smarts_box->append(
			BotUI::skill_label(BotUI::skill_levels()[i]));
	smarts_box->append(*row->smarts_box);

	row->focus_box = Gtk::make_managed<class Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);

	Gtk::Box *details_box = Gtk::make_managed<class Gtk::Box>(
		Gtk::Orientation::VERTICAL, 3);
	details_box->set_margin_top(3);
	details_box->set_margin_bottom(3);
	details_box->set_margin_start(3);
	details_box->set_margin_end(3);
	details_box->append(*depth_box);
	details_box->append(*smarts_box);
	details_box->append(*row->focus_box);

	row->details = Gtk::make_managed<class Gtk::Expander>(_("Details"));
	row->details->set_child(*details_box);

	row->box->append(*top_box);
	row->box->append(*row->color_picker);
	row->box->append(*row->details);

	// Per-row state so changed handlers know which row fired.
	row->kind_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this, &new_game_win::on_seat_kind_changed),
			row));
	row->family_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_family_changed), row));
	row->name_entry->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_name_changed), row));
	row->color_picker->signal_changed.connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_color_changed), row));
	row->depth_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_depth_changed), row));
	row->smarts_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_smarts_changed), row));

	// Populate the family/depth/smarts/focus selections without triggering
	// the default-name followers: a loaded name is taken as-is.
	_loading_seat = true;
	row->family_box->set_active_text(seat.family.empty() ?
		BotUI::NEUTRAL : seat.family);
	row->depth_box->set_active_text(BotUI::depth_label(seat.depth));
	row->smarts_box->set_active_text(BotUI::skill_label(
		seat.level >= 1 && seat.level <= 4 ? seat.level : 1));
	_loading_seat = false;

	// Remember the name this seat uses as a Human.
	if (!seat.human_name.empty())
		_human_names[row->id] = seat.human_name;
	else
		_human_names[row->id] = (seat.kind == 0 && !seat.name.empty()) ?
			seat.name : _player_name;

	seats_box->append(*row->box);
	// In GTK4 widgets are visible by default; sync_seat_row() below hides
	// the entries that do not apply to this row's kind.
	_rows.push_back(row);

	sync_seat_row(*row);
	refresh_focus_rows();
}


void new_game_win::remove_seat(size_t index)
{
	if (index >= _rows.size() || _rows.size() <= 2)
		return;

	SeatRow *row = _rows[index];
	if (row->sep)
	{
		seats_box->remove(*row->sep);
		delete row->sep;
	}
	seats_box->remove(*row->box);
	delete row->box;
	_rows.erase(_rows.begin() + index);
	_human_names.erase(row->id);
	_default_names.erase(row->id);
	delete row;

	refresh_focus_rows();
}


new_game_win::SeatRow *new_game_win::find_row(unsigned int id)
{
	for (size_t i = 0; i < _rows.size(); i++)
		if (_rows[i]->id == id)
			return _rows[i];
	return NULL;
}


void new_game_win::on_add_seat_button_clicked()
{
	// The board supports at most six players.
	if (_rows.size() >= 6)
		return;

	NewGameSeat seat;

	// New seats default to computer players, set up like the last bot the
	// user configured (here or in the in-game Computer Players window).
	Prefs prefs;
	prefs.read();

	seat.kind = 1;
	seat.name = "";
	seat.color = first_unused_color();
	seat.family = prefs.bot_type.empty() ? BotUI::NEUTRAL : prefs.bot_type;
	seat.level = prefs.bot_level >= 1 && prefs.bot_level <= 4 ?
		prefs.bot_level : 1;
	seat.depth = prefs.bot_depth;
	seat.focus_all = true;
	seat.id = 0;

	add_seat(seat);
}


void new_game_win::on_remove_seat_button_clicked()
{
	if (_rows.empty())
		return;
	remove_seat(_rows.size() - 1);
}


// Seat row handlers ---------------------------------------------------------


Glib::ustring new_game_win::default_name_for(SeatRow *row)
{
	unsigned int depth = BotUI::depth_values()[
		std::max(row->depth_box->get_active_row_number(), 0)];
	return BotUI::cute_name(BotUI::bot_type(
		row->family_box->get_active_text(), depth, 0));
}


void new_game_win::on_seat_kind_changed(SeatRow *row)
{
	int kind = row->kind_box->get_active_row_number();

	// A seat switched to a computer starts out configured like the last
	// bot the user set up; suppress the name follower while the combos
	// catch up, so the name logic below sees the new family.
	if (kind == 1)
	{
		Prefs prefs;
		prefs.read();

		_loading_seat = true;
		row->family_box->set_active_text(prefs.bot_type.empty() ?
			BotUI::NEUTRAL : prefs.bot_type);
		row->depth_box->set_active_text(
			BotUI::depth_label(prefs.bot_depth));
		unsigned int level = prefs.bot_level;
		row->smarts_box->set_active_text(BotUI::skill_label(
			level >= 1 && level <= 4 ? level : 1));
		_loading_seat = false;
	}

	// Make the row's fields match the new kind before any text changes, so
	// a name set on a visible widget sticks.
	sync_seat_row(*row);

	if (kind == 0)
	{
		std::map<unsigned int, Glib::ustring>::const_iterator found =
			_human_names.find(row->id);
		Glib::ustring name = found != _human_names.end() ?
			found->second : _player_name;

		// The programmatic set is not a user edit; tracking it would clobber
		// the remembered name with the empty delete step.
		_loading_seat = true;
		row->name_entry->set_text(name);
		_loading_seat = false;
	}
	else if (kind == 1 && !row->name_entry->get_text().empty())
	{
		// Follow the cute default only while the name still is one.
		Glib::ustring current = row->name_entry->get_text();
		std::map<unsigned int, Glib::ustring>::const_iterator human =
			_human_names.find(row->id);
		std::map<unsigned int, Glib::ustring>::const_iterator def =
			_default_names.find(row->id);
		if ((human != _human_names.end() && current == human->second) ||
			def == _default_names.end() || current == def->second)
		{
			Glib::ustring name = default_name_for(row);
			row->name_entry->set_text(name);
			_default_names[row->id] = name;
		}
	}

	// A seat that used to be remote has no color; players need a real one.
	if (kind != 2 && row->color == 0)
	{
		unsigned int color = first_unused_color();
		row->color = color;
		row->color_picker->set_selected(color);
	}

	// A seat that used to be remote has no name either; computers get their
	// cute default.
	if (kind == 1 && row->name_entry->get_text().empty())
	{
		Glib::ustring name = default_name_for(row);
		row->name_entry->set_text(name);
		_default_names[row->id] = name;
	}

	sync_seat_row(*row);
	refresh_focus_rows();
}


void new_game_win::on_seat_family_changed(SeatRow *row)
{
	// Suppress the default-name follower while seats load, so a saved name
	// is taken as-is.
	if (_loading_seat)
		return;

	// The family (and depth) only shape the cute default of a computer seat.
	if (row->kind_box->get_active_row_number() != 1)
		return;

	Glib::ustring current = row->name_entry->get_text();
	if (_default_names.count(row->id) != 0 &&
		current == _default_names[row->id])
	{
		Glib::ustring name = default_name_for(row);
		row->name_entry->set_text(name);
		_default_names[row->id] = name;
	}

	refresh_focus_rows();
	remember_last_bot(row);
}


void new_game_win::on_seat_name_changed(SeatRow *row)
{
	// Programmatic text sets (loading, kind switches) are not user edits.
	if (_loading_seat)
		return;

	// A Human seat's name edits keep the remembered Human name current, so
	// any later kind switch (to computer or remote and back) restores the
	// last name the user typed for this seat.
	if (row->kind_box->get_active_row_number() == 0)
		_human_names[row->id] = row->name_entry->get_text();
}


void new_game_win::on_seat_depth_changed(SeatRow *row)
{
	on_seat_family_changed(row);
}


void new_game_win::on_seat_smarts_changed(SeatRow *row)
{
	if (_loading_seat)
		return;

	remember_last_bot(row);
}


// Treat this computer seat's settings as what the next bot starts from:
// the + button in this dialog and the in-game Add Computer Player both
// read them back from the saved preferences.
void new_game_win::remember_last_bot(SeatRow *row)
{
	if (_loading_seat || row->kind_box->get_active_row_number() != 1)
		return;

	Prefs prefs;
	prefs.read();

	prefs.bot_type = row->family_box->get_active_text();
	prefs.bot_level = std::max(row->smarts_box->get_active_row_number(), 0) + 1;
	prefs.bot_depth = BotUI::depth_values()[
		std::max(row->depth_box->get_active_row_number(), 0)];
	prefs.smarts = BotUI::skill_smarts(prefs.bot_level);

	prefs.write();
}


void new_game_win::on_seat_color_changed(unsigned int color, SeatRow *row)
{
	if (color < 1 || color > 8 || color == row->color)
		return;

	unsigned int previous = row->color;

	// Another seat holding the new color takes ours instead (matching the
	// iOS app's swap-on-duplicate).  set_selected() does not re-emit, so the
	// swap settles here.
	for (size_t i = 0; i < _rows.size(); i++)
		if (_rows[i] != row && _rows[i]->color == color)
		{
			_rows[i]->color = previous;
			_rows[i]->color_picker->set_selected(previous);
		}

	row->color = color;
}


// Seat row helpers ----------------------------------------------------------


void new_game_win::sync_seat_row(SeatRow &row)
{
	int kind = row.kind_box->get_active_row_number();

	row.family_box->set_visible(kind == 1);
	row.details->set_visible(kind == 1);
	// Only human seats are named here; computers get cute defaults from the
	// engine and remotes show up as open seats on other devices.
	row.name_entry->set_visible(kind == 0);
	// Remote seats are unnamed placeholders, so they get no color either.
	row.color_picker->set_visible(kind != 2);
}


void new_game_win::refresh_focus_rows()
{
	for (size_t i = 0; i < _rows.size(); i++)
	{
		SeatRow *row = _rows[i];

		// Remember what the old checkboxes said, keyed by the seat each one
		// pointed at (parallel to focus_checks), so a removed seat's state
		// is not passed on to the next one and a newly added seat defaults
		// to included: everything is a friend unless the user unchecked it.
		std::map<unsigned int, bool> was_checked;
		for (size_t k = 0; k < row->focus_checks.size() &&
				k < row->focus_check_ids.size(); k++)
			was_checked[row->focus_check_ids[k]] =
				row->focus_checks[k]->get_active();

		// Drop the old checkboxes.
		std::vector<Gtk::Widget *> children = row->focus_box->get_children();
		for (size_t j = 0; j < children.size(); j++)
		{
			row->focus_box->remove(*children[j]);
			delete children[j];
		}
		row->focus_checks.clear();
		row->focus_check_ids.clear();

		if (row->kind_box->get_active_row_number() != 1)
			continue;

		Glib::ustring family = row->family_box->get_active_text();
		if (family != BotUI::FRIENDLY && family != BotUI::MEAN)
			continue;

		// Plain-new, not make_managed: this label is deleted manually in the
		// removal loop above the next time this row is refreshed (a managed
		// wrapper would already be gone once remove() drops the box's ref).
		Gtk::Label *label = new class Gtk::Label(
			family == BotUI::FRIENDLY ? _("Friendly to:") : _("Mean to:"));
		row->focus_box->append(*label);

		for (size_t j = 0; j < _rows.size(); j++)
		{
			if (_rows[j] == row)
				continue;
			bool mine;
			if (row->focus_pending)
				mine = std::find(row->focus_pending_ids.begin(),
					row->focus_pending_ids.end(), (unsigned int) j) !=
					row->focus_pending_ids.end();
			else
			{
				std::map<unsigned int, bool>::iterator found =
					was_checked.find(_rows[j]->id);
				mine = (found == was_checked.end()) ? true : found->second;
			}
			Gtk::CheckButton *check = new class Gtk::CheckButton(
				other_label(_rows[j]));
			check->set_active(mine);
			row->focus_box->append(*check);
			row->focus_checks.push_back(check);
			row->focus_check_ids.push_back(_rows[j]->id);
		}
		// focus_pending stays set until load_seats() has added every row, so
		// indices that point at rows loaded later are not lost.
	}
}


Glib::ustring new_game_win::other_label(SeatRow *row)
{
	int kind = row->kind_box->get_active_row_number();
	if (kind == 0)
		return row->name_entry->get_text().empty() ?
			Glib::ustring(_("Seat")) : row->name_entry->get_text();
	if (kind == 1)
		return row->family_box->get_active_text();
	return _("Open Seat");
}


unsigned int new_game_win::first_unused_color() const
{
	for (unsigned int color = 1; color <= 8; color++)
	{
		bool used = false;
		for (size_t i = 0; i < _rows.size(); i++)
			if (_rows[i]->color == color)
			{
				used = true;
				break;
			}
		if (!used)
			return color;
	}
	return 1;
}


std::vector<NewGameSeat> new_game_win::collect_seats()
{
	std::vector<NewGameSeat> seats;

	for (size_t i = 0; i < _rows.size(); i++)
	{
		SeatRow *row = _rows[i];
		NewGameSeat seat;

		seat.id = row->id;
		seat.kind = std::max(row->kind_box->get_active_row_number(), 0);
		seat.name = row->name_entry->get_text();
		seat.color = row->color;
		seat.family = row->family_box->get_active_text();
		seat.level = std::max(row->smarts_box->get_active_row_number(), 0) + 1;
		seat.depth = BotUI::depth_values()[
			std::max(row->depth_box->get_active_row_number(), 0)];
		seat.focus_all = true;

		// A seat that used to be remote carries no color; players need a
		// real one or the server refuses the join.  Fix the row too so the
		// picker shows what will be used.
		if (seat.kind != 2 && (seat.color < 1 || seat.color > 8))
		{
			for (unsigned int c = 1; c <= 8; c++)
			{
				bool used = false;
				for (size_t j = 0; j < _rows.size(); j++)
					if (_rows[j] != row && _rows[j]->color == c)
						used = true;
				for (size_t j = 0; j < seats.size(); j++)
					if (seats[j].kind != 2 && seats[j].color == c)
						used = true;
				if (!used)
				{
					seat.color = c;
					row->color = c;
					row->color_picker->set_selected(c);
					break;
				}
			}
			if (seat.color < 1 || seat.color > 8)
				seat.color = 1;
		}

		// Hidden entries can hold empty names; players need real ones.
		if (seat.kind == 0 && seat.name.empty())
			seat.name = _player_name;
		else if (seat.kind == 1 && seat.name.empty())
		{
			seat.name = default_name_for(row);
			_default_names[row->id] = seat.name;
		}

		// A computer seat carries the name it had as a Human, so a seat
		// saved as a computer comes back with the player's own name when
		// switched back to Human.
		if (seat.kind == 1)
		{
			std::map<unsigned int, Glib::ustring>::const_iterator found =
				_human_names.find(row->id);
			seat.human_name = found != _human_names.end() ?
				found->second : Glib::ustring();
		}

		// A computer seat with a partially- or fully-unchecked focus row
		// focuses exactly the checked seats.  focus_check_ids holds the seat
		// id each checkbox stands for.
		if (seat.kind == 1)
		{
			bool all = true;
			std::vector<unsigned int> ids;
			for (size_t k = 0; k < row->focus_checks.size() &&
					k < row->focus_check_ids.size(); k++)
			{
				if (row->focus_checks[k]->get_active())
					ids.push_back(row->focus_check_ids[k]);
				else
					all = false;
			}
			seat.focus_all = all;
			seat.focus_ids = ids;
		}

		seats.push_back(seat);
	}

	return seats;
}


// New Game page actions -----------------------------------------------------


void new_game_win::on_host_cancel_button_clicked()
{
	hide();
}


void new_game_win::on_host_button_clicked()
{
	std::vector<NewGameSeat> seats = collect_seats();

	if (seats.size() < 2)
	{
		std::cerr << "A game needs at least two players." << std::endl;
		return;
	}

	if (host_port_entry->get_text().length() == 0)
		return;

	unsigned int host_port =
		util::from_str<unsigned int>(host_port_entry->get_text());
	unsigned int cheechweb_port = (cheechweb->get_active()) ?
		util::from_str<unsigned int>(cheechweb_port_entry->get_text()) : 0;
	bool lj = long_jumps->get_active();
	bool ho = hop_others->get_active();
	bool so = stop_others->get_active();

	if (signal_start_server(host_port, cheechweb_port, lj, ho, so, seats))
	{
		Prefs prefs;

		prefs.read();

		// Keep the profile of the first human seat for joining games later.
		for (size_t i = 0; i < seats.size(); i++)
			if (seats[i].kind == 0)
			{
				prefs.name = seats[i].name;
				prefs.color = seats[i].color;
				break;
			}

		prefs.host_port = host_port;
		prefs.start_cheechweb = (cheechweb_port > 0);
		if (prefs.start_cheechweb)
			prefs.cheechweb_port = cheechweb_port;
		prefs.num_players = seats.size();
		prefs.long_jumps = lj;
		prefs.hop_others = ho;
		prefs.stop_others = so;
		prefs.write();
		save_seats();
	}

	hide();
}


void new_game_win::on_cheechweb_toggled()
{
	cheechweb_port_entry->set_sensitive(cheechweb->get_active());
}


// Join page -----------------------------------------------------------------


void new_game_win::on_join_cancel_button_clicked()
{
	hide();
}


void new_game_win::on_join_host_icon_pressed(Gtk::Entry::IconPosition)
{
	const auto alloc = join_host_entry->get_allocation();
	join_host_list->set_size_request(alloc.get_width() - 12, -1);
	if (join_host_popup->get_visible())
		join_host_popup->popdown();
	else
		join_host_popup->popup();
}


void new_game_win::on_join_host_picked(Gtk::ListBoxRow *row)
{
	Gtk::Label *label = dynamic_cast<Gtk::Label *>(row->get_child());
	if (!label)
		return;
	join_host_entry->set_text(label->get_text());
	join_host_popup->popdown();
}


bool new_game_win::on_join_host_key_pressed(guint keyval, guint, Gdk::ModifierType)
{
	if (keyval == GDK_KEY_BackSpace || keyval == GDK_KEY_Delete)
	{
		on_join_host_delete_selected();
		return true;
	}
	return false;
}


void new_game_win::on_join_host_delete_selected()
{
	Gtk::ListBoxRow *row = join_host_list->get_selected_row();
	if (!row)
		return;

	Gtk::Label *label = dynamic_cast<Gtk::Label *>(row->get_child());
	if (!label)
		return;

	// Drop the host from the saved history so it doesn't reappear next
	// time, then remove the live row from the open popup.
	Prefs prefs;
	prefs.read();
	for (auto i = prefs.join_hostnames.begin();
		 i != prefs.join_hostnames.end(); i++)
	{
		if (*i == label->get_text())
		{
			prefs.join_hostnames.erase(i);
			break;
		}
	}
	prefs.write();

	join_host_list->remove(*row);
}


void new_game_win::on_join_button_clicked()
{
	if (join_host_entry->get_text().length() > 0 &&
		join_port_entry->get_text().length() > 0 &&
		name_entry->get_text().length() > 0)
	{
		Prefs prefs;

		prefs.read();

		prefs.name = name_entry->get_text();
		prefs.color = get_selected_color();
		prefs.spectator = spectator->get_active();

		Glib::ustring hostname;

		hostname = join_host_entry->get_text();

		prefs.join_port = util::from_str<unsigned int>(join_port_entry->get_text());
		prefs.write();

		signal_start_client(hostname, prefs.join_port,
			prefs.name, prefs.color, prefs.spectator);

		hide();
	}
}


unsigned int new_game_win::get_selected_color() const
{
	unsigned int color = color_box->get_selected();
	return (color >= 1 && color <= 8) ? color : 1;
}


void new_game_win::set_selected_color(unsigned int color)
{
	color_box->set_selected(color >= 1 && color <= 8 ? color : 1);
}


// Seat persistence ----------------------------------------------------------


void new_game_win::load_seats()
{
	Prefs prefs;

	prefs.read();

	if (prefs.seats.empty())
	{
		// Legacy preferences: one human plus one computer.
		NewGameSeat human;
		human.kind = 0;
		human.name = prefs.name;
		human.color = prefs.color;
		human.level = 1;
		human.depth = 0;
		human.focus_all = true;
		human.id = 0;
		add_seat(human);

		NewGameSeat computer;
		computer.kind = 1;
		computer.name = "";
		computer.color = first_unused_color();
		computer.family = prefs.bot_type.empty() ? BotUI::NEUTRAL : prefs.bot_type;
		computer.level = prefs.bot_level;
		computer.depth = prefs.bot_depth;
		computer.focus_all = true;
		computer.id = 0;
		add_seat(computer);

		return;
	}

	for (size_t i = 0; i < prefs.seats.size(); i++)
	{
		std::vector<Glib::ustring> fields = split_fields(prefs.seats[i]);
		NewGameSeat seat;
		seat.id = 0;
		seat.level = 1;
		seat.depth = 0;
		seat.focus_all = true;

		if (fields.empty())
			continue;
		if (fields[0] == "H" && fields.size() >= 3)
		{
			seat.kind = 0;
			seat.name = fields[1];
			seat.color = util::from_str<unsigned int>(fields[2]);
		}
		else if (fields[0] == "C" && fields.size() >= 7)
		{
			seat.kind = 1;
			seat.family = unescape_field(fields[1]);
			seat.level = util::from_str<unsigned int>(fields[2]);
			seat.depth = util::from_str<unsigned int>(fields[3]);
			seat.name = unescape_field(fields[4]);
			seat.color = util::from_str<unsigned int>(fields[5]);
			seat.focus_all = (fields[6] == "all");
			if (!seat.focus_all)
				seat.focus_ids = split_indices(fields[6]);
			if (fields.size() >= 8)
				seat.human_name = unescape_field(fields[7]);
		}
		else if (fields[0] == "R")
		{
			seat.kind = 2;
			seat.color = 0;
		}
		else
			continue;

		add_seat(seat);
	}

	// The last refresh_focus_rows() applied every pending focus selection
	// with the full seat list present; consume the flags now.
	for (size_t i = 0; i < _rows.size(); i++)
		_rows[i]->focus_pending = false;
}


void new_game_win::save_seats()
{
	Prefs prefs;

	prefs.read();

	prefs.seats.clear();

	std::vector<NewGameSeat> seats = collect_seats();
	for (size_t i = 0; i < seats.size(); i++)
	{
		const NewGameSeat &seat = seats[i];
		Glib::ustring line;

		if (seat.kind == 0)
			line = "H|" + escape_field(seat.name) + "|" +
				util::to_str(seat.color);
		else if (seat.kind == 1)
		{
			Glib::ustring focus = "all";
			if (!seat.focus_all)
			{
				// Focus seats are stored as 0-based seat indices.
				std::map<unsigned int, unsigned int> index_of;
				for (size_t j = 0; j < seats.size(); j++)
					index_of[seats[j].id] = j;
				Glib::ustring list;
				for (size_t j = 0; j < seat.focus_ids.size(); j++)
				{
					if (index_of.count(seat.focus_ids[j]) == 0)
						continue;
					if (!list.empty())
						list += ",";
					list += util::to_str(index_of[seat.focus_ids[j]]);
				}
				focus = list;
			}
			line = "C|" + escape_field(seat.family) + "|" +
				util::to_str(seat.level) + "|" + util::to_str(seat.depth) +
				"|" + escape_field(seat.name) + "|" +
				util::to_str(seat.color) + "|" + focus + "|" +
				escape_field(seat.human_name);
		}
		else
			line = "R";

		prefs.seats.push_back(line);
	}

	prefs.write();
}
