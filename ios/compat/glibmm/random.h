/*
 *  Minimal Glib::Rand shim for the iOS build of cheech.
 *
 *  The core uses Glib::Rand only for get_int_range(), to pick uniformly
 *  among equally good bot moves, so an mt19937 with a uniform distribution
 *  is a faithful replacement.
 */

#ifndef CHEECH_COMPAT_GLIBMM_RANDOM_H
#define CHEECH_COMPAT_GLIBMM_RANDOM_H

#include <random>

namespace Glib {

class Rand
{
public:
	Rand()
	{
		std::random_device device;
		_engine.seed(device());
	}

	explicit Rand(unsigned int seed)
	{
		_engine.seed(seed);
	}

	// Returns a value in [begin, end).
	int get_int_range(int begin, int end)
	{
		if (end <= begin)
			return begin;

		std::uniform_int_distribution<int> dist(begin, end - 1);
		return dist(_engine);
	}

private:
	std::mt19937 _engine;
};

} // namespace Glib

#endif /* CHEECH_COMPAT_GLIBMM_RANDOM_H */
