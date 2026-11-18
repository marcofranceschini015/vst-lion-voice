// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{

/** Level treated as silence. Anything at or below it maps to a gain of 0. */
inline constexpr float minusInfinityDb = -100.0f;

/** Converts decibels to a linear amplitude factor: gain = 10^(dB / 20). */
inline float dbToGain (float db) noexcept
{
    return db <= minusInfinityDb ? 0.0f : std::pow (10.0f, db * 0.05f);
}

/** Converts a linear amplitude factor to decibels, clamped at minusInfinityDb. */
inline float gainToDb (float gain) noexcept
{
    return gain > 0.0f ? std::max (minusInfinityDb, 20.0f * std::log10 (gain)) : minusInfinityDb;
}

} // namespace lionvoice::dsp
