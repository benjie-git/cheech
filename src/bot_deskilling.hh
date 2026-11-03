//
//    Shared de-skilling constants for bots and the setup UIs.
//
//    Copyright (C) 2005-2026 Ben Levitt
//    SPDX-License-Identifier: MIT
//

#ifndef CHEECH_BOT_DESKILLING_HH
#define CHEECH_BOT_DESKILLING_HH

#include <array>
#include <cstddef>

namespace BotDeskilling
{
	// Per-step skip chance for skill levels 1..4 (Best..Nerfed).
	constexpr double kSkillSkipChance[] = {0.0, 0.15, 0.40, 0.60};
	constexpr unsigned int kSkillLevelCount =
		sizeof(kSkillSkipChance) / sizeof(kSkillSkipChance[0]);

	constexpr double kDefaultSkipChance = 0.0;
	constexpr double kMinSkipChance = 0.0;
	constexpr double kMaxSkipChance = 1.0;

	// How strongly a full forward/backward step changes the skip chance.
	// Forward steps scale down to 0.75x, backward steps up to 4x.
	constexpr double kMaxForwardSkipScale = 0.75;
	constexpr double kMaxBackwardSkipScale = 4.0;

	// The look-ahead distance cache stores 50 units per hole spacing; this
	// spans six hole spacings either side of neutral, and larger moves clamp.
	constexpr long kProgressRange = 300;

	constexpr std::size_t kSkipMultiplierCount =
		static_cast<std::size_t>(2 * kProgressRange + 1);

	constexpr std::array<double, kSkipMultiplierCount>
	make_skip_multiplier_table()
	{
		std::array<double, kSkipMultiplierCount> table = {};

		for (std::size_t i = 0; i < kSkipMultiplierCount; i++)
		{
			long progress = static_cast<long>(i) - kProgressRange;

			if (progress == 0)
			{
				table[i] = 1.0;
				continue;
			}

			double normalized = (double)progress / (double)kProgressRange;
			table[i] = (normalized >= 0.0)
				? 1.0 - (1.0 - kMaxForwardSkipScale) * normalized
				: 1.0 - (kMaxBackwardSkipScale - 1.0) * normalized;
		}

		return table;
	}

	inline constexpr auto kSkipMultiplierTable =
		make_skip_multiplier_table();

	// Legacy `smarts` percentages (50..100) and their nearest current level.
	constexpr double kLegacyBestSmarts = 98.0;
	constexpr double kLegacyGreatSmarts = 85.0;
	constexpr double kLegacyMidSmarts = 75.0;

	inline double clamp_skip_chance(double chance)
	{
		if (chance < kMinSkipChance)
			return kMinSkipChance;
		if (chance > kMaxSkipChance)
			return kMaxSkipChance;
		return chance;
	}

	inline double skill_skip_chance(unsigned int level)
	{
		if (level < 1 || level > kSkillLevelCount)
			level = 1;
		return kSkillSkipChance[level - 1];
	}

	// Direction multiplier for a step, from 1/4x (forward) to 4x (backward).
	// This is a multiplier, not a probability.  It is a table lookup on
	// purpose: this is called for every searched step.
	inline double skip_multiplier(long progress)
	{
		if (progress < -kProgressRange)
			progress = -kProgressRange;
		else if (progress > kProgressRange)
			progress = kProgressRange;

		return kSkipMultiplierTable[
			static_cast<std::size_t>(progress + kProgressRange)];
	}

	// Per-step skip probability after applying the direction multiplier.
	// Always clamped to 0..1.  A zero chance is an exact value here (the
	// setter clamps to kMinSkipChance), so no epsilon is needed.
	inline double skip_scale(double chance, long progress)
	{
		if (chance <= 0.0)
			return 0.0;

		return clamp_skip_chance(chance * skip_multiplier(progress));
	}

	inline double legacy_smarts_skip_chance(double smarts)
	{
		if (smarts >= kLegacyBestSmarts)
			return kSkillSkipChance[0];
		if (smarts >= kLegacyGreatSmarts)
			return kSkillSkipChance[1];
		if (smarts >= kLegacyMidSmarts)
			return kSkillSkipChance[2];
		return kSkillSkipChance[3];
	}

	inline double normalize_skip_chance(double chance)
	{
		if (chance > kMaxSkipChance)
			return legacy_smarts_skip_chance(chance);
		return clamp_skip_chance(chance);
	}
}

#endif
