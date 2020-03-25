/*
 *  This bot tries to make a good move for him that's bad for everyone else.
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

#ifndef _BOT_MEAN_HH
#define _BOT_MEAN_HH

#include "bot_friendly.hh"


class BotMean : public BotFriendly
{
	public:
		BotMean(unsigned int depth);

		virtual Glib::ustring get_default_name() const;

	protected:
                //virtual long score_this_move(GameBoard *board, 
			    //   unsigned int player,
			    //   MoveList *move);

  		void find_best_move(GameBoard *board, unsigned int player,
  							std::vector<MoveList> *best_moves,
  							long *best_score);
};

#endif // _BOT_MEAN_HH
