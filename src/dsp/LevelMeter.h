// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>

namespace lionvoice::dsp
{

/**
    Measures audio levels on the audio thread and hands them to the GUI without locks.

    Per channel (up to 2):
      - peak:  the highest absolute sample since the GUI last asked (so no short peak is ever missed,
               however slowly the GUI refreshes);
      - RMS:   average loudness over about 300 ms (exponential averaging);
      - clip:  sticky flag, set when a sample reaches 0 dBFS, cleared by the GUI.

    process() is real-time safe. The take/get/reset functions are meant for the GUI thread.
*/
class LevelMeter
{
public:
    static constexpr int maxChannels = 2;
    static constexpr double rmsTimeSeconds = 0.3;
    static constexpr float clipLevel = 1.0f; // 0 dBFS

    static_assert (std::atomic<float>::is_always_lock_free, "level meter needs lock-free atomics");

    void prepare (double sampleRate) noexcept
    {
        rmsCoefficient = 1.0 - std::exp (-1.0 / (rmsTimeSeconds * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        for (std::size_t ch = 0; ch < maxChannels; ++ch)
        {
            meanSquare[ch] = 0.0;
            peak[ch].store (0.0f);
            rms[ch].store (0.0f);
            clipped[ch].store (false);
        }
    }

    /** Audio thread: measures one block. */
    void process (const float* const* channels, int numChannels, int numSamples) noexcept
    {
        numChannels = std::clamp (numChannels, 0, maxChannels);
        activeChannels.store (numChannels, std::memory_order_relaxed);

        for (int c = 0; c < numChannels; ++c)
        {
            const auto ch = static_cast<std::size_t> (c);
            const float* data = channels[c];
            float blockPeak = 0.0f;
            double ms = meanSquare[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const double x = data[i];
                blockPeak = std::max (blockPeak, std::abs (data[i]));
                ms += rmsCoefficient * (x * x - ms);
            }

            meanSquare[ch] = ms;
            rms[ch].store (static_cast<float> (std::sqrt (ms)), std::memory_order_relaxed);
            raiseTo (peak[ch], blockPeak);

            if (blockPeak >= clipLevel)
                clipped[ch].store (true, std::memory_order_relaxed);
        }
    }

    // GUI thread ---------------------------------------------------------------

    /** Highest absolute sample since the previous call (linear, 1.0 = 0 dBFS). */
    float takePeak (int channel) noexcept { return peak[index (channel)].exchange (0.0f, std::memory_order_relaxed); }

    /** Current RMS level (linear). */
    float getRms (int channel) const noexcept { return rms[index (channel)].load (std::memory_order_relaxed); }

    bool hasClipped (int channel) const noexcept { return clipped[index (channel)].load (std::memory_order_relaxed); }

    void resetClip() noexcept
    {
        for (auto& flag : clipped)
            flag.store (false, std::memory_order_relaxed);
    }

    /** Channels seen in the last processed block (1 = mono, 2 = stereo). */
    int getNumChannels() const noexcept { return activeChannels.load (std::memory_order_relaxed); }

private:
    static std::size_t index (int channel) noexcept
    {
        return static_cast<std::size_t> (std::clamp (channel, 0, maxChannels - 1));
    }

    /** Lock-free "store the maximum". */
    static void raiseTo (std::atomic<float>& target, float value) noexcept
    {
        float current = target.load (std::memory_order_relaxed);
        while (value > current && ! target.compare_exchange_weak (current, value, std::memory_order_relaxed))
        {
        }
    }

    double rmsCoefficient = 1.0 - std::exp (-1.0 / (rmsTimeSeconds * 48000.0));
    std::array<double, maxChannels> meanSquare {}; // audio thread only

    std::array<std::atomic<float>, maxChannels> peak {};
    std::array<std::atomic<float>, maxChannels> rms {};
    std::array<std::atomic<bool>, maxChannels> clipped {};
    std::atomic<int> activeChannels { maxChannels };
};

} // namespace lionvoice::dsp
