// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "CharacterCompressor.h"
#include "BiquadDesign.h"

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{
namespace
{
constexpr float silenceLevel = 1.0e-6f; // -120 dBFS, keeps log10 finite
constexpr float ln10Over20 = 0.11512925f;
constexpr double mixSmoothingSeconds = 0.020;
} // namespace

CharacterCompressor::CharacterCompressor() noexcept
{
    mix.setCurrentAndTarget (current.mix);
}

GainComputer CharacterCompressor::curveFor (FetRatio ratio) noexcept
{
    switch (ratio)
    {
        case FetRatio::r4:
            return { thresholdDb, 4.0f, 6.0f };
        case FetRatio::r8:
            return { thresholdDb, 8.0f, 4.0f };
        case FetRatio::r12:
            return { thresholdDb, 12.0f, 2.0f };
        case FetRatio::r20:
            return { thresholdDb, 20.0f, 1.0f };
        case FetRatio::allButtons:
            return { thresholdDb, 20.0f, 0.0f };
    }
    return { thresholdDb, 4.0f, 6.0f };
}

float CharacterCompressor::staticGainReductionDb (FetRatio ratio, float levelDb) noexcept
{
    float reduction = curveFor (ratio).gainReductionDb (levelDb);

    if (ratio == FetRatio::allButtons && levelDb > thresholdDb)
        reduction -= allButtonsExtraDb * std::min (1.0f, (levelDb - thresholdDb) / 4.0f);

    return reduction;
}

void CharacterCompressor::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = std::max (1, maxBlockSize);
    dry.assign (static_cast<std::size_t> (blockSize * maxChannels), 0.0f);

    input.prepare (sampleRate, blockSize);
    output.prepare (sampleRate, blockSize);
    ballistics.prepare (sampleRate);
    mix.reset (sampleRate, mixSmoothingSeconds);

    const auto highPass = BiquadDesign::highPass (sampleRate, sidechainHighPassHz, BiquadDesign::butterworthQ12);
    for (auto& filter : sidechainFilter)
        filter.setCoefficients (highPass);

    setSettings (current);
    reset();
}

void CharacterCompressor::setSettings (const CharacterSettings& settings) noexcept
{
    current = settings;
    input.setGainDb (settings.inputDb);
    output.setGainDb (settings.outputDb);
    mix.setTarget (std::clamp (settings.mix, 0.0f, 1.0f));

    const double scale = settings.ratio == FetRatio::allButtons ? allButtonsTimeScale : 1.0;
    ballistics.setTimes (settings.attackMs * 0.001 * scale, settings.releaseMs * 0.001 * scale);
}

void CharacterCompressor::reset() noexcept
{
    for (auto& filter : sidechainFilter)
        filter.reset();
    ballistics.reset();
    gainReduction.store (0.0f, std::memory_order_relaxed);
}

void CharacterCompressor::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    numChannels = std::clamp (numChannels, 0, maxChannels);

    for (int offset = 0; offset < numSamples; offset += blockSize)
        processChunk (channels, numChannels, offset, std::min (blockSize, numSamples - offset));
}

void CharacterCompressor::processChunk (float* const* channels, int numChannels, int offset, int numSamples) noexcept
{
    std::array<float*, maxChannels> chunk {};
    for (int c = 0; c < numChannels; ++c)
        chunk[static_cast<std::size_t> (c)] = channels[c] + offset;

    // Keep the module input for the MIX control (only needed when some dry signal is blended in).
    const bool blend = mix.isSmoothing() || mix.getTarget() < 1.0f;
    if (blend)
        for (int c = 0; c < numChannels; ++c)
            std::copy_n (chunk[static_cast<std::size_t> (c)], numSamples,
                         dry.data() + static_cast<std::size_t> (c * blockSize));

    input.process (chunk.data(), numChannels, numSamples);

    float largestReduction = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        // Peak of the loudest channel, as seen by the (optionally high-passed) sidechain.
        float peak = 0.0f;
        for (int c = 0; c < numChannels; ++c)
        {
            const auto ch = static_cast<std::size_t> (c);
            const float x = chunk[ch][i];
            const float sc = current.sidechainHighPass ? sidechainFilter[ch].processSample (x) : x;
            peak = std::max (peak, std::abs (sc));
        }

        const float levelDb = 20.0f * std::log10 (peak + silenceLevel);
        const float reductionDb = ballistics.process (staticGainReductionDb (current.ratio, levelDb));
        largestReduction = std::min (largestReduction, reductionDb);

        const float gain = std::exp (reductionDb * ln10Over20);
        for (int c = 0; c < numChannels; ++c)
            chunk[static_cast<std::size_t> (c)][i] *= gain;
    }

    output.process (chunk.data(), numChannels, numSamples);

    if (blend)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float wet = mix.getNext();
            for (int c = 0; c < numChannels; ++c)
            {
                float& sample = chunk[static_cast<std::size_t> (c)][i];
                const float drySample = dry[static_cast<std::size_t> (c * blockSize + i)];
                sample = drySample + wet * (sample - drySample);
            }
        }
    }

    gainReduction.store (largestReduction, std::memory_order_relaxed);
}

} // namespace lionvoice::dsp
