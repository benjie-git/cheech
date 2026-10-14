#ifndef INCL_COMPAT_GIOMM_ASYNCRESULT_H
#define INCL_COMPAT_GIOMM_ASYNCRESULT_H

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

namespace Gio {

class AsyncResult
{
public:
	bool failed = false;
	int code = 0;
	Glib::ustring message;
};

// Counterpart of the real giomm SlotAsyncReady (sigc::slot<void(RefPtr<AsyncResult>&)>).
// Accepts the same mem_fun/bind wrappers as the compat sigc::signal, and
// carries the originating object's liveness so a callback is dropped once its
// object has been destroyed.
class SlotAsyncReady
{
public:
	SlotAsyncReady() = default;

	template <class F,
		std::enable_if_t<!std::is_base_of<SlotAsyncReady,
			std::decay_t<F>>::value, int> = 0>
	SlotAsyncReady(F&& f)
		: _state(sigc::internal::state_of(f, 0)),
		  _caller(std::forward<F>(f))
	{
	}

	bool alive() const
	{
		return !_state || _state->alive.load();
	}

	void operator()(Glib::RefPtr<AsyncResult>& result) const
	{
		if (_caller && alive())
			_caller(result);
	}

private:
	std::shared_ptr<sigc::internal::trackable_state> _state;
	std::function<void(Glib::RefPtr<AsyncResult>&)> _caller;
};

} // namespace Gio

#endif   // #ifndef INCL_COMPAT_GIOMM_ASYNCRESULT_H
