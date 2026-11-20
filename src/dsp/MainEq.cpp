// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "MainEq.h"
#include "BiquadDesign.h"

#include <algorithm>
#include <cmath>

namespace lionvoice::dsp
{
namespace
{
constexpr double attenQ = 0.8;

float toLog2 (float frequency) noexcept
{
    return std::log2 (std::max (frequency, 1.0f));
}

float fromLog2 (float value) noexcept
{
    return std::exp2 (value);
}
} // namespace

MainEq::MainEq() noexcept
{
    setSettings (MainEqSettings {});
    prepare (sampleRate);
}

std::array<BiquadCoefficients, MainEq::numFilters> MainEq::design (double fs, const MainEqSettings& s) noexcept
{
    std::array<BiquadCoefficients, numFilters> c;
    c[bodyBoost] = BiquadDesign::lowShelf (fs, s.bodyFrequency, BiquadDesign::butterworthQ12, s.bodyBoostDb);
    c[bodyAtten] = BiquadDesign::peaking (fs, s.bodyFrequency * attenFrequencyRatio, attenQ, -s.bodyAttenDb);
    c[presence] = BiquadDesign::peaking (fs, s.presenceFrequency, s.presenceQ, s.presenceBoostDb);
    c[air] = BiquadDesign::highShelf1stOrderMatched (fs, s.airFrequency, s.airBoostDb);
    return c;
}

double MainEq::magnitudeDb (const MainEqSettings& settings, double fs, double frequency) noexcept
{
    double total = 0.0;
    for (const auto& c : design (fs, settings))
        total += BiquadDesign::magnitudeDb (c, fs, frequency);
    return total;
}

void MainEq::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;

    for (auto* s : { &bodyLog2Frequency, &bodyBoostDb, &bodyAttenDb, &presenceLog2Frequency, &presenceQ, &presenceBoostDb,
                     &airLog2Frequency, &airBoostDb })
        s->reset (sampleRate, smoothingSeconds);

    reset();
}

void MainEq::setSettings (const MainEqSettings& s) noexcept
{
    bodyLog2Frequency.setTarget (toLog2 (s.bodyFrequency));
    bodyBoostDb.setTarget (s.bodyBoostDb);
    bodyAttenDb.setTarget (s.bodyAttenDb);
    presenceLog2Frequency.setTarget (toLog2 (s.presenceFrequency));
    presenceQ.setTarget (s.presenceQ);
    presenceBoostDb.setTarget (s.presenceBoostDb);
    airLog2Frequency.setTarget (toLog2 (s.airFrequency));
    airBoostDb.setTarget (s.airBoostDb);
}

void MainEq::reset() noexcept
{
    advanceSmoothers (1 << 30); // jump every ramp to its end
    updateCoefficients();

    for (auto& channel : filters)
        for (auto& filter : channel)
            filter.reset();
}

void MainEq::process (float* const* channels, int numChannels, int numSamples) noexcept
{
    numChannels = std::min (numChannels, maxChannels);

    for (int offset = 0; offset < numSamples; offset += coefficientUpdateInterval)
    {
        const int length = std::min (coefficientUpdateInterval, numSamples - offset);

        if (isSmoothing())
        {
            advanceSmoothers (length);
            updateCoefficients();
        }

        for (int c = 0; c < numChannels; ++c)
        {
            auto& chain = filters[static_cast<std::size_t> (c)];
            float* data = channels[c] + offset;

            for (int i = 0; i < length; ++i)
            {
                float x = data[i];
                for (auto& filter : chain)
                    x = filter.processSample (x);
                data[i] = x;
            }
        }
    }
}

bool MainEq::isSmoothing() const noexcept
{
    for (const auto* s : { &bodyLog2Frequency, &bodyBoostDb, &bodyAttenDb, &presenceLog2Frequency, &presenceQ,
                           &presenceBoostDb, &airLog2Frequency, &airBoostDb })
        if (s->isSmoothing())
            return true;
    return false;
}

void MainEq::advanceSmoothers (int numSamples) noexcept
{
    for (auto* s : { &bodyLog2Frequency, &bodyBoostDb, &bodyAttenDb, &presenceLog2Frequency, &presenceQ, &presenceBoostDb,
                     &airLog2Frequency, &airBoostDb })
        s->skip (numSamples);
}

MainEqSettings MainEq::currentValues() const noexcept
{
    MainEqSettings s;
    s.bodyFrequency = fromLog2 (bodyLog2Frequency.getCurrent());
    s.bodyBoostDb = bodyBoostDb.getCurrent();
    s.bodyAttenDb = bodyAttenDb.getCurrent();
    s.presenceFrequency = fromLog2 (presenceLog2Frequency.getCurrent());
    s.presenceQ = presenceQ.getCurrent();
    s.presenceBoostDb = presenceBoostDb.getCurrent();
    s.airFrequency = fromLog2 (airLog2Frequency.getCurrent());
    s.airBoostDb = airBoostDb.getCurrent();
    return s;
}

void MainEq::updateCoefficients() noexcept
{
    const auto coefficients = design (sampleRate, currentValues());

    for (auto& channel : filters)
        for (std::size_t f = 0; f < channel.size(); ++f)
            channel[f].setCoefficients (coefficients[f]);
}

} // namespace lionvoice::dsp
