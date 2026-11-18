// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/SubtractiveEq.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{

/** Settings that leave the audible range untouched: HPF at 20 Hz, every band at 0 dB. */
SubtractiveEqSettings neutralSettings()
{
    SubtractiveEqSettings s;
    s.hpfFrequency = 20.0f;
    return s;
}

void processInBlocks (SubtractiveEq& eq, std::vector<float>& signal, int blockSize)
{
    for (std::size_t start = 0; start < signal.size(); start += static_cast<std::size_t> (blockSize))
    {
        const auto n = std::min (static_cast<std::size_t> (blockSize), signal.size() - start);
        float* channels[] = { signal.data() + start };
        eq.process (channels, 1, static_cast<int> (n));
    }
}

/** Runs a sine through a freshly prepared EQ and returns the steady-state gain in dB. */
double measuredGainDb (const SubtractiveEqSettings& settings, double fs, double frequency)
{
    SubtractiveEq eq;
    eq.setSettings (settings);
    eq.prepare (fs);

    auto signal = lionvoice::test::makeSine (fs, frequency, 1.0);
    const auto inputRms = lionvoice::test::rms (signal, signal.size() / 2, signal.size());
    processInBlocks (eq, signal, 512);
    const auto outputRms = lionvoice::test::rms (signal, signal.size() / 2, signal.size());

    return 20.0 * std::log10 (outputRms / inputRms);
}

/** Largest jump between two consecutive samples. */
float maxStep (const std::vector<float>& signal, std::size_t from = 1)
{
    float result = 0.0f;
    for (std::size_t i = std::max<std::size_t> (from, 1); i < signal.size(); ++i)
        result = std::max (result, std::abs (signal[i] - signal[i - 1]));
    return result;
}

} // namespace

TEST_CASE ("Each band cuts by its amount at its centre frequency", "[subeq]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const auto band = static_cast<std::size_t> (GENERATE (0, 1, 2, 3));

    auto settings = neutralSettings();
    settings.bands[band].cutDb = -8.0f;
    const double centre = settings.bands[band].frequency;

    CHECK_THAT (measuredGainDb (settings, fs, centre), WithinAbs (-8.0, 0.2));
}

TEST_CASE ("Neutral settings are flat above 100 Hz", "[subeq]")
{
    const double frequency = GENERATE (150.0, 440.0, 1000.0, 3000.0, 8000.0);

    CHECK_THAT (measuredGainDb (neutralSettings(), 48000.0, frequency), WithinAbs (0.0, 0.1));
}

TEST_CASE ("High-pass slopes", "[subeq][hpf]")
{
    auto settings = neutralSettings();
    settings.hpfFrequency = 100.0f;

    SECTION ("12 dB/oct")
    {
        settings.hpfSlope = HpfSlope::db12;
        CHECK_THAT (measuredGainDb (settings, 48000.0, 100.0), WithinAbs (-3.0, 0.2));
        CHECK_THAT (measuredGainDb (settings, 48000.0, 50.0), WithinAbs (-12.3, 0.5));
    }

    SECTION ("18 dB/oct")
    {
        settings.hpfSlope = HpfSlope::db18;
        CHECK_THAT (measuredGainDb (settings, 48000.0, 100.0), WithinAbs (-3.0, 0.2));
        CHECK_THAT (measuredGainDb (settings, 48000.0, 50.0), WithinAbs (-18.1, 0.5));
    }
}

TEST_CASE ("magnitudeDb() matches the processed audio", "[subeq]")
{
    SubtractiveEqSettings settings;
    settings.hpfFrequency = 110.0f;
    settings.hpfSlope = HpfSlope::db18;
    settings.bands[0] = { 280.0f, -6.0f, 2.0f };
    settings.bands[1] = { 450.0f, -3.0f, 0.8f };
    settings.bands[2] = { 950.0f, -10.0f, 4.0f };
    settings.bands[3] = { 2500.0f, -4.5f, 1.4f };

    const double frequency = GENERATE (60.0, 110.0, 280.0, 450.0, 700.0, 950.0, 2500.0, 6000.0);

    CHECK_THAT (measuredGainDb (settings, 48000.0, frequency),
                WithinAbs (SubtractiveEq::magnitudeDb (settings, 48000.0, frequency), 0.1));
}

TEST_CASE ("Results don't depend on the host block size", "[subeq]")
{
    SubtractiveEqSettings settings;
    settings.hpfFrequency = 120.0f;
    settings.bands[0].cutDb = -9.0f;
    settings.bands[3].cutDb = -4.0f;

    auto render = [&] (int blockSize)
    {
        SubtractiveEq eq;
        eq.setSettings (settings);
        eq.prepare (48000.0);
        auto signal = lionvoice::test::makeSine (48000.0, 260.0, 0.2);
        processInBlocks (eq, signal, blockSize);
        return signal;
    };

    const auto reference = render (2048);
    CHECK (render (1) == reference);
    CHECK (render (7) == reference);
    CHECK (render (33) == reference);
}

TEST_CASE ("Sudden cut changes are smoothed (no click)", "[subeq][smoothing]")
{
    const double fs = 48000.0;
    auto settings = neutralSettings();

    SubtractiveEq eq;
    eq.setSettings (settings);
    eq.prepare (fs);

    // 250 Hz sine: its own largest sample-to-sample step is about 0.016.
    auto signal = lionvoice::test::makeSine (fs, 250.0, 0.5);

    std::vector<float> first (signal.begin(), signal.begin() + 12000);
    processInBlocks (eq, first, 512);

    settings.bands[0].cutDb = -15.0f; // full cut in one jump
    eq.setSettings (settings);

    std::vector<float> second (signal.begin() + 12000, signal.end());
    processInBlocks (eq, second, 512);

    first.insert (first.end(), second.begin(), second.end());
    for (const auto sample : first)
        REQUIRE (std::isfinite (sample));

    CHECK (maxStep (first, 2000) < 0.02f);
}

TEST_CASE ("Switching 12 <-> 18 dB/oct is smoothed (no click)", "[subeq][smoothing]")
{
    const double fs = 48000.0;
    auto settings = neutralSettings();
    settings.hpfFrequency = 100.0f;

    SubtractiveEq eq;
    eq.setSettings (settings);
    eq.prepare (fs);

    // 60 Hz sine: own largest step is about 0.004, while an instant slope switch would jump ~0.05.
    auto signal = lionvoice::test::makeSine (fs, 60.0, 1.5);
    const std::size_t third = signal.size() / 3;

    std::vector<float> out;
    for (int part = 0; part < 3; ++part)
    {
        if (part == 1)
            settings.hpfSlope = HpfSlope::db18;
        if (part == 2)
            settings.hpfSlope = HpfSlope::db12;
        eq.setSettings (settings);

        std::vector<float> chunk (signal.begin() + static_cast<std::ptrdiff_t> (third * static_cast<std::size_t> (part)),
                                  signal.begin() + static_cast<std::ptrdiff_t> (third * static_cast<std::size_t> (part + 1)));
        processInBlocks (eq, chunk, 256);
        out.insert (out.end(), chunk.begin(), chunk.end());
    }

    CHECK (maxStep (out, 4800) < 0.006f);
}

namespace
{

/**
    Feeds a sine through the EQ, applies `change` at changeSeconds, and returns the gain in dB measured
    over [windowStart, windowEnd) seconds after the change.
*/
template <typename ChangeFn>
double gainAfterChangeDb (SubtractiveEqSettings settings, double sineHz, double windowStart, double windowEnd,
                          ChangeFn&& change)
{
    constexpr double fs = 48000.0;
    constexpr std::size_t changeAt = 24000; // 0.5 s: the filters have settled

    SubtractiveEq eq;
    eq.setSettings (settings);
    eq.prepare (fs);

    const auto input = lionvoice::test::makeSine (fs, sineHz, 1.0);
    auto output = input;

    std::vector<float> first (output.begin(), output.begin() + changeAt);
    processInBlocks (eq, first, 32);

    change (settings);
    eq.setSettings (settings);

    std::vector<float> second (output.begin() + changeAt, output.end());
    processInBlocks (eq, second, 32);

    const auto from = static_cast<std::size_t> (windowStart * fs);
    const auto to = static_cast<std::size_t> (windowEnd * fs);
    std::vector<float> inputAfter (input.begin() + changeAt, input.end());

    return 20.0 * std::log10 (lionvoice::test::rms (second, from, to) / lionvoice::test::rms (inputAfter, from, to));
}

} // namespace

TEST_CASE ("Cut changes ramp over about 20 ms", "[subeq][smoothing]")
{
    // 0 -> -15 dB at 250 Hz. Measured over one period (4 ms) between 6 and 10 ms after the change,
    // the ramp is about 30-50 % done: clearly between the old (0 dB) and the new (-15 dB) level.
    // Without smoothing the filter would already sit near -15 dB.
    const double midRamp = gainAfterChangeDb (neutralSettings(), 250.0, 0.006, 0.010,
                                              [] (SubtractiveEqSettings& s) { s.bands[0].cutDb = -15.0f; });
    CHECK (midRamp < -2.0);
    CHECK (midRamp > -10.0);

    const double settled = gainAfterChangeDb (neutralSettings(), 250.0, 0.040, 0.048,
                                              [] (SubtractiveEqSettings& s) { s.bands[0].cutDb = -15.0f; });
    CHECK_THAT (settled, WithinAbs (-15.0, 0.3));
}

TEST_CASE ("Slope changes ramp over about 20 ms", "[subeq][smoothing]")
{
    // HPF 300 Hz, sine at 150 Hz: about -12.3 dB at 12 dB/oct and -18.1 dB at 18 dB/oct.
    auto settings = neutralSettings();
    settings.hpfFrequency = 300.0f;
    auto toSteep = [] (SubtractiveEqSettings& s) { s.hpfSlope = HpfSlope::db18; };

    const double midRamp = gainAfterChangeDb (settings, 150.0, 0.0033, 0.0100, toSteep); // one 6.7 ms period
    const double settled = gainAfterChangeDb (settings, 150.0, 0.0400, 0.0467, toSteep);

    CHECK_THAT (settled, WithinAbs (-18.1, 0.3));
    CHECK (midRamp > settled + 1.5);
    CHECK (midRamp < -12.3);
}

TEST_CASE ("Wild parameter sweeps stay finite", "[subeq][robustness]")
{
    SubtractiveEq eq;
    eq.prepare (44100.0);

    auto signal = lionvoice::test::makeSine (44100.0, 500.0, 1.0, 0.9f);
    SubtractiveEqSettings settings;

    for (std::size_t start = 0; start < signal.size(); start += 64)
    {
        const bool flip = (start / 64) % 2 == 0;
        settings.hpfFrequency = flip ? 20.0f : 300.0f;
        settings.hpfSlope = flip ? HpfSlope::db12 : HpfSlope::db18;
        for (auto& band : settings.bands)
            band = { flip ? band.frequency * 0.5f + 100.0f : band.frequency * 1.5f, flip ? 0.0f : -15.0f,
                     flip ? 0.5f : 6.0f };
        eq.setSettings (settings);

        const auto n = std::min<std::size_t> (64, signal.size() - start);
        float* channels[] = { signal.data() + start };
        eq.process (channels, 1, static_cast<int> (n));
    }

    for (const auto sample : signal)
        REQUIRE (std::isfinite (sample));
}

TEST_CASE ("Channels are processed independently", "[subeq]")
{
    SubtractiveEq eq;
    auto settings = neutralSettings();
    settings.hpfFrequency = 150.0f;
    settings.bands[1].cutDb = -12.0f;
    eq.setSettings (settings);
    eq.prepare (48000.0);

    auto left = lionvoice::test::makeSine (48000.0, 500.0, 0.1);
    std::vector<float> right (left.size(), 0.0f);
    float* channels[] = { left.data(), right.data() };
    eq.process (channels, 2, static_cast<int> (left.size()));

    for (const auto sample : right)
        REQUIRE (sample == 0.0f);
}
