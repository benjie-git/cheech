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

#include <thread>
#include <atomic>
#include <algorithm>

#include "bot_base.hh"
#if !defined(CHEECH_IOS)
#include "game_images.hh"
#else
#include "cheech_move_gate.hh"
#endif
#include "utility.hh"

#include "bot_random.hh"
#include "bot_simple.hh"
#include "bot_lookahead.hh"
#include "bot_friendly.hh"
#include "bot_mean.hh"


namespace {
	// Detuned bots normally choose among the top score tiers.  This rail stops
	// them from ever considering a move that is far worse than the best one, so
	// a clearly dominant line (e.g. one that finishes the player, worth tens of
	// thousands) is always played alone instead of being diluted by the tier
	// selection.  It scales with detune: 0 at 100% smarts, 5000 at 50%.
	const long kDetuneGapPerTier = 1000;

	// Total cap on the rail.  With tiers now doubling to 32, the per-tier
	// scaling alone would reach 31000; the rail must stay bounded so a detuned
	// bot never considers a move far below the best (0 at 100% smarts (1 tier), 5000 at
	// 6 or more tiers).
	const long kMaxDetuneGap = 5000;
}


BotBase::BotBase()
{
	_think_delay = 0;
	_move_step_delay = 400;
	_move_done_delay = 600;
	_smarts = 100;
	_abort = FALSE;
	_search_clone = false;
	_search_abort = NULL;
	_search_clones.clear();

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


void BotBase::set_smarts(int percent)
{
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	_smarts = percent;
}


int BotBase::get_smarts() const
{
	return _smarts;
}


int BotBase::top_move_tiers() const
{
	// 100% -> 1 tier (play the best move), 90% -> 2, 80% -> 4, ... 70% -> 8.
	int step = (100 - _smarts) / 10;

	if (step < 0)
		step = 0;
	if (step > 5)
		step = 5;

	return 1 << step;
}


BotBase::~BotBase()
{
	_abort = TRUE;

	for (unsigned int i = 0; i < _search_clones.size(); i++)
		delete _search_clones[i];
	_search_clones.clear();

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
	for (unsigned int c = 1; c <
#if defined(CHEECH_IOS)
		8
#else
		GameImages::get_num_colors()
#endif
		; c++)
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
	if (_abort)
		return false;

	if (_search_abort && _search_abort->load(std::memory_order_relaxed))
		return false;

	return true;
}


BotBase* BotBase::clone_for_search() const
{
	return NULL;
}


bool BotBase::supports_parallel_search() const
{
	return false;
}


void BotBase::prepare_search(GameBoard *board)
{
}


void BotBase::set_search_clone(bool search_clone)
{
	_search_clone = search_clone;
}


void BotBase::set_search_abort(std::atomic<bool> *flag)
{
	_search_abort = flag;
}


void BotBase::make_best_move()
{
	std::vector<MoveList> best_moves;
	GameBoard board(*_client.get_board());
	long best_score = LONG_MIN;

	int tiers = top_move_tiers();

	if (tiers > 1)
	{
		// Detuned: gather every legal root move and its score, then let the
		// bot choose at random from its top `tiers` score tiers.
		std::vector<MoveList> root_moves;
		std::vector<long> scores;

		collect_root_moves(&board, _client.get_my_player_number(), &root_moves);
		score_moves(&board, _client.get_my_player_number(),
					&root_moves, &scores, true);
		select_top_moves(root_moves, scores, tiers, &best_moves, &best_score);
	}
	else if (!parallel_root_search(&board, _client.get_my_player_number(),
								   &best_moves, &best_score))
	{
		find_best_move(&board, _client.get_my_player_number(),
					   &best_moves, &best_score);
	}

	// Aborted (undo/etc) or no legal move found.
	if (_abort || best_moves.empty())
		return;

	make_move(&(best_moves[_rand.get_int_range(0, (int)best_moves.size())]));
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
	std::bitset<GameBoard::SIZE> tos;

	for (unsigned int i = 0; i < 10; i++)
	{
		move->push_back(pegs[i]);
		tos.reset();
		tos.set(pegs[i]);
		find_better_move_for_peg(board, player, move, best_moves,
								 best_score, &tos);

		move->pop_back();

		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn()) return;
	}
}


void BotBase::find_better_move_for_peg(GameBoard *board, unsigned int player,
	MoveList *move,	std::vector<MoveList> *best_moves, long *best_score,
	std::bitset<GameBoard::SIZE> *tos)
{
	unsigned int from_hole = move->back();

	for (int dir = 0; dir < 6; dir++)
	{
		// Potential non-jumping move
		if (move->size() == 1 && (*board)[from_hole]->get_neighbor(dir))
		{
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
					if (best_moves)
					{
						best_moves->clear();
						best_moves->push_back(*move);
					}
				}
				else if (score == *best_score && best_moves)
				{
					best_moves->push_back(*move);
				}

				if (_think_delay && _client.ready())
				{
					_client.show_move(move);
					util::delay_ms(_think_delay);
				}

				move->pop_back();
				tos->set(to_hole);
			}
		}

		// Potential jumping move
		unsigned int to_hole = board->find_valid_jump(move->front(),
													  from_hole, dir);

		if (to_hole && !tos->test(to_hole))
		{
			move->push_back(to_hole);

			if (board->get_stop_others_allowed() ||
				!board->is_other_player_triangle(player, to_hole))
			{
				long score = score_move(board, player, move);
				if (score > *best_score)
				{
					*best_score = score;
					if (best_moves)
					{
						best_moves->clear();
						best_moves->push_back(*move);
					}
				}
				else if (score == *best_score && best_moves)
				{
					best_moves->push_back(*move);
				}

				if (_think_delay && _client.ready())
				{
					_client.show_move(move);
					util::delay_ms(_think_delay);
				}
			}
			tos->set(to_hole);

			// Recurse
			find_better_move_for_peg(board, player, move, best_moves,
									 best_score, tos);
			move->pop_back();

			// Abort if it's not my turn anymore (undo/etc)
			if (!is_still_my_turn()) return;
		}
	}
}


void BotBase::collect_root_moves(GameBoard *board, unsigned int player,
								 std::vector<MoveList> *moves)
{
	unsigned int *pegs = board->get_pegs(player);
	MoveList move(0);
	move.reserve(10);

	for (unsigned int i = 0; i < 10; i++)
	{
		move.push_back(pegs[i]);

		std::bitset<GameBoard::SIZE> tos;
		tos.reset();
		tos.set(pegs[i]);

		collect_peg_moves(board, player, &move, &tos, moves);

		move.pop_back();
	}
}


void BotBase::collect_peg_moves(GameBoard *board, unsigned int player,
	MoveList *move, std::bitset<GameBoard::SIZE> *tos,
	std::vector<MoveList> *moves)
{
	unsigned int from_hole = move->back();

	for (int dir = 0; dir < 6; dir++)
	{
		// Potential non-jumping move
		if (move->size() == 1 && (*board)[from_hole]->get_neighbor(dir))
		{
			unsigned int to_hole =
				(*board)[from_hole]->get_neighbor(dir)->get_id();

			if (board->valid_move_to(move->front(), to_hole) &&
				board->valid_move(from_hole, to_hole, dir) &&
				(board->get_stop_others_allowed() ||
				!board->is_other_player_triangle(player, to_hole)))
			{
				move->push_back(to_hole);
				moves->push_back(*move);
				move->pop_back();
				tos->set(to_hole);
			}
		}

		// Potential jumping move
		unsigned int to_hole = board->find_valid_jump(move->front(),
													  from_hole, dir);

		if (to_hole && !tos->test(to_hole))
		{
			move->push_back(to_hole);

			if (board->get_stop_others_allowed() ||
				!board->is_other_player_triangle(player, to_hole))
			{
				moves->push_back(*move);
			}
			tos->set(to_hole);

			// Recurse
			collect_peg_moves(board, player, move, tos, moves);
			move->pop_back();
		}
	}
}


bool BotBase::score_moves(GameBoard *board, unsigned int player,
						  std::vector<MoveList> *moves,
						  std::vector<long> *scores, bool allow_parallel)
{
	scores->assign(moves->size(), LONG_MIN);

	if (allow_parallel && supports_parallel_search() && _think_delay <= 0
		&& moves->size() >= 2)
	{
		unsigned int hw = std::thread::hardware_concurrency();

		if (hw >= 2)
		{
			if (hw > 16)
				hw = 16;

			unsigned int num_threads = hw;
			if (num_threads > moves->size())
				num_threads = (unsigned int)moves->size();

			// Reuse thread-private search clones (and their transposition
			// tables) across turns.  Each clone's TT is invalidated by the
			// generation counter, so it needs no per-turn clearing or
			// reallocation.
			while (_search_clones.size() < num_threads)
			{
				BotBase *clone = clone_for_search();

				if (!clone)
					break;

				clone->set_search_clone(true);
				_search_clones.push_back(clone);
			}

			if (_search_clones.size() >= num_threads)
			{
				std::atomic<unsigned int> next(0);
				std::atomic<unsigned int> done(0);
				std::atomic<bool> abort_flag(false);

				auto worker = [&](unsigned int t)
				{
					BotBase *clone = _search_clones[t];
					GameBoard local(*board);

					clone->set_search_abort(&abort_flag);
					clone->prepare_search(&local);

					unsigned int i;
					while (!abort_flag.load() &&
						   (i = next.fetch_add(1)) < (unsigned int)moves->size())
					{
						MoveList move = (*moves)[i];
						(*scores)[i] = clone->score_move(&local, player, &move);
					}

					done.fetch_add(1);
				};

				std::vector<std::thread> threads;
				threads.reserve(num_threads);

				for (unsigned int t = 0; t < num_threads; t++)
					threads.push_back(std::thread(worker, t));

				// Wait for the workers, but keep pumping the main loop so
				// that an undo is noticed and can abort the search.
				while (done.load() < num_threads)
				{
					if (_abort)
						abort_flag.store(true);

					util::delay_us(1000);
				}

				for (unsigned int t = 0; t < num_threads; t++)
					threads[t].join();

				return true;
			}
		}
	}

	// Serial fallback: score on this bot.  prepare_search() performs the
	// root-level setup the look-ahead bots need (distance cache, TT
	// generation, starting depth).
	prepare_search(board);

	for (unsigned int i = 0; i < moves->size(); i++)
	{
		MoveList move = (*moves)[i];
		(*scores)[i] = score_move(board, player, &move);

		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn())
			break;
	}

	return true;
}


bool BotBase::parallel_root_search(GameBoard *board, unsigned int player,
								   std::vector<MoveList> *best_moves,
								   long *best_score)
{
	if (!supports_parallel_search() || _think_delay > 0)
		return false;

	unsigned int hw = std::thread::hardware_concurrency();

	if (hw < 2)
		return false;
	if (hw > 16)
		hw = 16;

	std::vector<MoveList> root_moves;
	root_moves.reserve(64);
	collect_root_moves(board, player, &root_moves);

	if (root_moves.size() < 2)
		return false;

	std::vector<long> scores;
	score_moves(board, player, &root_moves, &scores, true);

	if (_abort)
		return true;

	// Merge in enumeration order so the chosen move and tie set match the
	// single-threaded search exactly.
	for (unsigned int i = 0; i < root_moves.size(); i++)
	{
		if (scores[i] == LONG_MIN)
			continue;

		if (scores[i] > *best_score)
		{
			*best_score = scores[i];
			best_moves->clear();
			best_moves->push_back(root_moves[i]);
		}
		else if (scores[i] == *best_score)
		{
			best_moves->push_back(root_moves[i]);
		}
	}

	return true;
}


void BotBase::select_top_moves(const std::vector<MoveList> &root_moves,
							   const std::vector<long> &scores, int tiers,
							   std::vector<MoveList> *best_moves,
							   long *best_score)
{
	*best_score = LONG_MIN;
	best_moves->clear();

	std::vector<unsigned int> order;
	order.reserve(root_moves.size());

	for (unsigned int i = 0; i < root_moves.size(); i++)
		if (scores[i] != LONG_MIN)
			order.push_back(i);

	std::stable_sort(order.begin(), order.end(),
		[&scores](unsigned int a, unsigned int b)
		{
			return scores[a] > scores[b];
		});

	if (order.empty())
		return;

	long best = scores[order[0]];

	// Dominance safety rail: never consider a move that is this far below the
	// best one, however many tiers are in play.
	long max_gap = kDetuneGapPerTier * (tiers - 1);

	if (max_gap > kMaxDetuneGap)
		max_gap = kMaxDetuneGap;

	long limit = best - max_gap;

	// If any move actually improves the position, never pick one that scores
	// zero or below (shuffling a peg backwards, leaving a goal, etc.) just
	// because detuning widened the tier selection.  When the best move only
	// scores zero the player cannot improve, so keep the usual tier choice
	// among the non-negative moves; when every move is negative the player is
	// stuck and the negatives are all that is left.
	if (best > 0 && limit <= 0)
		limit = 1;
	else if (best == 0 && limit < 0)
		limit = 0;

	int tier = -1;
	long current = LONG_MIN;

	for (unsigned int k = 0; k < order.size(); k++)
	{
		unsigned int i = order[k];

		if (scores[i] < limit)
			break;

		if (tier < 0 || scores[i] != current)
		{
			tier++;

			if (tier >= tiers)
				break;

			current = scores[i];

			if (tier == 0)
				*best_score = current;
		}

		best_moves->push_back(root_moves[i]);
	}
}


void BotBase::make_move(MoveList *list)
{
	if (list->empty())
		return;

#if defined(CHEECH_IOS) || defined(CHEECH_PORTABLE)
	// Non-blocking pacing: the move is committed on a timer once the previous
	// move's UI animation has finished.  This lets the next player's search
	// start as soon as the turn arrives, while its commit waits for the gate.
	if (!is_still_my_turn())
		return;

	MoveList move_copy = *list;
	Glib::signal_timeout().connect(sigc::bind(
		sigc::mem_fun(*this, &BotBase::try_commit_move), move_copy), 30);
	return;
#else
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
#endif
}


#if defined(CHEECH_IOS)
bool BotBase::try_commit_move(MoveList move)
{
	if (!is_still_my_turn())
		return false;

	// Keep polling until the in-flight animation has finished.
	if (!cheech::move_gate_open())
		return true;

	_client.make_move(&move);

	Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(&_client,
								   &GameClient::hide_move), false), 0);

	return false;
}
#endif


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


long BotBase::goal_block_penalty(GameBoard *board, unsigned int player,
								 MoveList *move) const
{
	if (!move || move->empty())
		return 0;

	GameHole *hole = (*board)[move->back()];
	if (!hole)
		return 0;

	unsigned int owner = hole->get_end_player();

	// Own goal or a hole that is nobody's goal: nothing to penalize.
	if (owner == 0 || owner == player || owner > board->get_num_players())
		return 0;

	// Ending a move inside an opponent's goal takes one of their ten slots,
	// so they can never finish while the peg sits there.  Discourage it
	// strongly; it is still chosen if no other move exists.
	long penalty = 20000;

	// Extra deterrent when it is that opponent's very last free hole.
	if (board->get_num_pegs_in_goal(owner) == 9)
		penalty += 10000;

	return -penalty;
}


long BotBase::goal_exit_penalty(GameBoard *board, unsigned int player,
								MoveList *move) const
{
	if (!move || move->empty())
		return 0;

	GameHole *from = (*board)[move->front()];
	GameHole *to = (*board)[move->back()];
	if (!from || !to)
		return 0;

	// Only a peg that is giving up its slot in the player's own goal matters.
	if (from->get_end_player() != player)
		return 0;
	if (to->get_end_player() == player)
		return 0;

	// Never worth walking a peg back out of the goal; keep the penalty well
	// above anything detuning can overlook.
	return -20000;
}
