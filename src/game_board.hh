/*
 *  The Chinese Checkers board.
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

#ifndef INCL_GAME_BOARD_HH
#define INCL_GAME_BOARD_HH

#include <vector>
#include <sigc++/sigc++.h>

#include "game_hole.hh"


typedef std::vector<unsigned int> MoveList;


class GameBoard : public sigc::trackable
{
public:
	const static unsigned int SIZE_X = 13;
	const static unsigned int SIZE_Y = 19;
	const static unsigned int SIZE = SIZE_X * SIZE_Y;

	const static unsigned int START_MAP[7][7];
	const static unsigned int GOAL_MAP[7];
	const static int BOARD_MAP[SIZE];

	sigc::signal<void, unsigned int> evt_player_finished;
	sigc::signal<void> evt_game_over;

private:
	unsigned int				_num_players;
	GameHole*					_board[SIZE];
	bool						_long_jumps;
	bool						_hop_others;
	bool						_stop_others;
	unsigned int				_pegs_in_goal[6];
	unsigned int				_pegs[6][10];

public:
	GameBoard(unsigned int num_players, bool long_jumps,
			  bool hop_others, bool stop_others);
	GameBoard(const GameBoard &board);
	GameBoard& operator=(const GameBoard& board);
	virtual ~GameBoard();

	void reset_board();
	void reset_peg_lists();
	void reconfigure_board(unsigned int num_players, bool long_jumps,
						   bool hop_others, bool stop_others);

	unsigned int get_num_players() const;
	bool get_long_jumps_allowed() const;
	bool get_hop_others_allowed() const;
	bool get_stop_others_allowed() const;
	unsigned int *get_pegs(unsigned int player);
	unsigned int get_size() const;
	GameHole* operator[](unsigned int i) const;

	double get_distance(unsigned int from, unsigned int to);
	int get_goal(unsigned int posn) const;
	unsigned int get_next_player(unsigned int from_player) const;
	bool player_finished(unsigned int player) const;
	bool game_finished() const;
	unsigned int get_num_pegs_in_goal(unsigned int player) const;

	bool is_other_player_triangle(unsigned int player, unsigned int hole) const;
	bool valid_move_to(unsigned int start, unsigned int to) const;
	bool valid_move(unsigned int from, unsigned int to,
					int check_dir = -1) const;
	bool valid_move_list(const MoveList& move_list,
						 bool final) const;
	unsigned int find_valid_jump(unsigned int start, unsigned int from,
								 int dir) const;

	bool make_move_list(const MoveList& move_list);
	void move_peg(unsigned int from, unsigned int to);


private:
	void init_neighbors();
};

#endif   // #ifndef INCL_GAME_BOARD_HH
