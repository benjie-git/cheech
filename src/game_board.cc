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

#include <math.h>

#include "config.h"
#include "utility.hh"
#include "game_board.hh"


const unsigned int
GameBoard::START_MAP[7][7] =
{
	{0, 0, 0, 0, 0, 0, 0},
	{0, 1, 0, 0, 0, 0, 0},
	{0, 1, 0, 0, 2, 0, 0},
	{0, 1, 0, 2, 0, 3, 0},
	{0, 0, 1, 2, 0, 3, 4},
	{0, 0, 1, 2, 3, 4, 5},
	{0, 1, 2, 3, 4, 5, 6}
};

const unsigned int
GameBoard::GOAL_MAP[7] = {0, 19, 77, 181, 227, 169, 65};

const int
GameBoard::BOARD_MAP[SIZE] =
{
	  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	
	-1, -1, -1, -1, -1, -1, 14, -1, -1, -1, -1, -1, -1,
	
	  -1, -1, -1, -1, -1, 14, 14, -1, -1, -1, -1, -1, -1,
	
	-1, -1, -1, -1, -1, 14, 14, 14, -1, -1, -1, -1, -1,
	
	  -1, -1, -1, -1, 14, 14, 14, 14, -1, -1, -1, -1, -1,
	
	63, 63, 63, 63,  0,  0,  0,  0,  0, 25, 25, 25, 25,
	
	  63, 63, 63,  0,  0,  0,  0,  0,  0, 25, 25, 25, -1,
	
	-1, 63, 63,  0,  0,  0,  0,  0,  0,  0, 25, 25, -1,
	
	  -1, 63,  0,  0,  0,  0,  0,  0,  0,  0, 25, -1, -1,
	
	-1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1,
	
	  -1, 52,  0,  0,  0,  0,  0,  0,  0,  0, 36, -1, -1,
	
	-1, 52, 52,  0,  0,  0,  0,  0,  0,  0, 36, 36, -1,
	
	  52, 52, 52,  0,  0,  0,  0,  0,  0, 36, 36, 36, -1,
	
	52, 52, 52, 52,  0,  0,  0,  0,  0, 36, 36, 36, 36,
	
	  -1, -1, -1, -1, 41, 41, 41, 41, -1, -1, -1, -1, -1,
	
	-1, -1, -1, -1, -1, 41, 41, 41, -1, -1, -1, -1, -1,
	
	  -1, -1, -1, -1, -1, 41, 41, -1, -1, -1, -1, -1, -1,
	
	-1, -1, -1, -1, -1, -1, 41, -1, -1, -1, -1, -1, -1,
	
	  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


GameBoard::GameBoard(unsigned int num_players, bool long_jumps, 
	bool hop_others, bool stop_others)
	:_num_players(num_players),
	 _long_jumps(long_jumps),
	 _hop_others(hop_others),
	 _stop_others(stop_others)
{
	// Creates all the holes with appropriate starting players
	for (unsigned int i = 0; i < SIZE; i++)
		if (BOARD_MAP[i] >= 0)
		{
			double x = (i % SIZE_X) + (util::even(i / SIZE_X) ? 0.5 : 0.0);
			double y = (i / SIZE_X) * (sqrt(3.0) / 2.0);
			_board[i] = new GameHole(i);
			_board[i]->set_location(x, y);
		}
		else
			_board[i] = NULL;

	reset_board();
	init_neighbors();
}


GameBoard::GameBoard(const GameBoard &board)
	:_num_players(board._num_players),
	 _long_jumps(board._long_jumps),
	 _hop_others(board._hop_others),
	 _stop_others(board._stop_others)
{
	for (unsigned int i = 0; i < SIZE; i++)
		if (BOARD_MAP[i] >= 0)
			_board[i] = new GameHole(*board[i]);
		else
			_board[i] = NULL;

	for (int i = 0; i < 6; i++)
		_pegs_in_goal[i] = board._pegs_in_goal[i];

	for (int i = 0; i < 6; i++)
		for (int j = 0; j < 10; j++)
			_pegs[i][j] = board._pegs[i][j];

	init_neighbors();
}


GameBoard& GameBoard::operator=(const GameBoard& board)
{
	if (this == &board)
		return *this;

	_num_players = board._num_players;
	_long_jumps = board._long_jumps;
	_hop_others = board._hop_others;
	_stop_others = board._stop_others;

	for (unsigned int i = 0; i < SIZE; i++)
	{
		delete _board[i];
		if (BOARD_MAP[i] >= 0)
			*_board[i] = *board[i];
		else
			_board[i] = NULL;
	}

	for (int i = 0; i < 6; i++)
		_pegs_in_goal[i] = board._pegs_in_goal[i];

	for (int i = 0; i < 6; i++)
		for (int j = 0; j < 10; j++)
			_pegs[i][j] = board._pegs[i][j];

	return *this;
}


GameBoard::~GameBoard()
{
	for (unsigned int i = 0; i < SIZE; i++)
		delete _board[i];
}


void GameBoard::reconfigure_board(unsigned int num_players, bool long_jumps,
								  bool hop_others, bool stop_others)
{
	_num_players = num_players;
	_long_jumps = long_jumps;
	_hop_others = hop_others;
	_stop_others = stop_others;

	reset_board();
}


void GameBoard::reset_board()
{
	// Creates all the holes with appropriate starting colors
	for (unsigned int i = 0; i < SIZE; i++)
	{
		if (BOARD_MAP[i] >= 0)
		{
			int start_player = (int)(START_MAP[_num_players][BOARD_MAP[i]%10]);
			int end_player = (int)(START_MAP[_num_players][BOARD_MAP[i]/10]);
	
			if (start_player > static_cast<int>(_num_players))
				start_player = 0;
	
			if (end_player > static_cast<int>(_num_players))
				end_player = 0;

			_board[i]->set_start_player(start_player);
			_board[i]->set_current_player(start_player);
			_board[i]->set_end_player(end_player);
		}
	}
	reset_peg_lists();
}


void GameBoard::reset_peg_lists()
{
	unsigned int peg_count[6];

	for (int i = 0; i < 6; i++)
	{
		_pegs_in_goal[i] = 0;
		peg_count[i] = 0;
	}

	// Setup the peg lists to reflect the current state of the board
	for (unsigned int i = 0; i < SIZE; i++)
	{
		if (_board[i] != NULL)
		{
			unsigned int cur_player = _board[i]->get_current_player();
			unsigned int end_player;
			if (cur_player)
			{
				_pegs[cur_player-1][peg_count[cur_player-1]] = i;
				_board[i]->set_peg_list_index(peg_count[cur_player-1]);
				peg_count[cur_player-1]++;

				end_player = _board[i]->get_end_player();
				if (end_player == cur_player)
				{
					_pegs_in_goal[cur_player-1]++;
				}
			}
		}
	}
}


unsigned int GameBoard::get_num_players() const 
{ 
	return _num_players; 
}


bool GameBoard::get_long_jumps_allowed() const
{
	return _long_jumps;
}


bool GameBoard::get_hop_others_allowed() const
{
	return _hop_others;
}


bool GameBoard::get_stop_others_allowed() const
{
	return _stop_others;
}


unsigned int *GameBoard::get_pegs(unsigned int player)
{
	return _pegs[player-1];
}


unsigned int GameBoard::get_size() const 
{ 
	return SIZE; 
}


GameHole* GameBoard::operator[](unsigned int i) const 
{
	return _board[i]; 
}


bool GameBoard::is_other_player_triangle(unsigned int player,
										 unsigned int hole) const
{
	return (_board[hole]->get_start_player() != player &&
			_board[hole]->get_start_player() <= _num_players &&
			_board[hole]->get_end_player() != player &&
			_board[hole]->get_end_player() <= _num_players &&
			(_board[hole]->get_start_player() != 0 ||
			 _board[hole]->get_end_player() != 0));
}


bool GameBoard::valid_move_to(unsigned int start, unsigned int to) const
{
	if (!_hop_others &&
		is_other_player_triangle(_board[start]->get_current_player(), to))
			return false;
	
	return (_board[to] && _board[to]->get_current_player() == 0);
}


bool GameBoard::valid_move(unsigned int from, unsigned int to,
						   int check_dir) const
{
	if (check_dir > -1)
		return (_board[from]->get_neighbor(check_dir) == _board[to]);

	for (int dir = 0; dir <= 5; dir++)
	{
		if (_board[from]->get_neighbor(dir) == _board[to])
			return true;
	}
		
	return false;
}


unsigned int GameBoard::find_valid_jump(unsigned int start, unsigned int from,
										int dir) const
{
	GameHole *cur;
	int N = 0;

	// Is Valid if we can: Start with the from hole...
	cur = _board[from];

	if (_long_jumps)
	{
		// ...followed by N(0-5) empty holes...
		for (N = 0; N < 6; N++)
		{
			GameHole *next = cur->get_neighbor(dir);
	
			if (!next || (next->get_current_player() != 0 &&
				next->get_id() != start))
					break;
			cur = next;
		}
		if (!cur || N == 6)
			return 0;
	}

	// ...followed by a non-empty hole...
	// (and don't allow jumping back over the moving peg)
	cur = cur->get_neighbor(dir);
	if (!cur || cur->get_current_player() == 0 ||
		cur->get_id() == start)
			return 0;

	if (_long_jumps)
	{
		// ...followed by N(0-5) empty holes...
		for (int i = 0; i < N; i++)
		{
			cur = cur->get_neighbor(dir);
			if (!cur || (cur->get_current_player() != 0 &&
				cur->get_id() != start))
					return 0;
		}
	}

	// ...followed by an empty hole
	cur = cur->get_neighbor(dir);
	if (!cur || (cur->get_current_player() != 0 &&
		cur->get_id() != start))
			return 0;

	if (!_hop_others &&
		is_other_player_triangle(_board[start]->get_current_player(),
		cur->get_id()))
			return 0;

	return cur->get_id();
}


bool GameBoard::valid_move_list(const MoveList& move_list,
								bool final) const
{
	if (move_list.size() < 2)
		return false;

	int from = move_list[0];
	int to = move_list[move_list.size() - 1];

	if (_board[from]->get_current_player() == 0 ||
		!valid_move_to(move_list[0], to))
			return false;

	if (final && !_stop_others && is_other_player_triangle(
		_board[from]->get_current_player(), to))
			return false;

	if (move_list.size() == 2 && valid_move(from, to))
		return true;

	for (unsigned int i = 0; i < move_list.size() - 1; i++)
	{
		if (!valid_move_to(move_list[0], move_list[i + 1]))
			return false;

		bool found = false;
		for (unsigned int dir = 0; dir < 6; dir++)
			if (find_valid_jump(move_list[0], move_list[i], dir) == 
				move_list[i + 1])
			{
				found = true;
				break;
			}
		if (!found)
			return false;
	}

	return true;
}


bool GameBoard::make_move_list(const MoveList& move_list)
{
	if (!valid_move_list(move_list, true))
		return false;

	unsigned int from = move_list[0];
	unsigned int to = move_list[move_list.size() - 1];

	unsigned int current_player = _board[from]->get_current_player();
	unsigned int next_player;

	move_peg(from, to);

	next_player = get_next_player(current_player);
	if (player_finished(current_player))
	{
		evt_player_finished(current_player);
		if (next_player == current_player)
		{
			evt_game_over();
			return true;
		}
	}

	return true;
}


void GameBoard::move_peg(unsigned int from, unsigned int to)
{
	unsigned int player = _board[from]->get_current_player();

	_board[to]->set_current_player(player);
	_board[from]->set_current_player(0);

	if (_board[from]->get_end_player() == player)
		_pegs_in_goal[player-1]--;
	if (_board[to]->get_end_player() == player)
		_pegs_in_goal[player-1]++;

	_pegs[player-1][_board[from]->get_peg_list_index()] = to;
	_board[to]->set_peg_list_index(_board[from]->get_peg_list_index());
}


unsigned int GameBoard::get_next_player(unsigned int from_player) const
{
	unsigned int next_player = from_player;
	do
	{
		if (++next_player > _num_players)
			next_player = 1;
		if (next_player == from_player)
			return from_player;
	} while (player_finished(next_player));

	return next_player;
}


bool GameBoard::player_finished(unsigned int player) const
{
	return (player > 0 && _pegs_in_goal[player-1] == 10);
}


bool GameBoard::game_finished() const
{
	for (unsigned int player = 1; player <= _num_players; player++)
		if (!player_finished(player))
			return false;

	return true;
}


unsigned int GameBoard::get_num_pegs_in_goal(unsigned int player) const
{
	return _pegs_in_goal[player-1];
}


void GameBoard::init_neighbors()
{
	// Connects all the holes together
	for (unsigned int i = 0; i < SIZE; i++)
	{
		if (_board[i] != 0)
		{
				_board[i]->set_neighbor(0, _board[i + 1]);
				_board[i]->set_neighbor(3, _board[i - 1]);

			if (util::even(i / SIZE_X))
			{
				_board[i]->set_neighbor(1, _board[i + SIZE_X + 1]);
				_board[i]->set_neighbor(2, _board[i + SIZE_X]);
				_board[i]->set_neighbor(4, _board[i - SIZE_X]);
				_board[i]->set_neighbor(5, _board[i - SIZE_X + 1]);
			}
			else
			{
				_board[i]->set_neighbor(1, _board[i + SIZE_X]);
				_board[i]->set_neighbor(2, _board[i + SIZE_X - 1]);
				_board[i]->set_neighbor(4, _board[i - SIZE_X - 1]);
				_board[i]->set_neighbor(5, _board[i - SIZE_X]);
			}
		}
	}
}


double GameBoard::get_distance(unsigned int from, unsigned int to)
{
	return _board[from]->get_distance_to(_board[to]);
}


int GameBoard::get_goal(unsigned int posn) const
{
	for (unsigned int i=1; i <= 6; i++)
		if (START_MAP[_num_players][i] == posn)
			return GOAL_MAP[i];

	return 0;
}
