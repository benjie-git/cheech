/*
 *  Smarter cheech-playig bot class-- looks ahead multiple moves, and scores
 *  not just the one move, but the whole board.
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

#include "bot_lookahead.hh"
#include "utility.hh"
#include <stdio.h>
#include <algorithm>

static inline uint64_t tt_key(uint64_t zobrist, unsigned int player,
							  unsigned int depth)
{
	uint64_t key = zobrist
		^ (0x9E3779B97F4A7C15ULL * player)
		^ (0xBF58476D1CE4E5B9ULL * depth);
	return key ? key : 1;
}

BotLookAhead::BotLookAhead(unsigned int depth, unsigned int tt_bits) :
	BotBase()
{
	_depth = depth;
	_full_depth = depth;
	_current_depth = 0;
	// LookAhead plans its own sequence of moves only: opponents are treated
	// as static obstacles and never get a turn in the search, so the bot
	// never spends tempo denying another player.  Mean keeps the paranoid
	// adversarial search (it sets _paranoid back to true).
	_paranoid = false;
	_root_player = 0;
	_self_penalty = 1;
	_dist_players = 0;
	_search_moves.resize(depth);
	_tt_gen = 0;
	_tt_mask = (1u << tt_bits) - 1;
	if (depth > 1 && tt_bits > 0)
		_tt.resize(1u << tt_bits);

	for (int p = 0; p < 7; p++)
		for (unsigned int h = 0; h < GameBoard::SIZE; h++)
			_dist_to_goal[p][h] = 0;
}


void BotLookAhead::set_self_penalty(int self_penalty)
{
	if (self_penalty < 1)
		self_penalty = 1;

	_self_penalty = self_penalty;
}


Glib::ustring BotLookAhead::default_name(unsigned int depth)
{
	switch (depth)
	{
		case 2:
			return "Rosie";
		case 3:
			return "Cosmo";
		case 4:
			return "Artoo";
		case 5:
			return "Brainiac";
		default:
			return "Who,Now?";
	}
}


Glib::ustring BotLookAhead::get_default_name() const
{
	return default_name(_depth);
}


Glib::ustring BotLookAhead::get_type_name() const
{
	return "Neutral";
}


void BotLookAhead::on_cmd_game_turn(unsigned int posn, 
									GameServer::GameStatus status,
									unsigned int move_count)
{
	// Do not disturb a search that is currently running: _current_depth
	// belongs to it, and resetting it mid-search indexes _search_moves out
	// of bounds.  The next turn will reset it once that search has finished.
	if (!_searching)
		_current_depth = _depth;
	BotBase::on_cmd_game_turn(posn, status, move_count);
}


BotBase* BotLookAhead::clone_for_search() const
{
	BotLookAhead *clone = new BotLookAhead(_full_depth, TT_CLONE_BITS);
	clone->set_self_penalty(_self_penalty);
	clone->set_friends(get_friends());
	clone->set_enemies(get_enemies());
	return clone;
}


bool BotLookAhead::supports_parallel_search() const
{
	// Each worker gets its own transposition table, so pure look-ahead loses
	// the cross-root-move transpositions that make its table so effective.
	// Only parallelise once the tree is deep enough to still come out ahead.
	return (_depth >= 4);
}


void BotLookAhead::prepare_search(GameBoard *board)
{
	_current_depth = _depth;
	update_distance_cache(board);
	_tt_gen++;
}


void BotLookAhead::find_best_move(GameBoard *board, unsigned int player,
								  std::vector<MoveList> *best_moves,
								  long *best_score)
{
	// Abort if it's not my turn anymore (undo/etc)
	if (!is_still_my_turn()) return;

	// A root search always starts at the full depth, even if a previous
	// search was aborted partway through and left _current_depth stale.
	if (best_moves && _current_depth != _depth)
		_current_depth = _depth;

	// In the cooperative recursion a sub-node belongs to whichever focused
	// player moves next: hand the turn to the next focused, unfinished
	// player rather than the nominal one.  (Plain LookAhead focuses on
	// everyone, so this only ever skips finished players.)
	if (_current_depth < _depth)
	{
		unsigned int focus = get_friends();
		if (focus != ALL_PLAYERS && _root_player > 0)
			focus |= (1u << (_root_player - 1));
		player = next_focused_player(board, player, focus);
	}

	bool at_root = (_current_depth == _depth);

	if (at_root)
	{
		update_distance_cache(board);
		_tt_gen++;
	}
	else if (!_tt.empty())
	{
		uint64_t key = tt_key(board->get_zobrist(), player, _current_depth);
		TTEntry &entry = _tt[key & _tt_mask];

		if (entry.key == key && entry.gen == _tt_gen)
		{
			*best_score = entry.score;
			return;
		}
	}

	// The move path buffer is local: find_better_move() keeps it balanced
	// (push/pop) and nested sub-node searches must not share it.
	MoveList path;
	path.reserve(10);

	find_better_move(board, player, &path, best_moves, best_score);

	// Only store real scores: a node with no legal move (or an aborted one)
	// leaves *best_score at LONG_MIN, which must never leak into the table.
	if (!at_root && !_tt.empty() && is_still_my_turn() &&
		*best_score != LONG_MIN)
	{
		uint64_t key = tt_key(board->get_zobrist(), player, _current_depth);
		TTEntry &entry = _tt[key & _tt_mask];

		entry.key = key;
		entry.score = *best_score;
		entry.best = 0;
		entry.gen = _tt_gen;
		entry.flag = TT_EXACT;
	}
}


void BotLookAhead::update_distance_cache(GameBoard *board)
{
	// Distances are static board geometry and get_goal() depends only on
	// the player count, so the cache needs rebuilding only when the player
	// count changes (it starts zeroed).
	unsigned int num_players = board->get_num_players();
	if (num_players == _dist_players)
		return;
	_dist_players = num_players;

	for (unsigned int p = 1; p <= 6; p++)
	{
		unsigned int goal = board->get_goal(p);

		for (unsigned int h = 0; h < GameBoard::SIZE; h++)
		{
			if (goal && (*board)[h])
				_dist_to_goal[p][h] =
					(long)(50 * board->get_distance(h, goal) + 0.5);
			else
				_dist_to_goal[p][h] = 0;
		}
	}
}


long BotLookAhead::score_move(GameBoard *board, unsigned int player,
							  MoveList *move)
{
	// score_move() is only ever called to score a root move, but it is also
	// reached with deeper players during the cooperative recursion, so only
	// record the root when we are actually at the root depth.
	if (_current_depth == _depth)
		_root_player = player;

	if (_paranoid)
		return paranoid_move_value(board, player, move);

	return score_move_recurse(board, player, move);
}


long BotLookAhead::score_move_recurse(GameBoard *board, unsigned int player,
									  MoveList *move)
{
	unsigned int front = move->front();
	unsigned int back = move->back();

	board->move_peg(front, back);

	long total_score = score_this_move(board, player, move);

	if (board->player_finished(player) || total_score > 9000)
	{
		board->move_peg(back, front);
		return total_score * _current_depth;
	}

	total_score = total_score * _current_depth;

	if (_current_depth > 1 && is_still_my_turn())
	{
		long best_score = LONG_MIN;

		_current_depth--;

		find_best_move(board, player, NULL, &best_score);

		// Abort if it's not my turn anymore (undo/etc)
		if (!is_still_my_turn())
		{
			board->move_peg(back, front);
			return total_score;
		}

		_current_depth++;

		// A sub-node with no legal move reports LONG_MIN; treat that as no
		// gain instead of overflowing the addition.
		if (best_score != LONG_MIN)
			total_score += best_score;
	}

	board->move_peg(back, front);

	return total_score;
}


long BotLookAhead::paranoid_move_value(GameBoard *board, unsigned int player,
									   MoveList *move)
{
	unsigned int front = move->front();
	unsigned int back = move->back();

	board->move_peg(front, back);

	_current_depth = _depth;

	long total_score = score_this_move(board, player, move);

	// A move that finishes the player ends the branch (own finishes are
	// scaled by the self-penalty, matching score_move_recurse).
	if (board->player_finished(player) || total_score > 9000)
	{
		board->move_peg(back, front);
		return total_score * _depth;
	}

	total_score = total_score * _depth;

	if (_depth > 1 && is_still_my_turn())
	{
		total_score += paranoid_search(board, board->get_next_player(player),
									   player, _depth - 1,
									   LONG_MIN, LONG_MAX);
	}

	board->move_peg(back, front);

	return total_score;
}


long BotLookAhead::paranoid_search(GameBoard *board, unsigned int player,
								   unsigned int root, unsigned int remaining,
								   long alpha, long beta)
{
	if (remaining == 0 || !is_still_my_turn())
		return 0;

	// The root player always participates; other players only if they are in
	// the enemy set.  A player outside the set is frozen: it gets no move and
	// does not consume a ply, so the horizon is spent only on players this
	// bot actually pays attention to.
	unsigned int focus = get_enemies() | (1u << (root - 1));

	if (!focuses_on(player, focus))
	{
		unsigned int next = next_focused_player(board, player, focus);

		if (next == player || !focuses_on(next, focus))
			return 0;

		return paranoid_search(board, next, root, remaining, alpha, beta);
	}

	bool maximizing = (player == root);

	uint64_t key = 0;
	TTEntry *entry = NULL;
	bool hit = false;

	if (!_tt.empty())
	{
		key = tt_key(board->get_zobrist(), player, remaining);
		entry = &_tt[key & _tt_mask];

		if (entry->key == key && entry->gen == _tt_gen)
		{
			hit = true;

			if (entry->flag == TT_EXACT)
				return entry->score;

			if (entry->flag == TT_LOWER)
			{
				if (entry->score > alpha)
					alpha = entry->score;
			}
			else if (entry->score < beta)
				beta = entry->score;

			if (alpha >= beta)
				return entry->score;
		}
	}

	long alpha0 = alpha;
	long beta0 = beta;

	std::vector<MoveList> &moves = _search_moves[remaining-1];
	moves.clear();
	collect_root_moves(board, player, &moves);

	if (moves.empty())
		return 0;

	// Try the transposition table's previous best move first; a re-visit
	// keeps the stored move ahead and skips the ordering sort below.
	if (hit && entry->best != 0)
	{
		for (unsigned int i = 0; i < moves.size(); i++)
		{
			if (moves[i].front() * GameBoard::SIZE + moves[i].back()
				== entry->best)
			{
				std::swap(moves[0], moves[i]);
				break;
			}
		}
	}
	else if (!hit)
	{
		// Move ordering: biggest gain for the player to move first.  This
		// happens to be the most promising move for both a maximising root
		// player and a minimising opponent, which is what alpha-beta needs.
		std::stable_sort(moves.begin(), moves.end(),
			[this, player](const MoveList &a, const MoveList &b)
			{
				long ga = _dist_to_goal[player][a.front()]
						 - _dist_to_goal[player][a.back()];
				long gb = _dist_to_goal[player][b.front()]
						 - _dist_to_goal[player][b.back()];
				return ga > gb;
			});
	}

	long best = maximizing ? LONG_MIN : LONG_MAX;
	uint32_t best_move = 0;

	for (unsigned int i = 0; i < moves.size(); i++)
	{
		MoveList &m = moves[i];
		unsigned int front = m.front();
		unsigned int back = m.back();

		board->move_peg(front, back);

		_current_depth = remaining;

		long d = score_this_move(board, player, &m);

		// A finishing move (or any big enough one) ends the branch: no
		// further plies are searched below it.
		long v = (maximizing ? d : -d) * remaining;
		if (d <= 9000 && remaining > 1)
			v += paranoid_search(board, board->get_next_player(player),
								 root, remaining - 1, alpha, beta);

		board->move_peg(back, front);

		if (maximizing)
		{
			if (v > best)
			{
				best = v;
				best_move = front * GameBoard::SIZE + back;
			}
			if (best > alpha)
				alpha = best;
		}
		else
		{
			if (v < best)
			{
				best = v;
				best_move = front * GameBoard::SIZE + back;
			}
			if (best < beta)
				beta = best;
		}

		if (alpha >= beta)
			break;
	}

	if (entry)
	{
		entry->key = key;
		entry->score = best;
		entry->best = best_move;
		entry->gen = _tt_gen;

		if (best <= alpha0)
			entry->flag = TT_UPPER;
		else if (best >= beta0)
			entry->flag = TT_LOWER;
		else
			entry->flag = TT_EXACT;
	}

	return best;
}


long BotLookAhead::score_this_move(GameBoard *board, 
								   unsigned int player,
								   MoveList *move)
{
	long progress = score_progress(board, player, move);

	// The root player's own progress may be discounted (Friendly divides by
	// _self_penalty = 2); the sportsmanship penalties are not scaled.
	if (player == _root_player)
		progress /= _self_penalty;

	return progress + goal_block_penalty(board, player, move)
				  + goal_exit_penalty(board, player, move);
}


long BotLookAhead::score_progress(GameBoard *board, 
								   unsigned int player,
								   MoveList *move)
{
	unsigned int front = move->front();
	unsigned int back = move->back();

	long from_dist = _dist_to_goal[player][front];
	long to_dist = _dist_to_goal[player][back];

	long total_score = from_dist - to_dist;

	if (board->player_finished(player))
	    total_score += 10000 + (2000 * _current_depth);
	else if (_current_depth == _depth && is_blocking_pegs(board, player))
		total_score -= 5000;

	return total_score;
}

