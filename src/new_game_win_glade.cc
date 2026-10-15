//
//    The New/Join Game window layout.  Hand-maintained here because the
//    .glade sources are not in the repository.
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

#include "config.h"
#include "new_game_win_glade.hh"
#include <glibmm/i18n.h>
#include <gtkmm/label.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>


new_game_win_glade::new_game_win_glade()
	: Gtk::Window()
{
	// The seat rows are built and owned by new_game_win.
	seats_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);

	seats_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
	seats_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
	seats_scroll->set_size_request(-1, 220);
	seats_scroll->set_child(*seats_box);

	Gtk::Label *players_label =
		Gtk::make_managed<Gtk::Label>(_("<b>Players</b>"));
	players_label->set_use_markup(true);
	Gtk::Frame *players_frame = Gtk::make_managed<Gtk::Frame>();
	players_frame->set_label_widget(*players_label);
	players_frame->set_child(*seats_scroll);
	// The Players Setup area is what should grow when the dialog grows;
	// the scrolled seat list inside it absorbs the extra height.
	players_frame->set_vexpand(true);

	add_seat_button = Gtk::make_managed<Gtk::Button>(_("Add Seat"));
	remove_seat_button =
		Gtk::make_managed<Gtk::Button>(_("Remove Seat"));
	Gtk::Box *seat_buttons = Gtk::make_managed<Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	seat_buttons->set_halign(Gtk::Align::END);
	seat_buttons->append(*add_seat_button);
	seat_buttons->append(*remove_seat_button);

	long_jumps = Gtk::make_managed<Gtk::CheckButton>(
		_("Allow Long Jumps (Sue Wen Rules)"));
	hop_others = Gtk::make_managed<Gtk::CheckButton>(
		_("Allow Hopping Through Other Players' Triangles"));
	stop_others = Gtk::make_managed<Gtk::CheckButton>(
		_("Allow Stopping In Other Players' Triangles"));
	Gtk::Box *rules_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
	rules_box->set_margin_top(6);
	rules_box->set_margin_bottom(6);
	rules_box->set_margin_start(6);
	rules_box->set_margin_end(6);
	rules_box->append(*long_jumps);
	rules_box->append(*hop_others);
	rules_box->append(*stop_others);

	Gtk::Label *speed_label =
		Gtk::make_managed<Gtk::Label>(_("Animation Speed:"));
	speed_label->set_halign(Gtk::Align::START);
	Glib::RefPtr<Gtk::Adjustment> move_scale_adj =
		Gtk::Adjustment::create(250, 0, 500, 10, 50, 0);
	move_scale = Gtk::make_managed<Gtk::Scale>(move_scale_adj,
		Gtk::Orientation::HORIZONTAL);
	move_scale->set_digits(0);
	move_scale->set_draw_value(true);
	move_scale->set_value_pos(Gtk::PositionType::RIGHT);
	move_scale->set_hexpand(true);
	move_scale->set_valign(Gtk::Align::CENTER);
	move_scale->set_format_value_func(
		[](double value) -> Glib::ustring {
			if (value < 1)
				return _("Instant");
			return Glib::ustring::format((int)(value + 0.5)) + " ms";
		});
	Gtk::Box *speed_box = Gtk::make_managed<Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 12);
	speed_box->set_margin_top(6);
	speed_box->append(*speed_label);
	speed_box->append(*move_scale);
	rules_box->append(*speed_box);

	Gtk::Label *rules_label =
		Gtk::make_managed<Gtk::Label>(_("<b>Game Setup</b>"));
	rules_label->set_use_markup(true);
	Gtk::Frame *rules_frame = Gtk::make_managed<Gtk::Frame>();
	rules_frame->set_label_widget(*rules_label);
	rules_frame->set_child(*rules_box);

	Gtk::Label *port_label =
		Gtk::make_managed<Gtk::Label>(_("Port Number:"));
	host_port_entry = Gtk::make_managed<Gtk::Entry>();
	host_port_entry->set_text(_("3838"));
	host_port_entry->set_max_width_chars(8);
	cheechweb = Gtk::make_managed<Gtk::CheckButton>(
		_("Also Start a CheechWeb server"));
	Gtk::Label *web_port_label = Gtk::make_managed<Gtk::Label>(
		_("CheechWeb Port Number:"));
	cheechweb_port_entry = Gtk::make_managed<Gtk::Entry>();
	cheechweb_port_entry->set_text(_("3839"));
	cheechweb_port_entry->set_max_width_chars(8);
	Gtk::Grid *network_table = Gtk::make_managed<Gtk::Grid>();
	network_table->set_margin_top(6);
	network_table->set_margin_bottom(6);
	network_table->set_margin_start(6);
	network_table->set_margin_end(6);
	network_table->set_row_spacing(3);
	network_table->set_column_spacing(3);
	network_table->attach(*port_label, 0, 0);
	network_table->attach(*host_port_entry, 1, 0);
	network_table->attach(*cheechweb, 0, 1, 2, 1);
	network_table->attach(*web_port_label, 0, 2);
	network_table->attach(*cheechweb_port_entry, 1, 2);

	Gtk::Expander *network_expander = Gtk::make_managed<Gtk::Expander>();
	// "frame" is the style class Gtk::Frame uses for its rounded border,
	// so this matches the Game Setup frame's look.
	network_expander->add_css_class("frame");
	Gtk::Label *network_label =
		Gtk::make_managed<Gtk::Label>(_("<b>Server Setup</b>"));
	network_label->set_use_markup(true);
	network_label->set_margin_top(4);
	network_label->set_margin_bottom(4);
	network_label->set_margin_start(6);
	network_label->set_margin_end(6);
	network_expander->set_label_widget(*network_label);
	network_expander->set_child(*network_table);

	host_cancel_button = Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
	host_button = Gtk::make_managed<Gtk::Button>(_("Start Game"));
	Gtk::Box *host_buttons = Gtk::make_managed<Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	host_buttons->set_halign(Gtk::Align::END);
	host_buttons->set_margin_top(6);
	host_buttons->set_margin_bottom(6);
	host_buttons->set_margin_start(6);
	host_buttons->set_margin_end(6);
	host_buttons->append(*host_cancel_button);
	host_buttons->append(*host_button);

	Gtk::Box *new_page = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
	new_page->set_margin_top(6);
	new_page->set_margin_bottom(6);
	new_page->set_margin_start(6);
	new_page->set_margin_end(6);
	new_page->append(*players_frame);
	new_page->append(*seat_buttons);
	new_page->append(*rules_frame);
	new_page->append(*network_expander);
	new_page->append(*host_buttons);

	// The Join page.
	Gtk::Label *name_label =
		Gtk::make_managed<Gtk::Label>(_("Name:"));
	name_entry = Gtk::make_managed<Gtk::Entry>();
	Gtk::Label *color_label =
		Gtk::make_managed<Gtk::Label>(_("Color:"));
	color_box = Gtk::make_managed<ColorPicker>();
	spectator = Gtk::make_managed<Gtk::CheckButton>(
		_("Join As A Spectator (Not Playing)"));
	Gtk::Grid *profile_table = Gtk::make_managed<Gtk::Grid>();
	profile_table->set_margin_top(6);
	profile_table->set_margin_bottom(6);
	profile_table->set_margin_start(6);
	profile_table->set_margin_end(6);
	profile_table->set_row_spacing(3);
	profile_table->set_column_spacing(3);
	profile_table->attach(*name_label, 0, 0);
	profile_table->attach(*name_entry, 1, 0);
	profile_table->attach(*color_label, 0, 1);
	profile_table->attach(*color_box, 1, 1);
	profile_table->attach(*spectator, 0, 2, 2, 1);
	name_entry->set_hexpand(true);

	Gtk::Label *profile_label =
		Gtk::make_managed<Gtk::Label>(_("<b>Your Player</b>"));
	profile_label->set_use_markup(true);
	Gtk::Frame *profile_frame = Gtk::make_managed<Gtk::Frame>();
	profile_frame->set_label_widget(*profile_label);
	profile_frame->set_child(*profile_table);

	Gtk::Label *host_label =
		Gtk::make_managed<Gtk::Label>(_("Host Name:"));
	join_host_entry = Gtk::make_managed<Gtk::Entry>();
	join_host_entry->set_icon_from_icon_name("pan-down-symbolic",
		Gtk::Entry::IconPosition::SECONDARY);
	join_host_popup = Gtk::make_managed<Gtk::Popover>();
	join_host_popup->set_parent(*join_host_entry);
	join_host_popup->set_has_arrow(false);
	join_host_popup->set_position(Gtk::PositionType::BOTTOM);
	join_host_list = Gtk::make_managed<Gtk::ListBox>();
	join_host_list->set_margin(6);
	Gtk::ScrolledWindow *join_host_scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
	join_host_scroll->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
	join_host_scroll->set_max_content_height(5 * 36);
	join_host_scroll->set_propagate_natural_height(true);
	join_host_scroll->set_child(*join_host_list);
	join_host_popup->set_child(*join_host_scroll);
	Gtk::Label *join_port_label =
		Gtk::make_managed<Gtk::Label>(_("Port Number:"));
	join_port_entry = Gtk::make_managed<Gtk::Entry>();
	join_port_entry->set_text(_("3838"));
	join_port_entry->set_max_width_chars(8);
	Gtk::Grid *join_table = Gtk::make_managed<Gtk::Grid>();
	join_table->set_margin_top(6);
	join_table->set_margin_bottom(6);
	join_table->set_margin_start(6);
	join_table->set_margin_end(6);
	join_table->set_row_spacing(3);
	join_table->set_column_spacing(3);
	join_table->attach(*host_label, 0, 0);
	join_table->attach(*join_host_entry, 1, 0);
	join_table->attach(*join_port_label, 0, 1);
	join_table->attach(*join_port_entry, 1, 1);
	join_host_entry->set_hexpand(true);
	join_port_entry->set_hexpand(true);

	Gtk::Label *server_label =
		Gtk::make_managed<Gtk::Label>(_("<b>Server</b>"));
	server_label->set_use_markup(true);
	Gtk::Frame *server_frame = Gtk::make_managed<Gtk::Frame>();
	server_frame->set_label_widget(*server_label);
	server_frame->set_child(*join_table);

	join_cancel_button =
		Gtk::make_managed<Gtk::Button>(_("_Cancel"), true);
	join_button = Gtk::make_managed<Gtk::Button>(_("Join Game"));
	Gtk::Box *join_buttons = Gtk::make_managed<Gtk::Box>(
		Gtk::Orientation::HORIZONTAL, 6);
	join_buttons->set_halign(Gtk::Align::END);
	join_buttons->set_margin_top(6);
	join_buttons->set_margin_bottom(6);
	join_buttons->set_margin_start(6);
	join_buttons->set_margin_end(6);
	join_buttons->append(*join_cancel_button);
	join_buttons->append(*join_button);

	Gtk::Box *join_page = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);
	join_page->set_margin_top(6);
	join_page->set_margin_bottom(6);
	join_page->set_margin_start(6);
	join_page->set_margin_end(6);
	join_page->append(*profile_frame);
	join_page->append(*server_frame);
	join_page->append(*join_buttons);

	role_notebook = Gtk::make_managed<Gtk::Notebook>();
	role_notebook->append_page(*new_page, _("New Game"));
	role_notebook->append_page(*join_page, _("Join An Existing Board"));

	Gtk::Box *top_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
	top_box->set_margin_top(6);
	top_box->set_margin_bottom(6);
	top_box->set_margin_start(6);
	top_box->set_margin_end(6);
	// GTK4 GtkBox only gives extra main-axis space to children with
	// expand set; without this the notebook stays at its minimum height.
	role_notebook->set_vexpand(true);
	top_box->append(*role_notebook);

	set_child(*top_box);

	set_title(_("New Game"));
	set_resizable(true);

	// Signal wiring.
	add_seat_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_add_seat_button_clicked));
	remove_seat_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_remove_seat_button_clicked));
	host_port_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_host_button_clicked));
	cheechweb->signal_toggled().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_cheechweb_toggled));
	cheechweb_port_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_host_button_clicked));
	host_cancel_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_host_cancel_button_clicked));
	host_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_host_button_clicked));
	set_default_widget(*host_button);
	name_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));
	join_port_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));
	join_cancel_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_cancel_button_clicked));
	join_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));
}


new_game_win_glade::~new_game_win_glade()
{
}
