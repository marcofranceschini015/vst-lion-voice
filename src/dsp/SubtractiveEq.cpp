// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "SubtractiveEq.h"
#include "BiquadDesign.h"

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{
namespace
{

constexpr std::size_t numBands = SubtractiveEqSettings::numBands;

float toLog2Frequency (float frequency) noexcept
{
    return std::log2 (std::max (frequency, 1.0f));
}

float secondOrderQFor (HpfSlope slope) noexcept
{
    return static_cast<float> (slope == HpfSlope::db18 ? BiquadDesign::butterworthQ18 : BiquadDesign::butterworthQ12);
}

float firstOrderMixFor (HpfSlope slope) noexcept
{
    return slope == HpfSlope::db18 ? 1.0f : 0.0f;
}

} // namespace

SubtractiveEq::SubtractiveEq() noexcept
{
    setSettings (SubtractiveEqSettings {});
    prepare (sampleRate);
}

void SubtractiveEq::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;

    auto resetSmoother = [this] (LinearSmoother& s) { s.reset (sampleRate, smoothingSeconds); };

    resetSmoother (hpfLog2Frequency);
    resetSmoother (hpfSecondOrderQ);
    resetSmoother (firstOrderMix);

    for (std::size_t b = 0; b < numBands; ++b)
    {
        resetSmoother (bandLog2Frequency[b]);
        resetSmoother (bandCutDb[b]);
        resetSmoother (bandQ[b]);
    }

    reset();
}

void SubtractiveEq::setSettings (const SubtractiveEqSettings& settings) noexcept
{
    hpfLog2Frequency.setTarget (toLog2Frequency (settings.hpfFrequency));
    hpfSecondOrderQ.setTarget (secondOrderQFor (settings.hpfSlope));
    firstOrderMix.setTarget (firstOrderMixFor (settings.hpfSlope));

    for (std::size_t b = 0; b < numBands; ++b)
    {
        bandLog2Frequency[b].setTarget (toLog2Frequency (settings.bands[b].frequency));
        bandCutDb[b].setTarget (settings.bands[b].cutDb);
        bandQ[b].setTarget (settings.bands[b].q);
    }
}

void SubtractiveEq::reset() noexcept
{
    advanceSmoothers (1 << 30); // jump every ramp to its end
    updateCoefficients();

    for (auto& channel : filters)
    {
        channel.hpfSecondOrder.reset();
        channel.hpfFirstOrder.reset();
        for (auto& band : channel.bands)
            band.reset();
    }
}

void SubtractiveEq::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    numChannels = std::min (numChannels, maxChannels);

    for (int offset = 0; offset < numSamples; offset += coefficientUpdateInterval)
    {
        const int length = std::min (coefficientUpdateInterval, numSamples - offset);

        // The first-order mix is interpolated per sample across the sub-block (start -> end);
        // everything else changes once per sub-block through the coefficients.
        const float mixStart = firstOrderMix.getCurrent();

        if (isSmoothing())
        {
            advanceSmoothers (length);
            updateCoefficients();
        }

        processSubBlock (channels, numChannels, offset, length, mixStart, firstOrderMix.getCurrent());
    }
}

bool SubtractiveEq::isSmoothing() const noexcept
{
    if (hpfLog2Frequency.isSmoothing() || hpfSecondOrderQ.isSmoothing() || firstOrderMix.isSmoothing())
        return true;

    for (std::size_t b = 0; b < numBands; ++b)
        if (bandLog2Frequency[b].isSmoothing() || bandCutDb[b].isSmoothing() || bandQ[b].isSmoothing())
            return true;

    return false;
}

void SubtractiveEq::advanceSmoothers (int numSamples) noexcept
{
    hpfLog2Frequency.skip (numSamples);
    hpfSecondOrderQ.skip (numSamples);
    firstOrderMix.skip (numSamples);

    for (std::size_t b = 0; b < numBands; ++b)
    {
        bandLog2Frequency[b].skip (numSamples);
        bandCutDb[b].skip (numSamples);
        bandQ[b].skip (numSamples);
    }
}

void SubtractiveEq::updateCoefficients() noexcept
{
    const double hpfFrequency = std::exp2 (static_cast<double> (hpfLog2Frequency.getCurrent()));
    const auto secondOrder = BiquadDesign::highPass (sampleRate, hpfFrequency, hpfSecondOrderQ.getCurrent());
    const auto firstOrder = BiquadDesign::highPass1stOrder (sampleRate, hpfFrequency);

    std::array<BiquadCoefficients, numBands> bandCoefficients;
    for (std::size_t b = 0; b < numBands; ++b)
        bandCoefficients[b] = BiquadDesign::peaking (sampleRate,
                                                     std::exp2 (static_cast<double> (bandLog2Frequency[b].getCurrent())),
                                                     bandQ[b].getCurrent(), bandCutDb[b].getCurrent());

    for (auto& channel : filters)
    {
        channel.hpfSecondOrder.setCoefficients (secondOrder);
        channel.hpfFirstOrder.setCoefficients (firstOrder);
        for (std::size_t b = 0; b < numBands; ++b)
            channel.bands[b].setCoefficients (bandCoefficients[b]);
    }
}

void SubtractiveEq::processSubBlock (float* const* channels, int numChannels, int offset, int numSamples,
                                     float mixStart, float mixEnd) noexcept
{
    const float mixStep = (mixEnd - mixStart) / static_cast<float> (numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = channels[ch] + offset;
        auto& f = filters[static_cast<std::size_t> (ch)];

        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = i + 1 == numSamples ? mixEnd : mixStart + mixStep * static_cast<float> (i + 1);

            float x = f.hpfSecondOrder.processSample (data[i]);
            const float firstOrder = f.hpfFirstOrder.processSample (x);
            x += mix * (firstOrder - x);

            for (auto& band : f.bands)
                x = band.processSample (x);

            data[i] = x;
        }
    }
}

double SubtractiveEq::magnitudeDb (const SubtractiveEqSettings& settings, double sampleRate,
                                   double frequency) noexcept
{
    const double hpf = std::max (static_cast<double> (settings.hpfFrequency), 1.0);

    double total = BiquadDesign::magnitudeDb (
        BiquadDesign::highPass (sampleRate, hpf, secondOrderQFor (settings.hpfSlope)), sampleRate, frequency);

    if (settings.hpfSlope == HpfSlope::db18)
        total += BiquadDesign::magnitudeDb (BiquadDesign::highPass1stOrder (sampleRate, hpf), sampleRate, frequency);

    for (const auto& band : settings.bands)
        total += BiquadDesign::magnitudeDb (
            BiquadDesign::peaking (sampleRate, std::max (static_cast<double> (band.frequency), 1.0), band.q, band.cutDb),
            sampleRate, frequency);

    return total;
}

} // namespace lionvoice::dsp
