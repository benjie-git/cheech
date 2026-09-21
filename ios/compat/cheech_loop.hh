/*
 *  A tiny poll()-based event loop for the iOS build of cheech.
 *
 *  The desktop game relies on the GLib main loop for socket readiness and
 *  timeouts.  On iOS the whole game core (server, clients and bots) runs on
 *  a single dedicated thread driven by this loop; SwiftUI talks to it via
 *  post().  This keeps every core object on one thread, matching the
 *  single-threaded assumptions of the original code.
 */

#ifndef CHEECH_COMPAT_LOOP_HH
#define CHEECH_COMPAT_LOOP_HH

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cheech {

class Loop
{
public:
	typedef std::function<void(bool readable, bool writable, bool error)> FdCallback;
	typedef std::function<bool()> TimerCallback;
	typedef unsigned long long TimerId;

	static Loop& instance();

	void start();
	void stop();

	bool on_loop_thread() const;
	bool running() const { return _running.load(); }

	int add_fd(int fd, bool read, bool write, FdCallback cb);
	void mod_fd(int id, bool read, bool write);
	void remove_fd(int id);

	// The callback returns true to keep rescheduling, false to stop.
	TimerId add_timer(TimerCallback cb, int ms);
	void remove_timer(TimerId id);

	// Thread-safe: queue work to run on the loop thread.
	void post(std::function<void()> fn);

	// Run a single pass.  Returns true if any work was performed.
	bool iteration(bool may_block);

private:
	Loop();
	~Loop();
	Loop(const Loop&) = delete;
	Loop& operator=(const Loop&) = delete;

	void run();
	void drain_posts();
	void wake();
	void drain_wake();

	struct Watch
	{
		int fd = -1;
		bool read = false;
		bool write = false;
		FdCallback cb;
	};

	struct Timer
	{
		TimerId id = 0;
		std::chrono::steady_clock::time_point due;
		int interval_ms = 0;
		TimerCallback cb;
	};

	std::unordered_map<int, Watch> _fds;
	int _next_fd_id = 1;

	std::vector<Timer> _timers;
	TimerId _next_timer_id = 1;

	// Sources currently dispatching.  Re-entrant iteration (for example the
	// core's util::delay_ms, which pumps the main context from inside a bot
	// timer callback) must not re-fire the source that is already running.
	std::unordered_set<int> _active_fds;
	std::unordered_set<TimerId> _active_timers;

	// Set while iteration() is executing.  The core's util::delay_ms pumps the
	// main context from deep inside a bot's recursive search; letting that
	// nested pump dispatch further callbacks makes the search re-enter itself
	// and overflow the stack.  Nested pumps become no-ops, so delay_ms turns
	// into a plain sleep.
	bool _iterating = false;

	std::mutex _post_mutex;
	std::deque<std::function<void()>> _posts;

	int _wake_fds[2] = {-1, -1};
	int _wake_watch = -1;

	std::thread _thread;
	std::atomic<bool> _running{false};
	std::atomic<bool> _stop_requested{false};
	std::thread::id _loop_thread_id;
};

} // namespace cheech

#endif /* CHEECH_COMPAT_LOOP_HH */
