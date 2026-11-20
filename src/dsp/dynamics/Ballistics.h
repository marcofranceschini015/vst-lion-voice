// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <cmath>

namespace lionvoice::dsp
{

/**
    Attack / release timing of a compressor, applied to the gain-reduction signal in dB.

    "Smooth branching" one-pole filter (Giannoulis, Massberg & Reiss, JAES 2012):
        gr = a * gr + (1 - a) * target,   a = attack coefficient while the reduction grows (target below gr),
                                          a = release coefficient while it recovers.

    Times are time constants: after one attack time the reduction has covered 63 % of the way to the target.
    Real-time safe; one instance per compressor (stereo-linked: one gain for all channels).
*/
class Ballistics
{
public:
    /** One-pole coefficient for a time constant; 0 seconds = instant. */
    static float coefficient (double seconds, double sampleRate) noexcept
    {
        return seconds > 0.0 ? static_cast<float> (std::exp (-1.0 / (seconds * sampleRate))) : 0.0f;
    }

    Ballistics() noexcept { setTimes (attackSeconds, releaseSeconds); }

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        setTimes (attackSeconds, releaseSeconds);
        reset();
    }

    void setTimes (double newAttackSeconds, double newReleaseSeconds) noexcept
    {
        attackSeconds = newAttackSeconds;
        releaseSeconds = newReleaseSeconds;
        attackCoefficient = coefficient (attackSeconds, sampleRate);
        releaseCoefficient = coefficient (releaseSeconds, sampleRate);
    }

    /** Advances one sample towards the target gain reduction (dB, <= 0) and returns the smoothed value. */
    float process (float targetDb) noexcept
    {
        const float a = targetDb < state ? attackCoefficient : releaseCoefficient;
        state = a * state + (1.0f - a) * targetDb;
        return state;
    }

    void reset (float valueDb = 0.0f) noexcept { state = valueDb; }
    float getCurrent() const noexcept { return state; }

private:
    double sampleRate = 48000.0;
    double attackSeconds = 0.01;
    double releaseSeconds = 0.1;
    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float state = 0.0f;
};

} // namespace lionvoice::dsp
