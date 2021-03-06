// generated 2006/6/6 16:06:08 PDT by benjie@paddy.(none)
// using glademm V2.6.0
//
// DO NOT EDIT THIS FILE ! It was created using
// glade-- /home/benjie/projects/cheech/cheech.glade
// for gtk 2.8.6 and gtkmm 2.8.0
//
// Please modify the corresponding derived classes in ./src/help_win.cc


#if defined __GNUC__ && __GNUC__ < 3
#error This program will crash if compiled with g++ 2.x
// see the dynamic_cast bug in the gtkmm FAQ
#endif //
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
#include <gtkmmconfig.h>
#if GTKMM_MAJOR_VERSION==2 && GTKMM_MINOR_VERSION>2
//#include <sigc++/compatibility.h>
#define GMM_GTKMM_22_24(a,b) b
#else //gtkmm 2.2
#define GMM_GTKMM_22_24(a,b) a
#endif //
#include "help_win_glade.hh"
#include <gdk/gdkkeysyms.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/label.h>
#include <gtkmm/viewport.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/box.h>

help_win_glade::help_win_glade(
) : Gtk::Window(Gtk::WINDOW_TOPLEVEL)
{

   Gtk::Window *help_win = this;
   gmm_data = new GlademmData(get_accel_group());

   Gtk::Label *label73 = Gtk::manage(new class Gtk::Label(_("<big><b>Using Cheech:</b></big>\n"
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
		"")));
   Gtk::Viewport *viewport1 = Gtk::manage(new class Gtk::Viewport(*manage(new Gtk::Adjustment(0,0,1)), *manage(new Gtk::Adjustment(0,0,1))));
   Gtk::ScrolledWindow *scrolledwindow2 = Gtk::manage(new class Gtk::ScrolledWindow());
   Gtk::Button *button1 = Gtk::manage(new class Gtk::Button(Gtk::StockID("gtk-close")));
   Gtk::HButtonBox *hbuttonbox8 = Gtk::manage(new class Gtk::HButtonBox(Gtk::BUTTONBOX_END, 0));
   Gtk::VBox *vbox11 = Gtk::manage(new class Gtk::VBox(false, 0));
   label73->set_alignment(0.5,0.5);
   label73->set_padding(0,0);
   label73->set_justify(Gtk::JUSTIFY_LEFT);
   label73->set_line_wrap(true);
   label73->set_use_markup(true);
   label73->set_selectable(false);
   viewport1->set_shadow_type(Gtk::SHADOW_IN);
   viewport1->add(*label73);
   scrolledwindow2->set_size_request(428,360);
   scrolledwindow2->set_flags(Gtk::CAN_FOCUS);
   scrolledwindow2->set_shadow_type(Gtk::SHADOW_NONE);
   scrolledwindow2->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
   scrolledwindow2->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
   scrolledwindow2->add(*viewport1);
   button1->set_flags(Gtk::CAN_FOCUS);
   button1->set_flags(Gtk::CAN_DEFAULT);
   button1->set_relief(Gtk::RELIEF_NORMAL);
   hbuttonbox8->set_border_width(6);
   hbuttonbox8->pack_start(*button1);
   vbox11->pack_start(*scrolledwindow2);
   vbox11->pack_start(*hbuttonbox8, Gtk::PACK_SHRINK, 0);
   help_win->set_title(_("How to Play Chinese Checkers"));
   help_win->set_modal(false);
   help_win->property_window_position().set_value(Gtk::WIN_POS_NONE);
   help_win->set_resizable(true);
   help_win->property_destroy_with_parent().set_value(false);
   help_win->add(*vbox11);
   label73->show();
   viewport1->show();
   scrolledwindow2->show();
   button1->show();
   hbuttonbox8->show();
   vbox11->show();
   button1->signal_clicked().connect(sigc::mem_fun(this, &help_win_glade::on_close_button_clicked), false);
}

help_win_glade::~help_win_glade()
{  delete gmm_data;
}
