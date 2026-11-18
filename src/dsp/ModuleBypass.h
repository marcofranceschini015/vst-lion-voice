// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Smoother.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace lionvoice::dsp
{

/**
    Click-free bypass switch, shared by every module of the chain.

    - Active:     only the module runs (no extra work).
    - Bypassed:   the module doesn't run at all and the audio passes bit-exact; the module is reset once,
                  so it restarts clean when re-enabled.
    - Switching:  a 10 ms crossfade between the dry signal and the processed one.

    prepare() allocates and must be called outside the audio thread. process() is real-time safe.
*/
class ModuleBypass
{
public:
    static constexpr double fadeSeconds = 0.010;
    static constexpr int maxSupportedChannels = 8;

    ModuleBypass() noexcept { fade.setCurrentAndTarget (1.0f); }

    void prepare (double sampleRate, int maxBlockSize, int maxChannels)
    {
        blockSize = std::max (1, maxBlockSize);
        numDryChannels = std::clamp (maxChannels, 1, maxSupportedChannels);
        dry.assign (static_cast<std::size_t> (blockSize * numDryChannels), 0.0f);

        fade.reset (sampleRate, fadeSeconds); // jumps to the current target: no fade when playback starts
    }

    void setBypassed (bool shouldBeBypassed) noexcept { fade.setTarget (shouldBeBypassed ? 0.0f : 1.0f); }

    bool isBypassed() const noexcept { return exactlyEqual (fade.getTarget(), 0.0f); }

    /**
        Runs one block through the bypass.
        processModule(float* const* channels, int numChannels, int numSamples) processes audio in place.
        resetModule() clears the module's memory.
    */
    template <typename ProcessFn, typename ResetFn>
    void process (float* const* channels, int numChannels, int numSamples, ProcessFn&& processModule,
                  ResetFn&& resetModule) noexcept
    {
        if (dry.empty()) // not prepared: no crossfade possible, switch immediately
            fade.setCurrentAndTarget (fade.getTarget());

        if (! fade.isSmoothing())
        {
            if (isBypassed())
            {
                if (! moduleIsReset)
                {
                    resetModule();
                    moduleIsReset = true;
                }
                return;
            }

            moduleIsReset = false;
            processModule (channels, numChannels, numSamples);
            return;
        }

        moduleIsReset = false;

        for (int offset = 0; offset < numSamples; offset += blockSize)
            crossfadeChunk (channels, numChannels, offset, std::min (blockSize, numSamples - offset), processModule);
    }

private:
    template <typename ProcessFn>
    void crossfadeChunk (float* const* channels, int numChannels, int offset, int numSamples,
                         ProcessFn& processModule) noexcept
    {
        numChannels = std::min (numChannels, maxSupportedChannels);
        const int fadedChannels = std::min (numChannels, numDryChannels);

        std::array<float*, maxSupportedChannels> chunk {};
        for (int ch = 0; ch < numChannels; ++ch)
            chunk[static_cast<std::size_t> (ch)] = channels[ch] + offset;

        for (int ch = 0; ch < fadedChannels; ++ch)
            std::copy_n (chunk[static_cast<std::size_t> (ch)], numSamples, dryChannel (ch));

        processModule (chunk.data(), numChannels, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const float wet = fade.getNext();

            for (int ch = 0; ch < fadedChannels; ++ch)
            {
                float& sample = chunk[static_cast<std::size_t> (ch)][i];
                const float drySample = dryChannel (ch)[i];
                sample = drySample + wet * (sample - drySample);
            }
        }
    }

    float* dryChannel (int channel) noexcept { return dry.data() + static_cast<std::size_t> (channel * blockSize); }

    LinearSmoother fade; // 1 = module fully in, 0 = fully bypassed
    std::vector<float> dry;
    int blockSize = 0;
    int numDryChannels = 0;
    bool moduleIsReset = false;
};

} // namespace lionvoice::dsp
