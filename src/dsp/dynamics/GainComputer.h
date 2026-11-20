// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <algorithm>

namespace lionvoice::dsp
{

/**
    The static curve of a compressor: for an input level (dBFS) it returns the output level, and so how many
    dB of gain reduction to apply. No timing here: attack and release are handled by Ballistics / OptoCell.

    Soft knee as in Giannoulis, Massberg & Reiss, "Digital Dynamic Range Compressor Design — A Tutorial and
    Analysis" (JAES, 2012). With threshold T, ratio R and knee width W:

        2(x - T) < -W   :  y = x                                     below the knee: untouched
        |2(x - T)| <= W :  y = x + (1/R - 1) (x - T + W/2)^2 / (2W)    inside the knee: smooth blend
        2(x - T) > W    :  y = T + (x - T) / R                       above the knee: slope 1/R

    The knee is centred on the threshold and the curve and its slope are continuous everywhere.
    Pure value type: real-time safe and cheap to copy.
*/
struct GainComputer
{
    float thresholdDb = 0.0f;
    float ratio = 4.0f;   // >= 1; use a large value (e.g. 1000) for a limiter
    float kneeDb = 6.0f;  // 0 = hard knee

    /** Output level in dB for an input level in dB. */
    float outputLevelDb (float inputDb) const noexcept
    {
        const float slope = 1.0f / std::max (ratio, 1.0f);
        const float overshoot = inputDb - thresholdDb;
        const float knee = std::max (kneeDb, 0.0f);

        if (2.0f * overshoot <= -knee)
            return inputDb;

        if (knee > 0.0f && 2.0f * overshoot < knee)
        {
            const float intoKnee = overshoot + knee * 0.5f;
            return inputDb + (slope - 1.0f) * intoKnee * intoKnee / (2.0f * knee);
        }

        return thresholdDb + overshoot * slope;
    }

    /** Gain change in dB (0 or negative) for an input level in dB. */
    float gainReductionDb (float inputDb) const noexcept { return outputLevelDb (inputDb) - inputDb; }
};

} // namespace lionvoice::dsp
