/*
 *  Minimal libsigc++ shim for the iOS build of cheech.
 *
 *  The game core uses only a small slice of sigc++:
 *    - sigc::signal<void, ...> with connect()
 *    - sigc::mem_fun()
 *    - sigc::bind() / sigc::bind_return()
 *    - sigc::trackable, for automatic disconnection when an object dies
 *
 *  This header implements exactly that slice on top of std::function.
 *  Liveness is tracked with a shared state object owned by sigc::trackable;
 *  slots hold a weak reference and are skipped (and pruned) once the
 *  originating object is gone.
 */

#ifndef CHEECH_COMPAT_SIGCXX_SIGCXX_H
#define CHEECH_COMPAT_SIGCXX_SIGCXX_H

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace sigc {

namespace internal {

struct trackable_state
{
	std::atomic<bool> alive{true};
};

} // namespace internal

class trackable
{
public:
	trackable() : _state(std::make_shared<internal::trackable_state>()) {}
	trackable(const trackable&) : _state(std::make_shared<internal::trackable_state>()) {}
	trackable(trackable&&) noexcept : _state(std::make_shared<internal::trackable_state>()) {}
	trackable& operator=(const trackable&) { return *this; }
	trackable& operator=(trackable&&) noexcept { return *this; }
	virtual ~trackable()
	{
		if (_state)
			_state->alive.store(false);
	}

	std::shared_ptr<internal::trackable_state> _sigc_state() const { return _state; }

private:
	std::shared_ptr<internal::trackable_state> _state;
};

class connection
{
public:
	connection() = default;
	explicit connection(std::function<void()> disconnect)
		: _disconnect(std::move(disconnect)) {}
	connection(connection&&) noexcept = default;
	connection& operator=(connection&&) noexcept = default;
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;

	void disconnect()
	{
		if (_disconnect)
		{
			std::function<void()> fn = std::move(_disconnect);
			_disconnect = nullptr;
			fn();
		}
	}

	bool connected() const { return static_cast<bool>(_disconnect); }
	void block() {}
	void unblock() {}

private:
	std::function<void()> _disconnect;
};

namespace internal {

// Detect a slot's trackable state (present on mem_fun/bind wrappers).
template <class F>
auto state_of(const F& f, int) -> decltype(f._sigc_state())
{
	return f._sigc_state();
}

template <class F>
std::shared_ptr<trackable_state> state_of(const F&, long)
{
	return nullptr;
}

} // namespace internal

template <class T_return, class... T_arg>
class signal;

template <class... Args>
class signal<void, Args...>
{
private:
	struct entry
	{
		std::function<void(Args...)> fn;
		std::weak_ptr<internal::trackable_state> state;
		bool has_state = false;
		bool active = true;
	};

public:
	connection connect(std::function<void(Args...)> fn)
	{
		auto e = std::make_shared<entry>();
		e->fn = std::move(fn);
		_slots.push_back(e);
		std::weak_ptr<entry> weak = e;
		return connection([this, weak]()
		{
			if (auto slot = weak.lock())
				slot->active = false;
		});
	}

	template <class F>
	connection connect(F&& f)
	{
		auto e = std::make_shared<entry>();
		e->fn = std::function<void(Args...)>(std::forward<F>(f));
		if (auto state = internal::state_of(f, 0))
		{
			e->state = state;
			e->has_state = true;
		}
		_slots.push_back(e);
		std::weak_ptr<entry> weak = e;
		return connection([this, weak]()
		{
			if (auto slot = weak.lock())
				slot->active = false;
		});
	}

	void emit(Args... args) const
	{
		std::vector<std::shared_ptr<entry>> snapshot = _slots;

		for (auto& e : snapshot)
		{
			if (!e->active)
				continue;

			if (e->has_state)
			{
				auto state = e->state.lock();
				if (!state || !state->alive.load())
				{
					e->active = false;
					continue;
				}
			}

			e->fn(args...);
		}

		auto& slots = _slots;
		slots.erase(std::remove_if(slots.begin(), slots.end(),
			[](const std::shared_ptr<entry>& e) { return !e->active; }),
			slots.end());
	}

	void operator()(Args... args) const { emit(args...); }

private:
	mutable std::vector<std::shared_ptr<entry>> _slots;
};

namespace internal {

template <class T, class R, class... A>
class mem_fun_slot
{
public:
	mem_fun_slot(T* obj, R (T::*method)(A...))
		: _obj(obj), _method(method), _state(obj->_sigc_state()) {}

	R operator()(A... args) const
	{
		return (_obj->*_method)(std::forward<A>(args)...);
	}

	std::shared_ptr<trackable_state> _sigc_state() const { return _state; }

private:
	T* _obj;
	R (T::*_method)(A...);
	std::shared_ptr<trackable_state> _state;
};

template <class F, class Tuple, class... CallArgs, std::size_t... I>
auto invoke_bound(F& f, Tuple& t, std::index_sequence<I...>, CallArgs&&... args)
	-> decltype(f(std::forward<CallArgs>(args)..., std::get<I>(t)...))
{
	return f(std::forward<CallArgs>(args)..., std::get<I>(t)...);
}

template <class F, class... Bound>
class bind_slot
{
private:
	mutable F _f;
	mutable std::tuple<Bound...> _bound;

public:
	bind_slot(F f, Bound... bound)
		: _f(std::move(f)), _bound(std::move(bound)...) {}

	template <class... CallArgs>
	auto operator()(CallArgs&&... args) const
		-> decltype(invoke_bound(_f, _bound,
			std::index_sequence_for<Bound...>(),
			std::forward<CallArgs>(args)...))
	{
		return invoke_bound(_f, _bound,
			std::index_sequence_for<Bound...>(),
			std::forward<CallArgs>(args)...);
	}

	auto _sigc_state() const -> decltype(_f._sigc_state())
	{
		return _f._sigc_state();
	}
};

template <class F, class R>
class bind_return_slot
{
private:
	mutable F _f;
	R _value;

public:
	bind_return_slot(F f, R value)
		: _f(std::move(f)), _value(std::move(value)) {}

	template <class... A>
	R operator()(A&&... args) const
	{
		_f(std::forward<A>(args)...);
		return _value;
	}

	auto _sigc_state() const -> decltype(_f._sigc_state())
	{
		return _f._sigc_state();
	}
};

} // namespace internal

template <class T, class R, class... A>
internal::mem_fun_slot<T, R, A...> mem_fun(T& obj, R (T::*method)(A...))
{
	return internal::mem_fun_slot<T, R, A...>(&obj, method);
}

template <class T, class R, class... A>
internal::mem_fun_slot<T, R, A...> mem_fun(T* obj, R (T::*method)(A...))
{
	return internal::mem_fun_slot<T, R, A...>(obj, method);
}

template <class F, class... Bound>
internal::bind_slot<typename std::decay<F>::type,
					typename std::decay<Bound>::type...>
bind(F&& f, Bound&&... bound)
{
	return internal::bind_slot<typename std::decay<F>::type,
							   typename std::decay<Bound>::type...>(
		std::forward<F>(f), std::forward<Bound>(bound)...);
}

template <class F, class R>
internal::bind_return_slot<typename std::decay<F>::type,
						   typename std::decay<R>::type>
bind_return(F&& f, R&& value)
{
	return internal::bind_return_slot<typename std::decay<F>::type,
									  typename std::decay<R>::type>(
		std::forward<F>(f), std::forward<R>(value));
}

} // namespace sigc

#endif /* CHEECH_COMPAT_SIGCXX_SIGCXX_H */
