/*
 *  Cross-thread "animation gate" for the iOS build of cheech.
 *
 *  Moves are applied to the core immediately, but the UI replays them as an
 *  animation.  To keep consecutive computer moves from overlapping, the bridge
 *  (which owns the animation timing) records the time until which an animation
 *  is expected to run.  A computer player refuses to commit its move while the
 *  gate is closed, so the next animation only starts once the previous one has
 *  finished.
 *
 *  The gate is a header-only atomic so both the bridge (CheechSession.mm) and
 *  the core bot code (bot_base.cc) share the same instance.
 */

#ifndef CHEECH_MOVE_GATE_HH
#define CHEECH_MOVE_GATE_HH

#include <atomic>
#include <chrono>

namespace cheech
{

inline std::atomic<long long>& move_gate_until()
{
	static std::atomic<long long> gate_until(0);
	return gate_until;
}

inline long long now_ms()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(
		steady_clock::now().time_since_epoch()).count();
}

// Extends the gate so it lasts at least `ms` from now.  Never shortens it,
// because several moves may be recorded before the animation starts.
inline void extend_move_gate(int ms)
{
	if (ms <= 0)
		return;

	long long until = now_ms() + ms;
	long long current = move_gate_until().load();

	while (until > current &&
		   !move_gate_until().compare_exchange_weak(current, until))
	{
		// `current` was refreshed by compare_exchange_weak.
	}
}

inline bool move_gate_open()
{
	return now_ms() >= move_gate_until().load();
}

} // namespace cheech

#endif /* CHEECH_MOVE_GATE_HH */
