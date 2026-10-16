/*
 *   The How To Play Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _HELP_WIN_HH
#  include "help_win_glade.hh"
#  define _HELP_WIN_HH


class help_win : public help_win_glade
{
	void on_close_button_clicked();
};

#endif
