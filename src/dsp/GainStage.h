// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Decibels.h"
#include "Smoother.h"

#include <algorithm>
#include <vector>

namespace lionvoice::dsp
{

/**
    A smoothed volume control, used for the IN and OUT faders.

    - Gain changes ramp over 20 ms, so moving a fader never clicks.
    - While ramping, the gain curve is computed once per block and applied identically to every
      channel (left and right always get the same gain).
    - At exactly 0 dB with no ramp in progress the audio is left untouched (bit-exact passthrough).

    prepare() allocates and must be called outside the audio thread (e.g. in prepareToPlay).
    setGainDb() and process() are real-time safe.
*/
class GainStage
{
public:
    static constexpr double rampSeconds = 0.020;

    GainStage() noexcept { smoother.setCurrentAndTarget (1.0f); }

    /** Prepares for playback and jumps straight to the latest gain (no fade-in when playback starts). */
    void prepare (double sampleRate, int maxBlockSize)
    {
        rampBuffer.assign (static_cast<std::size_t> (std::max (1, maxBlockSize)), 1.0f);
        smoother.reset (sampleRate, rampSeconds);
    }

    /** Sets the target gain in dB. Call once per block with the current parameter value. */
    void setGainDb (float gainDb) noexcept { smoother.setTarget (dbToGain (gainDb)); }

    /** Applies the gain in place to numChannels channels of numSamples samples each. */
    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        if (rampBuffer.empty()) // not prepared yet: leave the audio unchanged
            return;

        const int maxChunk = static_cast<int> (rampBuffer.size());

        // Hosts should never exceed the announced block size, but if one does we work in chunks.
        for (int offset = 0; offset < numSamples; offset += maxChunk)
            processChunk (channels, numChannels, offset, std::min (maxChunk, numSamples - offset));
    }

    float getCurrentGain() const noexcept { return smoother.getCurrent(); }
    bool isRamping() const noexcept { return smoother.isSmoothing(); }

private:
    void processChunk (float* const* channels, int numChannels, int offset, int numSamples) noexcept
    {
        if (! smoother.isSmoothing())
        {
            const float gain = smoother.getTarget();

            if (exactlyEqual (gain, 1.0f))
                return;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = channels[ch] + offset;
                for (int i = 0; i < numSamples; ++i)
                    data[i] *= gain;
            }
            return;
        }

        float* ramp = rampBuffer.data();
        for (int i = 0; i < numSamples; ++i)
            ramp[i] = smoother.getNext();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = channels[ch] + offset;
            for (int i = 0; i < numSamples; ++i)
                data[i] *= ramp[i];
        }
    }

    LinearSmoother smoother;
    std::vector<float> rampBuffer;
};

} // namespace lionvoice::dsp
