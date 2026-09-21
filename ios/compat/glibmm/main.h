/*
 *  Minimal glibmm main-loop shim for the iOS build of cheech.
 *
 *  Glib::signal_timeout() schedules a callback on the cheech event loop and
 *  stops automatically once the owning object (if it is sigc::trackable) is
 *  destroyed, mirroring glibmm's behaviour.
 */

#ifndef CHEECH_COMPAT_GLIBMM_MAIN_H
#define CHEECH_COMPAT_GLIBMM_MAIN_H

#include <memory>

#include <sigc++/sigc++.h>

#include "cheech_loop.hh"

#include <unistd.h>

namespace Glib {

class MainContext
{
public:
	static MainContext* get_default()
	{
		static MainContext context;
		return &context;
	}

	bool iteration(bool may_block)
	{
		return cheech::Loop::instance().iteration(may_block);
	}
};

inline void usleep(unsigned long usec)
{
	::usleep(usec);
}

class signal_timeout_t
{
public:
	template <class F>
	sigc::connection connect(F f, unsigned int ms, int priority = 0)
	{
		(void)priority;

		std::shared_ptr<sigc::internal::trackable_state> state =
			sigc::internal::state_of(f, 0);
		bool has_state = static_cast<bool>(state);

		auto callback = [f, state, has_state]() mutable -> bool
		{
			if (has_state && (!state || !state->alive.load()))
				return false;

			return f() ? true : false;
		};

		auto id = std::make_shared<cheech::Loop::TimerId>(
			cheech::Loop::instance().add_timer(callback, static_cast<int>(ms)));

		return sigc::connection([id]()
		{
			cheech::Loop::instance().remove_timer(*id);
		});
	}
};

inline signal_timeout_t signal_timeout()
{
	return signal_timeout_t();
}

} // namespace Glib

#endif /* CHEECH_COMPAT_GLIBMM_MAIN_H */
