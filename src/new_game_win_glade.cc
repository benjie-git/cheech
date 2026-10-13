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
#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/stock.h>


new_game_win_glade::new_game_win_glade()
	: Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{
	// The seat rows are built and owned by new_game_win.
	seats_box = Gtk::manage(new class Gtk::VBox(false, 6));

	seats_scroll = Gtk::manage(new class Gtk::ScrolledWindow());
	seats_scroll->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	seats_scroll->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	seats_scroll->set_size_request(-1, 220);
	seats_scroll->add(*seats_box);

	Gtk::Alignment *players_align =
		Gtk::manage(new class Gtk::Alignment(0.5, 0.5, 1, 1));
	players_align->set_padding(6, 6, 6, 6);
	players_align->add(*seats_scroll);

	Gtk::Label *players_label =
		Gtk::manage(new class Gtk::Label(_("<b>Players</b>")));
	players_label->set_use_markup(true);
	Gtk::Frame *players_frame = Gtk::manage(new class Gtk::Frame());
	players_frame->set_label_widget(*players_label);
	players_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	players_frame->add(*players_align);

	add_seat_button = Gtk::manage(new class Gtk::Button(_("Add Seat")));
	remove_seat_button =
		Gtk::manage(new class Gtk::Button(_("Remove Seat")));
	Gtk::HButtonBox *seat_buttons = Gtk::manage(
		new class Gtk::HButtonBox(Gtk::BUTTONBOX_END, 6));
	seat_buttons->pack_start(*add_seat_button);
	seat_buttons->pack_start(*remove_seat_button);

	long_jumps = Gtk::manage(new class Gtk::CheckButton(
		_("Allow Long Jumps (Sue Wen Rules)")));
	hop_others = Gtk::manage(new class Gtk::CheckButton(
		_("Allow Hopping Through Other Players' Triangles")));
	stop_others = Gtk::manage(new class Gtk::CheckButton(
		_("Allow Stopping In Other Players' Triangles")));
	Gtk::VBox *rules_box = Gtk::manage(new class Gtk::VBox(false, 0));
	rules_box->set_border_width(6);
	rules_box->pack_start(*long_jumps, Gtk::PACK_SHRINK);
	rules_box->pack_start(*hop_others, Gtk::PACK_SHRINK);
	rules_box->pack_start(*stop_others, Gtk::PACK_SHRINK);

	Gtk::Label *rules_label =
		Gtk::manage(new class Gtk::Label(_("<b>Game Setup</b>")));
	rules_label->set_use_markup(true);
	Gtk::Frame *rules_frame = Gtk::manage(new class Gtk::Frame());
	rules_frame->set_label_widget(*rules_label);
	rules_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	rules_frame->add(*rules_box);

	Gtk::Label *port_label =
		Gtk::manage(new class Gtk::Label(_("Port Number:")));
	host_port_entry = Gtk::manage(new class Gtk::Entry());
	host_port_entry->set_text(_("3838"));
	host_port_entry->set_max_width_chars(8);
	cheechweb = Gtk::manage(new class Gtk::CheckButton(
		_("Also Start a CheechWeb server")));
	Gtk::Label *web_port_label = Gtk::manage(
		new class Gtk::Label(_("CheechWeb Port Number:")));
	cheechweb_port_entry = Gtk::manage(new class Gtk::Entry());
	cheechweb_port_entry->set_text(_("3839"));
	cheechweb_port_entry->set_max_width_chars(8);
	Gtk::Table *network_table =
		Gtk::manage(new class Gtk::Table(2, 3, false));
	network_table->set_border_width(6);
	network_table->set_row_spacings(3);
	network_table->set_col_spacings(3);
	network_table->attach(*port_label, 0, 1, 0, 1, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	network_table->attach(*host_port_entry, 1, 2, 0, 1,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	network_table->attach(*cheechweb, 0, 2, 1, 2, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	network_table->attach(*web_port_label, 0, 1, 2, 3, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	network_table->attach(*cheechweb_port_entry, 1, 2, 2, 3,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);

	Gtk::Label *network_label =
		Gtk::manage(new class Gtk::Label(_("<b>CheechWeb</b>")));
	network_label->set_use_markup(true);
	Gtk::Frame *network_frame = Gtk::manage(new class Gtk::Frame());
	network_frame->set_label_widget(*network_label);
	network_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	network_frame->add(*network_table);

	host_cancel_button = Gtk::manage(new class Gtk::Button(Gtk::Stock::CANCEL));
	host_button = Gtk::manage(new class Gtk::Button(_("Start Game")));
	host_button->set_can_default(true);
	Gtk::HButtonBox *host_buttons = Gtk::manage(
		new class Gtk::HButtonBox(Gtk::BUTTONBOX_END, 6));
	host_buttons->set_border_width(6);
	host_buttons->pack_start(*host_cancel_button);
	host_buttons->pack_start(*host_button);

	Gtk::VBox *new_page = Gtk::manage(new class Gtk::VBox(false, 6));
	new_page->set_border_width(6);
	new_page->pack_start(*players_frame);
	new_page->pack_start(*seat_buttons, Gtk::PACK_SHRINK);
	new_page->pack_start(*rules_frame, Gtk::PACK_SHRINK);
	new_page->pack_start(*network_frame, Gtk::PACK_SHRINK);
	new_page->pack_start(*host_buttons, Gtk::PACK_SHRINK);

	// The Join page.
	Gtk::Label *name_label =
		Gtk::manage(new class Gtk::Label(_("Name:")));
	name_entry = Gtk::manage(new class Gtk::Entry());
	Gtk::Label *color_label =
		Gtk::manage(new class Gtk::Label(_("Color:")));
	color_box = Gtk::manage(new class Gtk::ComboBoxText());
	spectator = Gtk::manage(new class Gtk::CheckButton(
		_("Join As A Spectator (Not Playing)")));
	Gtk::Table *profile_table =
		Gtk::manage(new class Gtk::Table(2, 3, false));
	profile_table->set_border_width(6);
	profile_table->set_row_spacings(3);
	profile_table->set_col_spacings(3);
	profile_table->attach(*name_label, 0, 1, 0, 1, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	profile_table->attach(*name_entry, 1, 2, 0, 1,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	profile_table->attach(*color_label, 0, 1, 1, 2, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	profile_table->attach(*color_box, 1, 2, 1, 2,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	profile_table->attach(*spectator, 0, 2, 2, 3, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);

	Gtk::Label *profile_label =
		Gtk::manage(new class Gtk::Label(_("<b>Your Player</b>")));
	profile_label->set_use_markup(true);
	Gtk::Frame *profile_frame = Gtk::manage(new class Gtk::Frame());
	profile_frame->set_label_widget(*profile_label);
	profile_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	profile_frame->add(*profile_table);

	Gtk::Label *host_label =
		Gtk::manage(new class Gtk::Label(_("Host Name:")));
	join_host_entry = new class GtkComboBoxEntryText(true);
	Gtk::Label *join_port_label =
		Gtk::manage(new class Gtk::Label(_("Port Number:")));
	join_port_entry = Gtk::manage(new class Gtk::Entry());
	join_port_entry->set_text(_("3838"));
	join_port_entry->set_max_width_chars(8);
	Gtk::Table *join_table = Gtk::manage(new class Gtk::Table(2, 2, false));
	join_table->set_border_width(6);
	join_table->set_row_spacings(3);
	join_table->set_col_spacings(3);
	join_table->attach(*host_label, 0, 1, 0, 1, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	join_table->attach(*join_host_entry, 1, 2, 0, 1,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);
	join_table->attach(*join_port_label, 0, 1, 1, 2, Gtk::FILL,
		Gtk::AttachOptions(), 0, 0);
	join_table->attach(*join_port_entry, 1, 2, 1, 2,
		Gtk::EXPAND|Gtk::FILL, Gtk::AttachOptions(), 0, 0);

	Gtk::Label *server_label =
		Gtk::manage(new class Gtk::Label(_("<b>Server</b>")));
	server_label->set_use_markup(true);
	Gtk::Frame *server_frame = Gtk::manage(new class Gtk::Frame());
	server_frame->set_label_widget(*server_label);
	server_frame->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	server_frame->add(*join_table);

	join_cancel_button =
		Gtk::manage(new class Gtk::Button(Gtk::Stock::CANCEL));
	join_button = Gtk::manage(new class Gtk::Button(_("Join Game")));
	join_button->set_can_default(true);
	Gtk::HButtonBox *join_buttons = Gtk::manage(
		new class Gtk::HButtonBox(Gtk::BUTTONBOX_END, 6));
	join_buttons->set_border_width(6);
	join_buttons->pack_start(*join_cancel_button);
	join_buttons->pack_start(*join_button);

	Gtk::VBox *join_page = Gtk::manage(new class Gtk::VBox(false, 6));
	join_page->set_border_width(6);
	join_page->pack_start(*profile_frame, Gtk::PACK_SHRINK);
	join_page->pack_start(*server_frame, Gtk::PACK_SHRINK);
	join_page->pack_start(*join_buttons, Gtk::PACK_SHRINK);

	role_notebook = Gtk::manage(new class Gtk::Notebook());
	role_notebook->append_page(*new_page, _("New Game"));
	role_notebook->append_page(*join_page, _("Join An Existing Board"));

	Gtk::VBox *top_box = Gtk::manage(new class Gtk::VBox(false, 0));
	top_box->set_border_width(6);
	top_box->pack_start(*role_notebook);

	add(*top_box);

	set_title(_("Host Or Join A Board"));
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
	host_button->grab_default();
	name_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));
	join_port_entry->signal_activate().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));
	join_cancel_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_cancel_button_clicked));
	join_button->signal_clicked().connect(sigc::mem_fun(*this,
		&new_game_win_glade::on_join_button_clicked));

	// The old glade-generated code showed each widget individually; show
	// everything now that the window is built.
	show_all_children();
}


new_game_win_glade::~new_game_win_glade()
{
	delete join_host_entry;
}
