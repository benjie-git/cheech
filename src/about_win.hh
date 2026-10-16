/*
 *   The About Window.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#ifndef _ABOUT_WIN_HH
#  include "about_win_glade.hh"
#  define _ABOUT_WIN_HH


class about_win : public about_win_glade
{  
        
        void on_close_button_clicked();
};
#endif
