/*
 *  Abstract cheech-playig bot class.
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

#include <glibmm/random.h>
#include <glibmm/main.h>

#include "bot_base.hh"
#include "game_images.hh"
#include "utility.hh"

#include "bot_random.hh"
#include "bot_simple.hh"
#include "bot_lookahead.hh"
#include "bot_friendly.hh"
#include "bot_mean.hh"


BotBase::BotBase()
{
	_think_delay = 0;
	_move_step_delay = 400;
	_move_done_delay = 600;
	_abort = FALSE;

	_client.change_color(5);
}


BotBase* BotBase::new_bot_of_type(Glib::ustring type)
{
	type = type.lowercase();

	if (type == "random" || type == "r")
		return new BotRandom;
	else if (type == "simple(1)" || type == "simple" || type == "s")
		return new BotSimple;

	else if (type == "lookahead(2)" || type == "lookahead2" || type == "l2")
		return new BotLookAhead(2);
	else if (type == "lookahead(3)" || type == "lookahead3" || type == "l3")
		return new BotLookAhead(3);
	else if (type == "lookahead(4)" || type == "lookahead4" || type == "l4")
		return new BotLookAhead(4);
	else if (type == "lookahead(5)" || type == "lookahead5" || type == "l5")
		return new BotLookAhead(5);

	else if (type == "friendly(2)" || type == "friendly2" || type == "f2")
		return new BotFriendly(2);
	else if (type == "friendly(3)" || type == "friendly3" || type == "f3")
		return new BotFriendly(3);
	else if (type == "friendly(4)" || type == "friendly4" || type == "f4")
		return new BotFriendly(4);
	else if (type == "friendly(5)" || type == "friendly5" || type == "f5")
		return new BotFriendly(5);

	else if (type == "mean(2)" || type == "mean2" || type == "m2")
		return new BotMean(2);
	else if (type == "mean(3)" || type == "mean3" || type == "m3")
		return new BotMean(3);
	else if (type == "mean(4)" || type == "mean4" || type == "m4")
		return new BotMean(4);
	else if (type == "mean(5)" || type == "mean5" || type == "m5")
		return new BotMean(5);

	return NULL;  // Shouldn't happen
}


void BotBase::set_name(Glib::ustring name)
{
	_client.change_name((name != "") ? name : get_default_name());
}


void BotBase::set_color(unsigned int color)
{
	_client.change_color(color);
}


void BotBase::set_think_delay(int delay)
{
	_think_delay = delay;
}


void BotBase::set_move_delay(int delay, int done_delay)
{
	_move_step_delay = delay;
	_move_done_delay = done_delay;
}


BotBase::~BotBase()
{
	_abort = TRUE;
	_client.leave_game();
}


GameClient* BotBase::get_game_client()
{
	return &_client;
}


void BotBase::join_game(Glib::ustring host, unsigned int port)
{
	if (_client.get_my_name() == "")
		_client.change_name(get_default_name());

	_client.evt_disconnected.connect(sigc::mem_fun(*this,
		&BotBase::on_disconnect));
	_client.evt_connected.connect(sigc::mem_fun(*this,
		&BotBase::on_connect));
	_client.evt_cancelled.connect(sigc::mem_fun(*this,
		&BotBase::on_cancelled));
	_client.evt_message.connect(sigc::mem_fun(*this,
		&BotBase::on_message));
	_client.cmd_choose_new_name.connect(sigc::mem_fun(*this,
		&BotBase::on_cmd_choose_new_name));
	_client.cmd_choose_new_color.connect(sigc::mem_fun(*this,
		&BotBase::on_cmd_choose_new_color));
	_client.cmd_game_turn.connect(sigc::mem_fun(*this,
		&BotBase::on_cmd_game_turn));

	_client.join_game(host, port, false);
}


void BotBase::leave_game()
{
	_abort = TRUE;
	_client.leave_game();
}


void BotBase::on_connect()
{
	_abort = FALSE;
	evt_connected();
}


void BotBase::on_cancelled()
{
	_abort = TRUE;
	evt_cancelled();
}


void BotBase::on_disconnect()
{
	_abort = TRUE;
	evt_disconnected();
}


void BotBase::on_message(Glib::ustring msg)
{
	evt_message(msg);
}


void BotBase::on_cmd_choose_new_name(Glib::ustring name)
{
	int n = 0;

	if (name.length() > 2 && name[name.length()-2] == ' ' &&
		name[name.length()-1] >= '2' && name[name.length()-1] <= '6')
			n = util::from_str<int>(name.substr(name.length()-1));

	if (n)
		_client.change_name(name.substr(0, name.length()-2) + " " + util::to_str(n+1));
	else
		_client.change_name(name + " 2");
}


void BotBase::on_cmd_choose_new_color(Glib::ustring name, int color)
{
	for (unsigned int c = 1; c < GameImages::get_num_colors(); c++)
	{
		bool taken = false;

		for (unsigned int p = 1; p <= 6; p++)
			if (_client.get_player_color(p) == c)
				taken = true;

		if (!taken)
		{
			_client.change_color(c);
			return;
		}
	}
}


void BotBase::on_cmd_game_turn(unsigned int posn,
							   GameServer::GameStatus status,
							   unsigned int move_count)
{
	// Work around a gnet bug by using a timeout
	if (posn == _client.get_my_player_number())
	{
		_abort = FALSE;
		Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this,
			&BotBase::make_best_move), false), 1);
	}
	else {
		_abort = TRUE;
	}

//	if (move_count % 50 == 0) {
//		printf("oops\n");
//	}
}


bool BotBase::is_still_my_turn()
{
	return (!_abort);
}


void BotBase::make_best_move()
{
	std::vector<MoveList> best_moves;
	GameBoard board(*_client.get_board());
	long best_score = LONG_MIN;

	find_best_move(&board, _client.get_my_player_number(),
					&best_moves, &best_score);

	make_move(&(best_moves[_rand.get_int_range(0, best_moves.size())]));
}


void BotBase::find_best_move(GameBoard *board, unsigned int player,
							 std::vector<MoveList> *best_moves,
							 long *best_score)
{
	// Abort if it's not my turn anymore (undo/etc)
	if (!is_still_my_turn()) return;

	MoveList move(0);
	move.reserve(10);

	find_better_move(board, player, &move, best_moves, best_score);
}


void BotBase::find_better_move(GameBoard *board, unsigned int player,
	MoveList *move,	std::vector<MoveList> *best_moves, long *best_score)
{
	unsigned int *pegs = board->get_pegs(player);
	std::set<unsigned int> tos;

	for (unsigned int i = 0; i < 10; i++)
	{
		move->push_back(pegs[i]);
		tos.clear();
		tos.insert(pegs[i]);
		find_better_move_for_peg(board, player, move, best_moves,
								 best_score, &tos);

		move->pop_back();

		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn()) return;
	}
}


void BotBase::find_better_move_for_peg(GameBoard *board, unsigned int player,
	MoveList *move,	std::vector<MoveList> *best_moves, long *best_score,
	std::set<unsigned int> *tos)
{
	unsigned int from_hole = move->back();

	for (int dir = 0; dir < 6; dir++)
	{
		// Potential non-jumping move
		if (move->size() == 1 && (*board)[from_hole]->get_neighbor(dir))
		{
			//if (move->size() == 1 && (from_hole == 70 || from_hole == 72)) {
			//	printf("zing!\n");
			//}
			unsigned int to_hole =
				(*board)[from_hole]->get_neighbor(dir)->get_id();

			if (board->valid_move_to(move->front(), to_hole) &&
				board->valid_move(from_hole, to_hole, dir) &&
				(board->get_stop_others_allowed() ||
				!board->is_other_player_triangle(player, to_hole)))
			{
				move->push_back(to_hole);
				long score = score_move(board, player, move);
				if (score > *best_score)
				{
					*best_score = score;
					best_moves->clear();
					best_moves->push_back(*move);
				}
				else if (score == *best_score)
				{
					best_moves->push_back(*move);
				}

				if (_client.ready() && _think_delay)
				{
					_client.show_move(move);
					//printf("%ld\n", score);
					util::delay_ms(_think_delay);
				}

				move->pop_back();
				tos->insert(to_hole);
			}
		}

		// Potential jumping move
		unsigned int to_hole = board->find_valid_jump(move->front(),
													  from_hole, dir);

		if (to_hole && tos->find(to_hole) == tos->end())
		{
			move->push_back(to_hole);

			if (board->get_stop_others_allowed() ||
				!board->is_other_player_triangle(player, to_hole))
			{
				long score = score_move(board, player, move);
				if (score > *best_score)
				{
					*best_score = score;
					best_moves->clear();
					best_moves->push_back(*move);
				}
				else if (score == *best_score)
				{
					best_moves->push_back(*move);
				}

				if (_client.ready() && _think_delay)
				{
					_client.show_move(move);
					//printf("%ld\n", score);
					util::delay_ms(_think_delay);
				}
			}
			tos->insert(to_hole);

			// Recurse
			find_better_move_for_peg(board, player, move, best_moves,
									 best_score, tos);
			move->pop_back();

			// Abort if it's not my turn anymore (undo/etc)
			if (!is_still_my_turn()) return;
		}
	}
}


void BotBase::make_move(MoveList *list)
{
	if (list->empty())
		return;

	if (_move_step_delay > 0)
	{
		MoveList partial(0);

		for (MoveList::iterator h = list->begin();
			h < list->end(); h++)
		{
			// Abort if it's not my turn anymore (undo/etc)
			if (!is_still_my_turn()) return;

			partial.push_back(*h);
			_client.show_move(&partial);
			util::delay_ms(_move_step_delay);
		}
		util::delay_ms(_move_done_delay - _move_step_delay);
	}
	else
	{
		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn()) return;

		_client.show_move(list);
		util::delay_ms(_move_done_delay);
	}

	// Abort if it's not my turn anymore (undo/etc)
	if (!is_still_my_turn()) return;

	_client.make_move(list);

	Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(&_client,
								   &GameClient::hide_move), false), 0);
}


bool BotBase::is_blocking_pegs(GameBoard *board, unsigned int player)
{
	if (board->get_num_pegs_in_goal(player) < 4)
		return false;

	GameHole *goal = (*board)[board->get_goal(player)];
	GameHole *test;
	unsigned int dirs[2] = {10, 10};

	for (unsigned int dir=0; dir<6; dir++) {
		if (goal->get_neighbor(dir)) {
			if (dirs[0] == 10)
				dirs[0] = dir;
			else
				dirs[1] = dir;
		}
	}

	// Make sure I'm not blocking someone else into my goal corner hole...
	test = goal;
	if (test->get_current_player() != 0 &&
		test->get_current_player() != player &&
		test->get_neighbor(dirs[0], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[0], 2)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 2)->get_current_player() == player)
			return true;

	// ...nor into one of my goal corner hole's neighbor holes
	test = goal->get_neighbor(dirs[0]);
	if (test->get_current_player() != 0 &&
		test->get_current_player() != player &&
		test->get_neighbor(dirs[0], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[0], 2)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 2)->get_current_player() == player)
			return true;

	// ...nor into the other of my goal corner hole's neighbor holes
	test = goal->get_neighbor(dirs[1]);
	if (test->get_current_player() != 0 &&
		test->get_current_player() != player &&
		test->get_neighbor(dirs[0], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[0], 2)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 1)->get_current_player() == player &&
		test->get_neighbor(dirs[1], 2)->get_current_player() == player)
			return true;

	return false;
}
