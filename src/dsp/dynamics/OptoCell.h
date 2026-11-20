// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Ballistics.h"

#include <algorithm>

namespace lionvoice::dsp
{

/**
    Timing model of an LA-2A style optical gain cell (electro-luminescent panel + light-dependent resistor).

    The real cell is slow and has memory, which is what makes an opto compressor sound smooth:
      - attack about 10 ms;
      - release in two stages: about half of the reduction recovers in ~60 ms, the rest over 0.5 ... 5 s;
      - the longer and harder it has been compressing, the slower that second stage gets.

    Model: the gain reduction is the average of two halves that follow the same target with the same attack but
    different releases (fast: 60 ms; slow: 0.5 ... 5 s). A "memory" value follows the amount of reduction with a
    10 s time constant and sets the slow release time.

    Works on gain reduction in dB (0 or negative). Real-time safe.
*/
class OptoCell
{
public:
    static constexpr double attackSeconds = 0.010;
    static constexpr double fastReleaseSeconds = 0.060;
    static constexpr double slowReleaseMinSeconds = 0.5;
    static constexpr double slowReleaseMaxSeconds = 5.0;
    static constexpr double memorySeconds = 10.0;
    static constexpr float memoryReferenceDb = 6.0f; // sustained reduction that fully "charges" the memory

    OptoCell() noexcept { prepare (48000.0); }

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        attack = Ballistics::coefficient (attackSeconds, sampleRate);
        fastRelease = Ballistics::coefficient (fastReleaseSeconds, sampleRate);
        memoryCoefficient = Ballistics::coefficient (memorySeconds, sampleRate);
        reset();
    }

    void reset() noexcept
    {
        fast = 0.0f;
        slow = 0.0f;
        memory = 0.0f;
    }

    /** Advances one sample towards the target gain reduction (dB, <= 0); returns the cell's reduction. */
    float process (float targetDb) noexcept
    {
        fast = follow (fast, targetDb, fastRelease);
        slow = follow (slow, targetDb, slowReleaseCoefficient());

        const float reduction = 0.5f * (fast + slow);

        // Memory charges towards (reduction / reference) and discharges with a 10 s time constant.
        memory = memoryCoefficient * memory + (1.0f - memoryCoefficient) * (-reduction / memoryReferenceDb);
        return reduction;
    }

    float getReductionDb() const noexcept { return 0.5f * (fast + slow); }

    /** 0 = rested cell, 1 = fully charged (slowest release). */
    float getMemory() const noexcept { return std::clamp (memory, 0.0f, 1.0f); }

    /** Current time constant of the slow release stage. */
    double getSlowReleaseSeconds() const noexcept
    {
        return slowReleaseMinSeconds + (slowReleaseMaxSeconds - slowReleaseMinSeconds) * static_cast<double> (getMemory());
    }

private:
    float follow (float state, float target, float releaseCoefficient) const noexcept
    {
        const float a = target < state ? attack : releaseCoefficient;
        return a * state + (1.0f - a) * target;
    }

    /**
        Recomputed every sample because the memory changes continuously. exp() is replaced by its first-order
        approximation, which is exact to better than 1e-9 for time constants of 0.5 s and longer.
    */
    float slowReleaseCoefficient() const noexcept
    {
        return static_cast<float> (1.0 - 1.0 / (getSlowReleaseSeconds() * sampleRate));
    }

    double sampleRate = 48000.0;
    float attack = 0.0f;
    float fastRelease = 0.0f;
    float memoryCoefficient = 0.0f;

    float fast = 0.0f;
    float slow = 0.0f;
    float memory = 0.0f;
};

} // namespace lionvoice::dsp
