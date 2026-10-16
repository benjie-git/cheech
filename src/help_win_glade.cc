// generated 2006/6/6 16:06:08 PDT by benjie@paddy.(none)
// Copyright (C) 2005-2026 Ben Levitt
// SPDX-License-Identifier: MIT
//
// using glademm V2.6.0
//
// converted to gtkmm 4
//
// Please modify the corresponding derived classes in ./src/help_win.cc

#include "config.h"
/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (GETTEXT_PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif
#include "help_win_glade.hh"
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/box.h>

help_win_glade::help_win_glade(
) : Gtk::Window()
{
   Gtk::Window *help_win = this;

   Gtk::Label *label73 = Gtk::make_managed<Gtk::Label>(_("<big><b>Using Cheech:</b></big>\n"
		"\n"
		"To start a new game, Choose the 'Host New Board' menu item and have your friends connect to you.  Or use the 'Join a Board' menu item if someone else is already hosting a cheech board.\n"
		"\n"
		"If your friends are busy, Host a New Board, and add computer players into the game using the 'Setup Computer Player...' and 'Add Computer Player' menu items.  Watch out, they're not super smart, but they are thorough!\n"
		"\n"
		"To make a move, click on the peg you want to move, then on the empty hole to move your peg to.  If you're making a multiple-hop move, you need to click on each hole that you're hopping through along your chain of hops.  When you've finished clicking out your hopping path, double click the last hole, or press Enter to make your move.\n"
		"\n"
		"To allow friends to connect to your hosted cheech game using a web browser, enable cheechweb in the 'New Game' dialog, and have your friends connect to this computer's IP address, using the port you choose. \n"
		"(like this:  http://<b>your.ip.address.here</b>:3839/ )\n"
		"\n"
		"\n"
		"<big><b>Rules:</b></big>\n"
		"\n"
		"The object of the game is to get all 10 of your pieces from your starting triangle into the triangle opposite.  Players take turns moving one piece at a time.  On each move you can:\n"
		"\n"
		" * move a piece to any adjacent position (which must be empty)\n"
		" * hop a piece over an adjacent piece into the next position in the same direction (which must be empty)\n"
		" * chain together multiple hops\n"
		"\n"
		"\n"
		"<big><b>Variations:</b></big>\n"
		"\n"
		"cheech currently supports 3 rule variations.\n"
		"\n"
		" * <b>Long Jumps</b>: with this rule variation enabled, each hop can be longer.  Normally, hops always move from one position, over an adjacent position to the next position in the same direction (which must be empty).  With Long Jumps, you can hop from one position, over any number (0 or more) of empty positions, followed by a piece, followed by the same number of empty positions again, and into the next position (which must be empty).  Multiple hops of different-lengths can be chained together in one move.\n"
		"\n"
		" * <b>Hopping Through Other Players' Triangles</b>: If this is disabled, then you're not allowed to end a move in, or pass through any other player's starting or ending triangle.\n"
		"\n"
		" * <b>Stopping In Other Players' Triangles</b>: If this is disabled, then you're not allowed to end a move in any other player's starting or ending triangle.\n"
		"\n"
		"For a more detailed discussion of the rules (including diagrams!), check out the Wikipedia entry on Chinese Checkers.\n"
		""));
   Gtk::ScrolledWindow *scrolledwindow2 = Gtk::make_managed<Gtk::ScrolledWindow>();
   Gtk::Button *button1 = Gtk::make_managed<Gtk::Button>(_("_Close"), true);
   Gtk::Box *hbuttonbox8 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
   Gtk::Box *vbox11 = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 0);
   label73->set_justify(Gtk::Justification::LEFT);
   label73->set_wrap(true);
   label73->set_use_markup(true);
   scrolledwindow2->set_size_request(428,360);
   scrolledwindow2->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
   scrolledwindow2->set_child(*label73);
   hbuttonbox8->set_margin_top(6);
   hbuttonbox8->set_margin_bottom(6);
   hbuttonbox8->set_margin_start(6);
   hbuttonbox8->set_margin_end(6);
   hbuttonbox8->set_halign(Gtk::Align::END);
   hbuttonbox8->append(*button1);
   vbox11->append(*scrolledwindow2);
   vbox11->append(*hbuttonbox8);
   help_win->set_title(_("How to Play Chinese Checkers"));
   help_win->set_resizable(true);
   help_win->set_child(*vbox11);
   button1->signal_clicked().connect(sigc::mem_fun(*this, &help_win_glade::on_close_button_clicked), false);
}

help_win_glade::~help_win_glade()
{
}
