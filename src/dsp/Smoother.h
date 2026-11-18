// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{

/**
    Moves a value towards a target in a straight line over a fixed time.

    Changing a parameter (gain, filter frequency...) in one jump creates audible clicks ("zipper noise").
    The smoother spreads each change over a short ramp instead.

    Real-time safe: no allocation, no locks. Not thread-safe: use it from the audio thread only.

    Usage:
        prepare:   smoother.reset (sampleRate, 0.02);  smoother.setCurrentAndTarget (initial);
        on change: smoother.setTarget (newValue);
        per sample: const auto v = smoother.getNext();
*/
class LinearSmoother
{
public:
    /** Sets the ramp length. Jumps straight to the current target (no ramp in progress afterwards). */
    void reset (double sampleRate, double rampSeconds) noexcept
    {
        rampLength = std::max (1, static_cast<int> (std::lround (sampleRate * rampSeconds)));
        setCurrentAndTarget (target);
    }

    /** Jumps immediately to a value, without ramping. */
    void setCurrentAndTarget (float value) noexcept
    {
        current = target = value;
        step = 0.0f;
        remaining = 0;
    }

    /** Starts a ramp from the current value to a new target. */
    void setTarget (float newTarget) noexcept
    {
        if (newTarget == target)
            return;

        target = newTarget;
        remaining = rampLength;
        step = (target - current) / static_cast<float> (remaining);
    }

    /** Advances by one sample and returns the new value. */
    float getNext() noexcept
    {
        if (remaining <= 0)
            return target;

        --remaining;
        current = remaining == 0 ? target : clampToTarget (current + step);
        return current;
    }

    /** Advances by several samples at once (e.g. when a whole block shares one value). */
    void skip (int numSamples) noexcept
    {
        if (numSamples >= remaining)
        {
            setCurrentAndTarget (target);
            return;
        }

        remaining -= numSamples;
        current = clampToTarget (current + step * static_cast<float> (numSamples));
    }

    float getCurrent() const noexcept { return current; }
    float getTarget() const noexcept { return target; }
    bool isSmoothing() const noexcept { return remaining > 0; }

private:
    /** Guards against floating-point rounding pushing the value past the target. */
    float clampToTarget (float value) const noexcept
    {
        return step > 0.0f ? std::min (value, target) : std::max (value, target);
    }

    float current = 0.0f;
    float target = 0.0f;
    float step = 0.0f;
    int remaining = 0;
    int rampLength = 1;
};

} // namespace lionvoice::dsp
