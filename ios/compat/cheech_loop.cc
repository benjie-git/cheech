/*
 *  A tiny poll()-based event loop for the iOS build of cheech.
 */

#include "cheech_loop.hh"

#include <algorithm>
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

namespace cheech {

Loop& Loop::instance()
{
	static Loop loop;
	return loop;
}

Loop::Loop()
{
	if (pipe(_wake_fds) == 0)
	{
		fcntl(_wake_fds[0], F_SETFL, O_NONBLOCK);
		fcntl(_wake_fds[1], F_SETFL, O_NONBLOCK);
		_wake_watch = add_fd(_wake_fds[0], true, false,
			[this](bool, bool, bool) { drain_wake(); });
	}
}

Loop::~Loop()
{
	stop();
	if (_wake_watch >= 0)
		remove_fd(_wake_watch);
	if (_wake_fds[0] >= 0)
		::close(_wake_fds[0]);
	if (_wake_fds[1] >= 0)
		::close(_wake_fds[1]);
}

void Loop::start()
{
	if (_running.exchange(true))
		return;

	_stop_requested.store(false);
	_thread = std::thread([this]() { run(); });
}

void Loop::stop()
{
	if (!_running.exchange(false))
		return;

	_stop_requested.store(true);
	wake();

	if (_thread.joinable() && std::this_thread::get_id() != _loop_thread_id)
		_thread.join();
}

bool Loop::on_loop_thread() const
{
	return std::this_thread::get_id() == _loop_thread_id;
}

int Loop::add_fd(int fd, bool read, bool write, FdCallback cb)
{
	int id = _next_fd_id++;
	Watch watch;
	watch.fd = fd;
	watch.read = read;
	watch.write = write;
	watch.cb = std::move(cb);
	_fds[id] = std::move(watch);
	return id;
}

void Loop::mod_fd(int id, bool read, bool write)
{
	auto it = _fds.find(id);
	if (it != _fds.end())
	{
		it->second.read = read;
		it->second.write = write;
	}
}

void Loop::remove_fd(int id)
{
	_fds.erase(id);
}

Loop::TimerId Loop::add_timer(TimerCallback cb, int ms)
{
	Timer timer;
	timer.id = _next_timer_id++;
	timer.interval_ms = ms < 0 ? 0 : ms;
	timer.due = std::chrono::steady_clock::now()
		+ std::chrono::milliseconds(timer.interval_ms);
	timer.cb = std::move(cb);
	_timers.push_back(std::move(timer));
	return _timers.back().id;
}

void Loop::remove_timer(TimerId id)
{
	_timers.erase(std::remove_if(_timers.begin(), _timers.end(),
		[id](const Timer& t) { return t.id == id; }), _timers.end());
}

void Loop::post(std::function<void()> fn)
{
	{
		std::lock_guard<std::mutex> lock(_post_mutex);
		_posts.push_back(std::move(fn));
	}
	wake();
}

void Loop::wake()
{
	if (_wake_fds[1] >= 0)
	{
		char byte = 'w';
		ssize_t ignored = ::write(_wake_fds[1], &byte, 1);
		(void)ignored;
	}
}

void Loop::drain_wake()
{
	if (_wake_fds[0] < 0)
		return;

	char buffer[64];
	while (::read(_wake_fds[0], buffer, sizeof(buffer)) > 0)
		;
}

void Loop::drain_posts()
{
	std::deque<std::function<void()>> posts;
	{
		std::lock_guard<std::mutex> lock(_post_mutex);
		posts.swap(_posts);
	}

	for (auto& fn : posts)
		fn();
}

void Loop::run()
{
	_loop_thread_id = std::this_thread::get_id();
	while (!_stop_requested.load())
		iteration(true);
}

bool Loop::iteration(bool may_block)
{
	if (_iterating)
		return false;

	_iterating = true;

	bool did_work = false;

	drain_posts();

	// Fire due timers.  Snapshot the ids first because callbacks may add or
	// remove timers.
	const auto now = std::chrono::steady_clock::now();
	std::vector<TimerId> due;
	for (const auto& timer : _timers)
		if (timer.due <= now)
			due.push_back(timer.id);

	for (TimerId id : due)
	{
		if (_active_timers.count(id))
			continue;

		auto it = std::find_if(_timers.begin(), _timers.end(),
			[id](const Timer& t) { return t.id == id; });
		if (it == _timers.end())
			continue;

		TimerCallback cb = it->cb;
		int interval = it->interval_ms;

		did_work = true;

		_active_timers.insert(id);
		bool repeat = cb();
		_active_timers.erase(id);

		auto current = std::find_if(_timers.begin(), _timers.end(),
			[id](const Timer& t) { return t.id == id; });
		if (current == _timers.end())
			continue;

		if (repeat)
			current->due = std::chrono::steady_clock::now()
				+ std::chrono::milliseconds(interval);
		else
			_timers.erase(current);
	}

	// Build the poll set.
	std::vector<pollfd> pollfds;
	std::vector<int> ids;
	pollfds.reserve(_fds.size());
	ids.reserve(_fds.size());

	for (const auto& pair : _fds)
	{
		const Watch& watch = pair.second;
		if (watch.fd < 0)
			continue;

		pollfd pfd;
		pfd.fd = watch.fd;
		pfd.events = 0;
		if (watch.read)
			pfd.events |= POLLIN;
		if (watch.write)
			pfd.events |= POLLOUT;
		pfd.revents = 0;
		pollfds.push_back(pfd);
		ids.push_back(pair.first);
	}

	int timeout = 0;
	if (may_block)
	{
		timeout = 1000;

		const auto current = std::chrono::steady_clock::now();
		for (const auto& timer : _timers)
		{
			auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
				timer.due - current).count();
			if (remaining < 0)
				remaining = 0;
			if (remaining < timeout)
				timeout = static_cast<int>(remaining);
		}
	}

	if (pollfds.empty())
	{
		if (may_block && timeout > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
	}
	else
	{
		int result = ::poll(pollfds.data(), (uint)pollfds.size(), timeout);
		if (result > 0)
		{
			for (size_t i = 0; i < pollfds.size(); ++i)
			{
				if (pollfds[i].revents == 0)
					continue;

				auto it = _fds.find(ids[i]);
				if (it == _fds.end())
					continue;

				if (_active_fds.count(ids[i]))
					continue;

				Watch watch = it->second;
				bool rd = (pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)) != 0;
				bool wr = (pollfds[i].revents & POLLOUT) != 0;
				bool err = (pollfds[i].revents & (POLLERR | POLLNVAL)) != 0;

				did_work = true;

				_active_fds.insert(ids[i]);
				watch.cb(rd, wr, err);
				_active_fds.erase(ids[i]);
			}
		}
	}

	_iterating = false;

	return did_work;
}

} // namespace cheech
