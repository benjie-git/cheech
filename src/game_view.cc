/*
 *   The GUI Chinese Checkers Board.
 *
 *  Copyright (C) 2005-2026 Ben Levitt
 *
 *  SPDX-License-Identifier: MIT
 */

#include <gdk/gdk.h>
#include "click_counter.hh"
#include <gtkmm/eventcontrollerkey.h>
#include <algorithm>
#include <sstream>
#include <cmath>

#include "game_view.hh"
#include "game_images.hh"
#include "utility.hh"
#include "bot_lookahead.hh"
#include "cheech_move_gate.hh"

#define PI 3.14159265359
#define MULTIPLIER 1.5


namespace {

struct BoardPoint
{
	double x;
	double y;
};

double cross(const BoardPoint& origin, const BoardPoint& a,
			 const BoardPoint& b)
{
	return (a.x - origin.x) * (b.y - origin.y)
		 - (a.y - origin.y) * (b.x - origin.x);
}

std::vector<BoardPoint> convex_hull(std::vector<BoardPoint> points)
{
	if (points.size() <= 2)
		return points;

	std::sort(points.begin(), points.end(),
		[](const BoardPoint& a, const BoardPoint& b) {
			return a.x == b.x ? a.y < b.y : a.x < b.x;
		});

	std::vector<BoardPoint> lower;
	for (const BoardPoint& point : points)
	{
		while (lower.size() >= 2
			&& cross(lower[lower.size() - 2], lower.back(), point) <= 0.0)
			lower.pop_back();
		lower.push_back(point);
	}

	std::vector<BoardPoint> upper;
	for (auto point = points.rbegin(); point != points.rend(); point++)
	{
		while (upper.size() >= 2
			&& cross(upper[upper.size() - 2], upper.back(), *point) <= 0.0)
			upper.pop_back();
		upper.push_back(*point);
	}

	lower.pop_back();
	upper.pop_back();
	lower.insert(lower.end(), upper.begin(), upper.end());
	return lower;
}

std::vector<BoardPoint> hex_board_vertices(
	const std::vector<BoardPoint>& points, double padding)
{
	std::vector<BoardPoint> vertices = convex_hull(points);
	if (vertices.size() != 6)
		return std::vector<BoardPoint>();

	double cx = 0.0, cy = 0.0;
	for (const BoardPoint& point : vertices)
	{
		cx += point.x;
		cy += point.y;
	}
	cx /= 6.0;
	cy /= 6.0;

	for (BoardPoint& point : vertices)
	{
		double dx = point.x - cx;
		double dy = point.y - cy;
		double distance = std::hypot(dx, dy);
		if (distance == 0.0)
			continue;

		double scale = (distance + padding) / distance;
		point.x = cx + dx * scale;
		point.y = cy + dy * scale;
	}

	return vertices;
}

void quadratic_to(const Cairo::RefPtr<Cairo::Context>& cr,
				  const BoardPoint& start, const BoardPoint& control,
				  const BoardPoint& end)
{
	double c1x = start.x + (control.x - start.x) * 2.0 / 3.0;
	double c1y = start.y + (control.y - start.y) * 2.0 / 3.0;
	double c2x = end.x + (control.x - end.x) * 2.0 / 3.0;
	double c2y = end.y + (control.y - end.y) * 2.0 / 3.0;
	cr->curve_to(c1x, c1y, c2x, c2y, end.x, end.y);
}

void rounded_polygon_path(const Cairo::RefPtr<Cairo::Context>& cr,
						  const std::vector<BoardPoint>& vertices,
						  double radius)
{
	if (vertices.size() < 3)
		return;

	cr->begin_new_path();
	if (radius <= 0.0)
	{
		cr->move_to(vertices[0].x, vertices[0].y);
		for (size_t i = 1; i < vertices.size(); i++)
			cr->line_to(vertices[i].x, vertices[i].y);
		cr->close_path();
		return;
	}

	std::vector<BoardPoint> entries(vertices.size());
	std::vector<BoardPoint> exits(vertices.size());

	for (size_t i = 0; i < vertices.size(); i++)
	{
		const BoardPoint& prev =
			vertices[(i + vertices.size() - 1) % vertices.size()];
		const BoardPoint& curr = vertices[i];
		const BoardPoint& next = vertices[(i + 1) % vertices.size()];

		double pvx = prev.x - curr.x;
		double pvy = prev.y - curr.y;
		double nvx = next.x - curr.x;
		double nvy = next.y - curr.y;
		double plen = std::hypot(pvx, pvy);
		double nlen = std::hypot(nvx, nvy);

		entries[i] = curr;
		exits[i] = curr;
		if (plen == 0.0 || nlen == 0.0)
			continue;

		double dot = (pvx * nvx + pvy * nvy) / (plen * nlen);
		dot = std::max(-1.0, std::min(1.0, dot));
		double angle = std::acos(dot);
		double tangent = angle > 1e-9
			? radius / std::tan(angle / 2.0) : 0.0;
		tangent = std::min(tangent, std::min(0.5 * plen, 0.5 * nlen));

		entries[i].x = curr.x + pvx / plen * tangent;
		entries[i].y = curr.y + pvy / plen * tangent;
		exits[i].x = curr.x + nvx / nlen * tangent;
		exits[i].y = curr.y + nvy / nlen * tangent;
	}

	cr->move_to(entries[0].x, entries[0].y);
	for (size_t i = 0; i < vertices.size(); i++)
	{
		if (i > 0)
			cr->line_to(entries[i].x, entries[i].y);
		quadratic_to(cr, entries[i], vertices[i], exits[i]);
	}
	cr->close_path();
}

} // namespace


game_view::~game_view()
{
	destroy_holes();
	delete _view_board;
}


game_view::game_view() : Gtk::DrawingArea()
{
	_client = NULL;
	_board = NULL;
	_view_board = new GameBoard(1, false, false, false);
	_locked = true;
	_center = GamePoint();
	_board_center = GamePoint();
	_scale = 1.0;
	_anim_step_ms = 0;
	_anim_done_ms = 0;
	_anim_start_us = 0;
	_anim_player = 0;
	_anim_pending_valid = false;
	_anim_x = 0.0;
	_anim_y = 0.0;
	_anim_tick_id = 0;

	double peg_x = GameImages::get_peg_size().get_x();
	double peg_y = GameImages::get_peg_size().get_y();
	double min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9;

	GameBoard b(1, false, false, false);
	for (unsigned int i = 0; i < b.get_size(); i++)
	{
		if (!b[i])
			continue;

		double x_offset = (util::even(i / GameBoard::SIZE_X) ? 0.5 : 0.0) + 1.0;
		double rx = ((i % GameBoard::SIZE_X) + x_offset) * peg_x * MULTIPLIER;
		double ry = (i / GameBoard::SIZE_X) * peg_y * MULTIPLIER
					* (sqrt(3.0) / 2.0);

		if (rx < min_x) min_x = rx;
		if (rx > max_x) max_x = rx;
		if (ry < min_y) min_y = ry;
		if (ry > max_y) max_y = ry;
	}

	// Pad the board bounding box so the background hugs the lattice (keeping
	// it close to square) instead of filling the whole (tall) widget.
	double pad = peg_x * MULTIPLIER * 0.8;

	_board_center = GamePoint((int)lround((min_x + max_x) / 2.0),
							   (int)lround((min_y + max_y) / 2.0));
	_base_width = (int)lround(max_x - min_x + 2.0 * pad);
	_base_height = (int)lround(max_y - min_y + 2.0 * pad);

	set_size_request(_base_width, _base_height);

	set_focusable(true);
	grab_focus();

	// Clicks (any button; double-click commits a move) and key presses.
	// ClickCounter counts successive clicks itself: GTK's built-in
	// counter does not always restart when the pointer moves between
	// clicks.
	auto click = ClickCounter::create();
	click->signal_counted().connect(sigc::mem_fun(*this,
		&game_view::on_view_clicked));
	add_controller(click);

	auto key = Gtk::EventControllerKey::create();
	key->signal_key_pressed().connect(sigc::mem_fun(*this,
		&game_view::on_view_key_pressed), false);
	add_controller(key);

	set_draw_func(sigc::mem_fun(*this, &game_view::on_draw));

	signal_resize().connect(sigc::mem_fun(*this, &game_view::on_resize));
}


void game_view::on_realize()
{
	// We need to call the base on_realize()
	Gtk::DrawingArea::on_realize();

	create_holes();
}


void game_view::on_resize(int width, int height)
{
	_center = GamePoint(width/2, height/2);

	double sx = (double)width / _base_width;
	double sy = (double)height / _base_height;
	_scale = (sx < sy) ? sx : sy;

	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->set_scale(_scale);

	queue_draw();

	evt_board_resized.emit();
}


void game_view::create_holes()
{
	// on_realize can fire again (re-realize); don't leak the old holes.
	if (!_holes.empty())
		return;

	GameBoard b(1, false, false, false);

	_holes.resize(b.get_size());

	for (unsigned int i=0; i < b.get_size(); i++)
	{
		if (b[i])
		{
			double x = GameImages::get_peg_size().get_x();
			double y = GameImages::get_peg_size().get_y();
			double x_offset = (util::even(i / GameBoard::SIZE_X) ? 0.5 : 0.0)
								+ 1.0;
			double y_offset = 0;
			GamePoint offset((int)(((i % GameBoard::SIZE_X)
				+ x_offset) * x * MULTIPLIER)-_board_center.get_x(),
				(int)(((i / GameBoard::SIZE_X) + y_offset) * y * MULTIPLIER 
				* (sqrt(3.0) / 2.0))-_board_center.get_y());
			
			_holes[i] = new GameViewHole(&_center, &offset);
			_holes[i]->set_scale(_scale);
		}
		else
			_holes[i] = NULL;
	}

	rebuild_board();
}


void game_view::destroy_holes()
{
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			delete *hole;
	_holes.resize(0);
}


void game_view::set_client(GameClient *c)
{
	_client = c;
}


void game_view::set_input_client(GameClient *c)
{
	_client = c;
	_board = (c && c->get_board()) ? c->get_board() : NULL;

	// The holes are only created on realize; a signal can arrive first.
	if (_holes.empty())
		return;

	// Re-point the holes at the new client, but keep drawing from the
	// lagged view board: its state is still valid (all seat clients hold
	// the same board) and any running or parked replay keeps running.
	for (unsigned int i = 0; i < _view_board->get_size(); i++)
		if (_holes[i])
			_holes[i]->setup(c, (*_view_board)[i]);

	update_move_button();
}


void game_view::rebuild_board()
{
	// The board is being replaced wholesale: drop any replay state.  If a
	// replay was actually running, let the turn label re-sync.
	bool was_active = stop_anim();
	_anim_pending_valid = false;

	// The holes are only created on realize; a signal can arrive first.
	if (_holes.empty())
		return;

	if (_client && _client->get_board())
	{
		_board = _client->get_board();

		// The view draws from its own lagged copy of the board so a
		// replay always plays over the exact state its move was made in.
		*_view_board = *_board;

		for (unsigned int i=0; i < _board->get_size(); i++)
			if (_holes[i])
				_holes[i]->setup(_client, (*_view_board)[i]);
	}
	else
	{
		for (unsigned int i=0; i < _holes.size(); i++)
			if (_holes[i])
				_holes[i]->setup(NULL, NULL);

		_board = NULL;
	}

	if (was_active)
		sig_anim_idle.emit();
}


void game_view::set_locked(bool l)
{
	_locked = l;
	update_move_button();
}


// Any button press clears/rebuilds the selection; double-click on the last
// hole of a path commits the move.  The click count comes from ClickCounter
// (see click_counter.hh).  A double click with no move in progress acts
// like a single click: rapid clicking while waiting for a turn must not
// swallow the first selection made after the board unlocks.
void game_view::on_view_clicked(unsigned int clicks, double x, double y)
{
	grab_focus();
	evt_user_action.emit();

	if (_locked)
		return;

	for (unsigned int i = 0; i < _holes.size(); i++)
	{
		if (_holes[i] && _holes[i]->in_bounds((int)x, (int)y))
		{
			if (clicks > 1 && _move_list.size() > 0)
			{
				// Double-click extends a path in progress, or commits it
				// when repeated on its last hole.
				if (_move_list.back() != i)
					read_move(i);
				if (_move_list.size() >= 2 && i == _move_list.back())
					write_move();
			}
			else
			{
				read_move(i);
			}

			return;
		}
	}
}


bool game_view::on_view_key_pressed(guint keyval, guint keycode,
	Gdk::ModifierType state)
{
	evt_user_action.emit();

	switch (keyval)
	{
		case GDK_KEY_Escape:
			if (!_locked)
			{
				erase_move();
				return true;
			}
		case GDK_KEY_Return:
		case GDK_KEY_space:
			if (!_locked)
			{
				write_move();
				return true;
			}
		default:
			if (state == Gdk::ModifierType(0) ||
				state == Gdk::ModifierType(GDK_SHIFT_MASK))
			{
				evt_unhandled_key.emit(keyval);
				return true;
			}
			break;
	}
	return false;
}


void game_view::read_move(unsigned int i)
{
	if (_locked)
		return;
	
	if (!move_list_contains(i))
	{
		// Clicking another of the player's pegs clears any path built so
		// far and starts a fresh selection from that peg.
		if (_move_list.size() > 0 &&
			(*_board)[i]->get_current_player() ==
			_client->get_my_player_number())
		{
			erase_move();
			_move_list.push_back(i);
			_client->show_move(&_move_list);
			return;
		}

		_move_list.push_back(i);
		if (!((_move_list.size() == 1 &&
			(*_board)[i]->get_current_player() ==
			_client->get_my_player_number()) ||
			_board->valid_move_list(_move_list, false)))
		{
			_move_list.pop_back();
		}
		else
		{
			_client->show_move(&_move_list);
		}
	} else {
		if (_move_list.back() == i)
		{
			_holes[_move_list.back()]->set_hilighted(false);
			_move_list.pop_back();
		}
		else
		{
			while (_move_list.size() > 0 && _move_list.back() != i)
			{
				_holes[_move_list.back()]->set_hilighted(false);
				_move_list.pop_back();
			}
		}
		if (_move_list.size() > 0 )
			_client->show_move(&_move_list);
		else
			_client->hide_move();
	}

	update_move_button();
}


void game_view::write_move()
{
	if (_locked)
		return;

	if (_move_list.size() > 1 )
	{
		_client->make_move(&_move_list);
		erase_move();
	}

	update_move_button();
}


void game_view::erase_move()
{
	if (_locked)
		return;

	while (!_move_list.empty())
	{
		if (!_holes.empty())
			_holes[_move_list.back()]->set_hilighted(false);
		_move_list.pop_back();
	}

	_client->hide_move();

	update_move_button();
}


// The Move overlay button is visible exactly while the local player has
// their own move path in progress.  Another player's telegraphed move also
// occupies _move_list (it arrives via show_move), so ownership of the path's
// first hole distinguishes a local selection from a telegraph.
void game_view::update_move_button()
{
	bool active = !_locked && !_move_list.empty() && _board && _client &&
		(*_board)[_move_list.front()]->get_current_player() ==
			_client->get_my_player_number();

	sig_move_path_changed.emit(active);
}


void game_view::show_move(MoveList *list)
{
	hide_move();

	if (_holes.empty())
		return;

	for (MoveList::iterator move = list->begin();
		move < list->end(); move++)
	{
		_holes[(*move)]->set_hilighted(true);
		_move_list.push_back((*move));
	}

	update_move_button();
}


void game_view::hide_move()
{
	while (!_move_list.empty())
	{
		_holes[_move_list.back()]->set_hilighted(false);
		_move_list.pop_back();
	}

	update_move_button();
}


// Queues a replay of a move the server has already applied.  The view
// board still shows the pre-move state; the moving peg is lifted from its
// source hole and glides along the path while the move is applied to the
// view board only when the replay lands, keeping every replay honest even
// when further moves have arrived meanwhile.
void game_view::replay_move(const MoveList &list, int step_ms, int done_ms)
{
	if (list.size() < 2 || step_ms <= 0 || !_client || !_view_board)
		return;

	if (_anim_tick_id)
	{
		// A replay is running: park this move and start it when the
		// current one lands, like the iOS app.  At most one move waits:
		// a newer arrival replaces it, and the superseded move is applied
		// to the view board silently, since it will never be animated.
		if (_anim_pending_valid)
			_view_board->make_move_list(_anim_pending.list);

		_anim_pending.list = list;
		_anim_pending.step_ms = step_ms;
		_anim_pending.done_ms = done_ms;
		_anim_pending_valid = true;

		// Hold bots off until this replay actually runs.  The start
		// re-anchors with the same window; extend_move_gate never
		// shortens, so the two together cover the wait.
		cheech::extend_move_gate(step_ms * ((int)list.size() - 1) + done_ms);
		return;
	}

	start_replay(list, step_ms, done_ms);
}


// Begins replaying one move over the view board, which is in the exact
// pre-move state: lift the moving peg from its source hole and animate it
// along the path.  The animation gate is anchored here, where the replay
// actually runs, so waiting bots commit exactly when the board is idle
// again -- not when the move happened to arrive.  Returns false when the
// move was applied silently instead: a peg missing on the view board
// cannot be drawn (the view board would fall behind), and neither can a
// peg whose player has left the game and taken their color with them
// (animating it would draw a ghost).
bool game_view::start_replay(const MoveList &list, int step_ms, int done_ms)
{
	unsigned int mover = ((*_view_board)[list.front()])
		? (*_view_board)[list.front()]->get_current_player() : 0;
	if (mover == 0 ||
		!_client || _client->get_player_color(mover) == 0)
	{
		_view_board->make_move_list(list);
		return false;
	}

	_anim_path = list;
	_anim_step_ms = step_ms;
	_anim_done_ms = done_ms;
	_anim_player = mover;
	_anim_start_us = g_get_monotonic_time();

	// The holes are only created on realize; a move can arrive first.
	if (_holes.empty())
	{
		_view_board->make_move_list(list);
		return false;
	}

	GamePoint start = _holes[list.front()]->get_offset();
	_anim_x = start.get_x();
	_anim_y = start.get_y();

	_holes[list.front()]->set_suppress_peg(true);

	_anim_tick_id = add_tick_callback(sigc::mem_fun(*this,
		&game_view::on_anim_tick));
	cheech::extend_move_gate(_anim_step_ms *
		((int)list.size() - 1) + _anim_done_ms);
	queue_draw();

	sig_anim_started.emit();
	return true;
}


// Tears down a running replay: removes the tick callback, un-suppresses
// the source hole, and applies the move to the view board silently.
// Returns true when a replay was actually in progress.
bool game_view::stop_anim()
{
	bool was_active = _anim_tick_id || !_anim_path.empty();

	if (_anim_tick_id)
	{
		remove_tick_callback(_anim_tick_id);
		_anim_tick_id = 0;
	}

	if (!_anim_path.empty())
	{
		if (_holes[_anim_path.front()])
			_holes[_anim_path.front()]->set_suppress_peg(false);
		_view_board->make_move_list(_anim_path);
		_anim_path.clear();
	}

	return was_active;
}


// Fast-forwards the view board to the true board: applies the running
// replay's move and the parked one, in order.  Used when the board is
// being replaced anyway (resync/undo/leave) or when the local player
// commits mid-replay and everything should snap to its final position.
void game_view::flush_replays()
{
	bool was_active = stop_anim();

	if (_anim_pending_valid)
	{
		_view_board->make_move_list(_anim_pending.list);
		_anim_pending_valid = false;
	}

	if (was_active)
		sig_anim_idle.emit();

	queue_draw();
}


// flush_replays() plus the move itself: the local player's own move
// lands on the view board at once, after everything queued before it.
void game_view::apply_move_now(const MoveList &list)
{
	flush_replays();

	if (_view_board && list.size() > 1)
		_view_board->make_move_list(list);
}


void game_view::finish_anim()
{
	bool was_active = stop_anim();

	// The parked replay starts only now, so the moves play one after
	// another without overlapping (like the iOS app).
	if (_anim_pending_valid)
	{
		MoveList next = _anim_pending.list;
		int next_step = _anim_pending.step_ms;
		int next_done = _anim_pending.done_ms;
		_anim_pending_valid = false;

		if (start_replay(next, next_step, next_done))
		{
			queue_draw();
			return;
		}
	}

	if (was_active)
		sig_anim_idle.emit();

	queue_draw();
}


// Position of the in-flight peg along the move path: progress is measured
// in hop units, each hop eased in and out.
GamePoint game_view::anim_point(double progress)
{
	int hops = (int)_anim_path.size() - 1;
	if (hops < 1)
		return _holes[_anim_path.front()]->get_offset();

	if (progress < 0.0)
		progress = 0.0;
	if (progress > hops)
		progress = hops;

	int seg = (progress >= hops) ? hops - 1 : (int)progress;
	double local = progress - seg;
	double t = (local < 0.5) ?
		4.0 * local * local * local :
		1.0 - pow(-2.0 * local + 2.0, 3.0) / 2.0;

	GamePoint a = _holes[_anim_path[seg]]->get_offset();
	GamePoint b = _holes[_anim_path[seg + 1]]->get_offset();

	return GamePoint((int)lround(a.get_x() +
		(b.get_x() - a.get_x()) * t),
		(int)lround(a.get_y() + (b.get_y() - a.get_y()) * t));
}


bool game_view::on_anim_tick(const Glib::RefPtr<Gdk::FrameClock>&)
{
	int hops = (int)_anim_path.size() - 1;
	double elapsed = (g_get_monotonic_time() - _anim_start_us) / 1000.0;
	double total = hops * _anim_step_ms + _anim_done_ms;

	if (elapsed >= total)
	{
		// Returning false removes the callback; clear the id first so
		// finish_anim() does not remove it twice.
		_anim_tick_id = 0;
		finish_anim();
		return false;
	}

	double progress = elapsed / (double)_anim_step_ms;
	GamePoint p = anim_point(progress);
	_anim_x = p.get_x();
	_anim_y = p.get_y();
	queue_draw();

	return true;
}


bool game_view::move_list_contains(unsigned int i)
{
	for (MoveList::iterator move = _move_list.begin();
		move < _move_list.end(); move++)
			if (*move == i)
				return true;

	return false;
}


void game_view::on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int, int)
{
	// Fill the widget with black (the letterbox area around the board),
	// then paint the board background as a rounded hexagon around the six
	// star points, centered in the available space.
	cr->set_source_rgb(0, 0, 0);
	cr->paint();

	double pad = GameImages::get_peg_size().get_x() * MULTIPLIER * 0.8;
	std::vector<BoardPoint> hole_points;
	hole_points.reserve(_holes.size());
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
	{
		if (!*hole)
			continue;

		GamePoint offset = (*hole)->get_offset();
		hole_points.push_back(
			{(double)offset.get_x(), (double)offset.get_y()});
	}

	std::vector<BoardPoint> hex = hex_board_vertices(hole_points, pad);
	if (!hex.empty())
	{
		for (BoardPoint& point : hex)
		{
			point.x = _center.get_x() + point.x * _scale;
			point.y = _center.get_y() + point.y * _scale;
		}

		cr->set_source_rgb(0.55, 0.40, 0.24);
		rounded_polygon_path(cr, hex, pad * _scale);
		cr->fill();
	}
	else
	{
		double bw = _base_width * _scale;
		double bh = _base_height * _scale;
		double bx = _center.get_x() - bw / 2.0;
		double by = _center.get_y() - bh / 2.0;
		double r = 24.0;
		if (r > bw / 2.0) r = bw / 2.0;
		if (r > bh / 2.0) r = bh / 2.0;

		cr->set_source_rgb(0.55, 0.40, 0.24);
		cr->begin_new_path();
		cr->arc(bx + r, by + r, r, PI, 1.5 * PI);
		cr->arc(bx + bw - r, by + r, r, 1.5 * PI, 2 * PI);
		cr->arc(bx + bw - r, by + bh - r, r, 0, 0.5 * PI);
		cr->arc(bx + r, by + bh - r, r, 0.5 * PI, PI);
		cr->close_path();
		cr->fill();
	}

	cr->save();
	cr->translate(_center.get_x(), _center.get_y());
	cr->scale(_scale, _scale);

	// Draw the lattice connecting adjacent holes (same edge rule as the
	// iOS board: each hole's forward neighbours, guarded by distance so
	// row boundaries don't wrap).
	double unit_x = GameImages::get_peg_size().get_x() * MULTIPLIER;
	cr->set_source_rgb(0.49, 0.34, 0.20);
	cr->set_line_width(std::max(1.0, unit_x * 0.06));
	cr->begin_new_path();
	for (unsigned int i = 0; i < _holes.size(); i++)
	{
		if (!_holes[i])
			continue;

		unsigned int row = i / GameBoard::SIZE_X;
		unsigned int candidates[3];
		unsigned int ncand;
		if (util::even(row))
		{
			candidates[0] = i + 1;
			candidates[1] = i + GameBoard::SIZE_X + 1;
			candidates[2] = i + GameBoard::SIZE_X;
			ncand = 3;
		}
		else
		{
			candidates[0] = i + 1;
			candidates[1] = i + GameBoard::SIZE_X;
			candidates[2] = i + GameBoard::SIZE_X - 1;
			ncand = 3;
		}

		GamePoint a = _holes[i]->get_offset();
		for (unsigned int c = 0; c < ncand; c++)
		{
			if (candidates[c] >= _holes.size() || !_holes[candidates[c]])
				continue;

			GamePoint b = _holes[candidates[c]]->get_offset();
			double dx = b.get_x() - a.get_x();
			double dy = b.get_y() - a.get_y();
			if (sqrt(dx * dx + dy * dy) >= unit_x * 1.1)
				continue;

			cr->move_to(a.get_x(), a.get_y());
			cr->line_to(b.get_x(), b.get_y());
		}
	}
	cr->stroke();

	// Draw all the holes
	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole)
				(*hole)->draw(cr);

	if (_move_list.size() >=2)
	{
		// Draw the arcs beteen holes on the movelist
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end()-1; move++)
				draw_move_arc(cr, _holes[*move]->get_offset(),
					_holes[*(move+1)]->get_offset());
	
		// Then redraw the holes on the move_list on top of the arcs
		for (MoveList::iterator move = _move_list.begin();
			move < _move_list.end(); move++)
				_holes[*move]->draw(cr);
	}

	// The peg in flight, drawn on top of everything else.
	if (_anim_tick_id && _client && !_anim_path.empty())
		GameViewHole::draw_peg_at(cr, _anim_x, _anim_y,
			_client->get_player_color(_anim_player));

	cr->restore();

	return;
}


void game_view::draw_move_arc(const Cairo::RefPtr<Cairo::Context>& cr,
							  GamePoint from, GamePoint to)
{
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(4);
	cr->move_to(from.get_x(), from.get_y());
	cr->line_to(to.get_x(), to.get_y());
	cr->stroke();
}


void game_view::rotate_to_local_player(unsigned int posn)
{
	// The caller normally guarantees a board, but a stray early call would
	// previously have crashed here.
	if (_board)
		rotate_to_seat(_board->get_num_players(), posn);
}


void game_view::rotate_to_seat(unsigned int num_players, unsigned int posn)
{
	g_assert(posn >= 1 && posn <= 6);

	double theta = 0;

	for (int i = 1; i <= 6; i++)
	{
		if (GameBoard::START_MAP[num_players][i] == posn)
		{
			theta = (1 - i) * PI / 3;
			break;
		}
	}

	for (std::vector<GameViewHole*>::iterator hole = _holes.begin();
		hole < _holes.end(); hole++)
			if (*hole != NULL)
				(*hole)->rotate(theta);
}
