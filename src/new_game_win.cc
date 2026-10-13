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
#include <glibmm/i18n.h>
#include "utility.hh"


namespace
{
	// The colors offered per seat, in GameHole color order 1..8.
	const char *color_names[] = {
		"Red", "Orange", "Yellow", "Green",
		"Blue", "Purple", "Black", "White"
	};

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
}


new_game_win::new_game_win() : new_game_win_glade()
{
	Prefs prefs;

	prefs.read();

	_player_name = prefs.name;
	_next_seat_id = 1;
	_loading_seat = false;

	for (unsigned int i = 0; i < 8; i++)
		color_box->append(color_names[i]);

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

	for (std::vector<Glib::ustring>::iterator i = prefs.join_hostnames.begin();
		 i != prefs.join_hostnames.end(); i++)
			join_host_entry->append(*i);
	if (prefs.join_hostnames.size() > 0)
		join_host_entry->set_active_text(prefs.join_hostnames.front());

	join_port_entry->set_text(util::to_str<unsigned int>(prefs.join_port));
	spectator->set_active(prefs.spectator);

	join_host_entry->get_entry()->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win::on_join_button_clicked));
	join_host_entry->signal_changed().connect(
		sigc::mem_fun(*(join_host_entry->get_entry()), &Widget::grab_focus));
	join_host_entry->set_focus_on_click(false);

	// Remember the seat layout across games and app runs, even when the
	// dialog is closed without starting a game.
	signal_hide().connect(sigc::mem_fun(*this, &new_game_win::save_seats));
}


void new_game_win::show_join_page(bool joining)
{
	if (joining)
	{
		role_notebook->set_current_page(1);
		join_button->grab_default();
	}
	else
	{
		role_notebook->set_current_page(0);
		host_button->grab_default();
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
	row->box = new Gtk::VBox(false, 3);
	row->box->set_border_width(3);

	row->kind_box = Gtk::manage(new class Gtk::ComboBoxText());
	row->kind_box->append(_("Human"));
	row->kind_box->append(_("Computer"));
	row->kind_box->append(_("Remote"));
	row->kind_box->set_active(seat.kind);

	row->family_box = Gtk::manage(new class Gtk::ComboBoxText());
	row->family_box->append(BotUI::FRIENDLY);
	row->family_box->append(BotUI::NEUTRAL);
	row->family_box->append(BotUI::MEAN);

	row->name_entry = Gtk::manage(new class Gtk::Entry());
	row->name_entry->set_text(seat.name);
	row->name_entry->set_max_width_chars(12);

	row->color_box = Gtk::manage(new class Gtk::ComboBoxText());
	for (unsigned int i = 0; i < 8; i++)
		row->color_box->append(color_names[i]);
	row->color_box->set_active(row->color >= 1 && row->color <= 8 ?
		row->color - 1 : 0);

	Gtk::HBox *top_box = Gtk::manage(new class Gtk::HBox(false, 6));
	top_box->pack_start(*row->kind_box, Gtk::PACK_SHRINK);
	top_box->pack_start(*row->family_box, Gtk::PACK_SHRINK);
	top_box->pack_start(*row->name_entry, Gtk::PACK_SHRINK);
	top_box->pack_start(*row->color_box, Gtk::PACK_SHRINK);

	Gtk::HBox *depth_box = Gtk::manage(new class Gtk::HBox(false, 6));
	depth_box->pack_start(*Gtk::manage(new class Gtk::Label(_("Depth:"))),
		Gtk::PACK_SHRINK);
	row->depth_box = Gtk::manage(new class Gtk::ComboBoxText());
	for (size_t i = 0; i < BotUI::depth_values().size(); i++)
		row->depth_box->append(
			BotUI::depth_label(BotUI::depth_values()[i]));
	depth_box->pack_start(*row->depth_box, Gtk::PACK_SHRINK);

	Gtk::HBox *smarts_box = Gtk::manage(new class Gtk::HBox(false, 6));
	smarts_box->pack_start(*Gtk::manage(new class Gtk::Label(_("Smarts:"))),
		Gtk::PACK_SHRINK);
	row->smarts_box = Gtk::manage(new class Gtk::ComboBoxText());
	for (size_t i = 0; i < BotUI::skill_levels().size(); i++)
		row->smarts_box->append(
			BotUI::skill_label(BotUI::skill_levels()[i]));
	smarts_box->pack_start(*row->smarts_box, Gtk::PACK_SHRINK);

	row->focus_box = Gtk::manage(new class Gtk::HBox(false, 6));

	Gtk::VBox *details_box = Gtk::manage(new class Gtk::VBox(false, 3));
	details_box->set_border_width(3);
	details_box->pack_start(*depth_box, Gtk::PACK_SHRINK);
	details_box->pack_start(*smarts_box, Gtk::PACK_SHRINK);
	details_box->pack_start(*row->focus_box, Gtk::PACK_SHRINK);

	row->details = Gtk::manage(new class Gtk::Expander(_("Details")));
	row->details->add(*details_box);

	row->box->pack_start(*top_box, Gtk::PACK_SHRINK);
	row->box->pack_start(*row->details, Gtk::PACK_SHRINK);

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
	row->color_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_color_changed), row));
	row->depth_box->signal_changed().connect(
		sigc::bind(sigc::mem_fun(*this,
			&new_game_win::on_seat_depth_changed), row));

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
	_human_names[row->id] = (seat.kind == 0 && !seat.name.empty()) ?
		seat.name : _player_name;

	seats_box->pack_start(*row->box, Gtk::PACK_SHRINK);
	// Show only this row's subtree: show_all() on the whole seats_box would
	// re-show widgets that sync_seat_row hid on earlier rows.
	row->box->show_all();
	_rows.push_back(row);

	sync_seat_row(*row);
	refresh_focus_rows();
}


void new_game_win::remove_seat(size_t index)
{
	if (index >= _rows.size() || _rows.size() <= 2)
		return;

	SeatRow *row = _rows[index];
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

	seat.kind = 0;
	seat.name = "";
	seat.color = first_unused_color();
	seat.family = BotUI::NEUTRAL;
	seat.level = 1;
	seat.depth = 0;
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
	int old_kind = GPOINTER_TO_INT(row->kind_box->get_data("prev-kind"));

	if (old_kind == 0)
		_human_names[row->id] = row->name_entry->get_text();

	if (kind == 0)
		row->name_entry->set_text(_human_names.count(row->id) ?
			_human_names[row->id] : _player_name);
	else if (kind == 1 && !row->name_entry->get_text().empty())
	{
		// Follow the cute default only while the name still is one.
		Glib::ustring current = row->name_entry->get_text();
		if (current == _human_names[row->id] ||
			_default_names.count(row->id) == 0 ||
			current == _default_names[row->id])
		{
			Glib::ustring name = default_name_for(row);
			row->name_entry->set_text(name);
			_default_names[row->id] = name;
		}
	}

	row->kind_box->set_data("prev-kind", GINT_TO_POINTER(kind));

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
}


void new_game_win::on_seat_name_changed(SeatRow *row)
{
	// Nothing to do; the name is read when the game starts.
	(void)row;
}


void new_game_win::on_seat_depth_changed(SeatRow *row)
{
	on_seat_family_changed(row);
}


void new_game_win::on_seat_color_changed(SeatRow *row)
{
	int index = row->color_box->get_active_row_number();
	if (index < 0)
		return;
	unsigned int color = index + 1;
	if (color == row->color)
		return;

	unsigned int previous = row->color;

	// Another seat holding the new color takes ours instead (matching the
	// iOS app's swap-on-duplicate).
	for (size_t i = 0; i < _rows.size(); i++)
		if (_rows[i] != row && _rows[i]->color == color)
		{
			_rows[i]->color = previous;
			_rows[i]->color_box->set_active(previous - 1);
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

		Gtk::Label *label = Gtk::manage(new class Gtk::Label(
			family == BotUI::FRIENDLY ? _("Friendly to:") : _("Mean to:")));
		row->focus_box->pack_start(*label, Gtk::PACK_SHRINK);

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
			row->focus_box->pack_start(*check, Gtk::PACK_SHRINK);
			row->focus_checks.push_back(check);
			row->focus_check_ids.push_back(_rows[j]->id);
		}
		row->focus_box->show_all();
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


std::vector<NewGameSeat> new_game_win::collect_seats() const
{
	std::vector<NewGameSeat> seats;

	for (size_t i = 0; i < _rows.size(); i++)
	{
		const SeatRow *row = _rows[i];
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


void new_game_win::on_join_button_clicked()
{
	if (join_host_entry->get_entry()->get_text().length() > 0 &&
		join_port_entry->get_text().length() > 0 &&
		name_entry->get_text().length() > 0)
	{
		Prefs prefs;

		prefs.read();

		prefs.name = name_entry->get_text();
		prefs.color = get_selected_color();
		prefs.spectator = spectator->get_active();

		Glib::ustring hostname;

		if (join_host_entry->get_active_text() != "")
			hostname = join_host_entry->get_active_text();
		else
			hostname = join_host_entry->get_entry()->get_text();

		prefs.join_port = util::from_str<unsigned int>(join_port_entry->get_text());
		prefs.write();

		signal_start_client(hostname, prefs.join_port,
			prefs.name, prefs.color, prefs.spectator);

		hide();
	}
}


unsigned int new_game_win::get_selected_color() const
{
	int index = color_box->get_active_row_number();
	return (index >= 0 && index < 8) ? index + 1 : 1;
}


void new_game_win::set_selected_color(unsigned int color)
{
	color_box->set_active(color >= 1 && color <= 8 ? color - 1 : 0);
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
			seat.family = fields[1];
			seat.level = util::from_str<unsigned int>(fields[2]);
			seat.depth = util::from_str<unsigned int>(fields[3]);
			seat.name = fields[4];
			seat.color = util::from_str<unsigned int>(fields[5]);
			seat.focus_all = (fields[6] == "all");
			if (!seat.focus_all)
				seat.focus_ids = split_indices(fields[6]);
		}
		else if (fields[0] == "R")
			seat.kind = 2;
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
				util::to_str(seat.color) + "|" + focus;
		}
		else
			line = "R";

		prefs.seats.push_back(line);
	}

	prefs.write();
}
