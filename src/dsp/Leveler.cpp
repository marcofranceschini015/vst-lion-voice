// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "Leveler.h"
#include "BiquadDesign.h"

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{
namespace
{
constexpr double sidechainHighPassHz = 80.0;
constexpr double emphasisHz = 3000.0;
constexpr double emphasisDb = 2.0;
constexpr float silencePower = 1.0e-12f; // -120 dB, keeps log10 finite
constexpr float ln10Over20 = 0.11512925f;
} // namespace

Leveler::Leveler() noexcept
{
    threshold.setCurrentAndTarget (thresholdForPeakReduction (LevelerSettings {}.peakReduction));
}

float Leveler::thresholdForPeakReduction (float peakReduction) noexcept
{
    return 10.0f - 0.5f * std::clamp (peakReduction, 0.0f, 100.0f);
}

GainComputer Leveler::curveFor (bool limit, float thresholdDb) noexcept
{
    return limit ? GainComputer { thresholdDb, 10.0f, 6.0f } : GainComputer { thresholdDb, 3.0f, 14.0f };
}

void Leveler::prepare (double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    const auto highPass = BiquadDesign::highPass1stOrder (sampleRate, sidechainHighPassHz);
    const auto emphasis = BiquadDesign::highShelf (sampleRate, emphasisHz, BiquadDesign::butterworthQ12, emphasisDb);
    for (std::size_t ch = 0; ch < maxChannels; ++ch)
    {
        sidechainHighPass[ch].setCoefficients (highPass);
        sidechainEmphasis[ch].setCoefficients (emphasis);
    }

    detectorCoefficient = Ballistics::coefficient (detectorSeconds, sampleRate);
    threshold.reset (sampleRate, thresholdSmoothingSeconds);
    cell.prepare (sampleRate);
    makeup.prepare (sampleRate, maxBlockSize);

    reset();
}

void Leveler::setSettings (const LevelerSettings& settings) noexcept
{
    threshold.setTarget (thresholdForPeakReduction (settings.peakReduction));
    limitMode = settings.limit;
    makeup.setGainDb (settings.gainDb);
}

void Leveler::reset() noexcept
{
    for (std::size_t ch = 0; ch < maxChannels; ++ch)
    {
        sidechainHighPass[ch].reset();
        sidechainEmphasis[ch].reset();
    }

    detectorPower = 0.0f;
    cell.reset();
    gainReduction.store (0.0f, std::memory_order_relaxed);
}

void Leveler::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    numChannels = std::clamp (numChannels, 0, maxChannels);
    float largestReduction = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Sidechain: power of the loudest channel (stereo link).
        float power = 0.0f;
        for (int c = 0; c < numChannels; ++c)
        {
            const auto ch = static_cast<std::size_t> (c);
            const float sc = sidechainEmphasis[ch].processSample (sidechainHighPass[ch].processSample (channels[c][i]));
            power = std::max (power, sc * sc);
        }

        // Averaged over ~10 ms, like the light of the electro-luminescent panel.
        detectorPower = detectorCoefficient * detectorPower + (1.0f - detectorCoefficient) * power;
        const float levelDb = 10.0f * std::log10 (detectorPower + silencePower);

        const float targetDb = curveFor (limitMode, threshold.getNext()).gainReductionDb (levelDb);
        const float reductionDb = cell.process (targetDb);
        largestReduction = std::min (largestReduction, reductionDb);

        const float gain = std::exp (reductionDb * ln10Over20);
        for (int c = 0; c < numChannels; ++c)
            channels[c][i] *= gain;
    }

    makeup.process (channels, numChannels, numSamples);
    gainReduction.store (largestReduction, std::memory_order_relaxed);
}

} // namespace lionvoice::dsp
