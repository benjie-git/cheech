/*
 *  Copyright (C) 2005-2026 Ben Levitt
 *  SPDX-License-Identifier: MIT
 *
 *  Minimal Glib::SignalProxy shim for the iOS build of cheech.
 *
 *  giomm uses Glib::SignalProxy for signals with a non-void return value
 *  (for instance Gio::SocketService::signal_incoming(), whose handler
 *  returns a bool to indicate whether the connection was consumed).  The
 *  compat sigc::signal only supports void returns, so this implements the
 *  small slice needed here: connect() plus an internal emit() that stops at
 *  the first handler returning true.
 */

#ifndef INCL_COMPAT_GLIBMM_SIGNALPROXY_H
#define INCL_COMPAT_GLIBMM_SIGNALPROXY_H

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <sigc++/sigc++.h>

namespace Glib {

template <class T_Signal> class SignalProxy;

template <class R, class... Args>
class SignalProxy<R(Args...)>
{
private:
	struct entry
	{
		std::function<R(Args...)> fn;
		std::shared_ptr<sigc::internal::trackable_state> state;
		bool active = true;
	};

public:
	SignalProxy()
		: _entries(std::make_shared<std::vector<std::shared_ptr<entry>>>())
	{
	}

	template <class F>
	sigc::connection connect(F&& f, bool /*after*/ = false)
	{
		// Extract the trackable state before the callable is moved into the
		// std::function below.
		std::shared_ptr<sigc::internal::trackable_state> state =
			sigc::internal::state_of(f, 0);

		std::shared_ptr<entry> e = std::make_shared<entry>();
		e->fn = std::function<R(Args...)>(std::forward<F>(f));
		e->state = state;
		_entries->push_back(e);

		std::weak_ptr<entry> weak = e;
		return sigc::connection([weak]()
		{
			if (std::shared_ptr<entry> slot = weak.lock())
				slot->active = false;
		});
	}

	bool emit(Args... args)
	{
		// Snapshot the entries so a handler that connects or disconnects
		// during the emission can't invalidate the iteration.
		std::vector<std::shared_ptr<entry>> snapshot = *_entries;
		for (std::shared_ptr<entry>& e : snapshot)
		{
			if (!e->active)
				continue;

			if (e->state && !e->state->alive.load())
			{
				e->active = false;
				continue;
			}

			if (e->fn(args...))
				return true;
		}

		return false;
	}

private:
	std::shared_ptr<std::vector<std::shared_ptr<entry>>> _entries;
};

} // namespace Glib

#endif   // #ifndef INCL_COMPAT_GLIBMM_SIGNALPROXY_H
