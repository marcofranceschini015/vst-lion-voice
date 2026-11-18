// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <cmath>
#include <numbers>
#include <vector>

namespace lionvoice::test
{

/** A sine wave of the given frequency and amplitude. */
inline std::vector<float> makeSine (double sampleRate, double frequency, double seconds, float amplitude = 0.5f)
{
    const auto numSamples = static_cast<std::size_t> (sampleRate * seconds);
    std::vector<float> signal (numSamples);

    for (std::size_t i = 0; i < numSamples; ++i)
        signal[i] = amplitude
                  * static_cast<float> (std::sin (2.0 * std::numbers::pi * frequency * static_cast<double> (i)
                                                  / sampleRate));

    return signal;
}

/** RMS level of the samples in [start, end). */
inline double rms (const std::vector<float>& signal, std::size_t start, std::size_t end)
{
    double sum = 0.0;

    for (std::size_t i = start; i < end; ++i)
        sum += static_cast<double> (signal[i]) * static_cast<double> (signal[i]);

    return std::sqrt (sum / static_cast<double> (end - start));
}

} // namespace lionvoice::test
