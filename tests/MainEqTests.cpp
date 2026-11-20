// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/BiquadDesign.h"
#include "dsp/MainEq.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{
void processInBlocks (MainEq& eq, std::vector<float>& signal, int blockSize)
{
    for (std::size_t start = 0; start < signal.size(); start += static_cast<std::size_t> (blockSize))
    {
        const auto n = std::min (static_cast<std::size_t> (blockSize), signal.size() - start);
        float* channels[] = { signal.data() + start };
        eq.process (channels, 1, static_cast<int> (n));
    }
}

double measuredGainDb (const MainEqSettings& settings, double fs, double frequency)
{
    MainEq eq;
    eq.setSettings (settings);
    eq.prepare (fs);

    auto signal = lionvoice::test::makeSine (fs, frequency, 1.0);
    const auto inputRms = lionvoice::test::rms (signal, signal.size() / 2, signal.size());
    processInBlocks (eq, signal, 512);
    return 20.0 * std::log10 (lionvoice::test::rms (signal, signal.size() / 2, signal.size()) / inputRms);
}
} // namespace

TEST_CASE ("Main EQ at default settings leaves the signal untouched", "[maineq]")
{
    MainEq eq;
    eq.prepare (48000.0);

    auto signal = lionvoice::test::makeSine (48000.0, 440.0, 0.2);
    const auto original = signal;
    processInBlocks (eq, signal, 256);

    for (std::size_t i = 0; i < signal.size(); ++i)
        REQUIRE_THAT (signal[i], WithinAbs (original[i], 1.0e-5));
}

TEST_CASE ("Presence boosts by its amount at its frequency", "[maineq]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const float frequency = GENERATE (3000.0f, 4500.0f, 6000.0f);

    MainEqSettings s;
    s.presenceFrequency = frequency;
    s.presenceBoostDb = 6.0f;

    CHECK_THAT (measuredGainDb (s, fs, frequency), WithinAbs (6.0, 0.2));
    CHECK_THAT (measuredGainDb (s, fs, 200.0), WithinAbs (0.0, 0.2));
}

TEST_CASE ("Body boost lifts the lows and leaves the mids", "[maineq]")
{
    MainEqSettings s;
    s.bodyFrequency = 80.0f;
    s.bodyBoostDb = 6.0f;

    CHECK_THAT (measuredGainDb (s, 48000.0, 20.0), WithinAbs (6.0, 0.6));
    CHECK_THAT (measuredGainDb (s, 48000.0, 80.0), WithinAbs (3.0, 0.3)); // half the boost at the corner
    CHECK_THAT (measuredGainDb (s, 48000.0, 1000.0), WithinAbs (0.0, 0.1));
}

TEST_CASE ("Pultec trick: boost and atten together give deep lows and a dip above", "[maineq]")
{
    MainEqSettings s;
    s.bodyFrequency = 80.0f;
    s.bodyBoostDb = 6.0f;
    s.bodyAttenDb = 6.0f;

    CHECK (measuredGainDb (s, 48000.0, 30.0) > 3.0);   // lows still clearly boosted
    CHECK (measuredGainDb (s, 48000.0, 200.0) < -3.0); // the boom/mud area above is tightened
    CHECK_THAT (measuredGainDb (s, 48000.0, 2000.0), WithinAbs (0.0, 0.3));
}

TEST_CASE ("Air shelf follows the analog curve up to 20 kHz (no Nyquist cramping)", "[maineq][air]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const double corner = GENERATE (8000.0, 12000.0, 16000.0);
    const double boost = 6.0;

    const auto c = BiquadDesign::highShelf1stOrderMatched (fs, corner, boost);

    for (const double f : { 100.0, 1000.0, 5000.0, 10000.0, 16000.0, 20000.0 })
        CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, f),
                    WithinAbs (BiquadDesign::analogHighShelf1stOrderDb (f, corner, boost), 0.5));

    // The plain bilinear version of the same shelf misses badly at 20 kHz when the sample rate is 44.1 kHz:
    // this is the problem the matched design solves.
    if (fs < 45000.0 && corner >= 12000.0)
    {
        const auto bilinear = BiquadDesign::highShelf (fs, corner, 0.5, boost);
        CHECK (std::abs (BiquadDesign::magnitudeDb (bilinear, fs, 20000.0)
                         - BiquadDesign::analogHighShelf1stOrderDb (20000.0, corner, boost))
               > 0.5);
    }
}

TEST_CASE ("Air shelf at 0 dB is transparent", "[maineq][air]")
{
    const auto c = BiquadDesign::highShelf1stOrderMatched (44100.0, 12000.0, 0.0);
    for (const double f : { 50.0, 1000.0, 10000.0, 20000.0 })
        CHECK_THAT (BiquadDesign::magnitudeDb (c, 44100.0, f), WithinAbs (0.0, 1.0e-6));
}

TEST_CASE ("magnitudeDb() matches the processed audio", "[maineq]")
{
    MainEqSettings s;
    s.bodyFrequency = 70.0f;
    s.bodyBoostDb = 4.0f;
    s.bodyAttenDb = 3.0f;
    s.presenceFrequency = 5000.0f;
    s.presenceQ = 1.8f;
    s.presenceBoostDb = 5.0f;
    s.airFrequency = 10000.0f;
    s.airBoostDb = 4.0f;

    const double frequency = GENERATE (30.0, 70.0, 175.0, 1000.0, 5000.0, 10000.0, 18000.0);
    CHECK_THAT (measuredGainDb (s, 48000.0, frequency), WithinAbs (MainEq::magnitudeDb (s, 48000.0, frequency), 0.1));
}

TEST_CASE ("Main EQ boosts ramp over about 20 ms", "[maineq][smoothing]")
{
    constexpr double fs = 48000.0;
    MainEq eq;
    eq.prepare (fs);

    const auto input = lionvoice::test::makeSine (fs, 4500.0, 0.6);
    auto output = input;

    std::vector<float> first (output.begin(), output.begin() + 24000);
    processInBlocks (eq, first, 32);

    MainEqSettings s;
    s.presenceBoostDb = 10.0f; // 0 -> +10 dB in one jump
    eq.setSettings (s);

    std::vector<float> second (output.begin() + 24000, output.end());
    processInBlocks (eq, second, 32);

    std::vector<float> inputAfter (input.begin() + 24000, input.end());
    auto gainDb = [&] (double from, double to)
    {
        const auto a = static_cast<std::size_t> (from * fs);
        const auto b = static_cast<std::size_t> (to * fs);
        return 20.0 * std::log10 (lionvoice::test::rms (second, a, b) / lionvoice::test::rms (inputAfter, a, b));
    };

    const double midRamp = gainDb (0.006, 0.010);
    CHECK (midRamp > 1.5);
    CHECK (midRamp < 7.0);
    CHECK_THAT (gainDb (0.040, 0.060), WithinAbs (10.0, 0.3));
}

TEST_CASE ("Main EQ results don't depend on the host block size", "[maineq]")
{
    MainEqSettings s;
    s.bodyBoostDb = 5.0f;
    s.presenceBoostDb = 3.0f;
    s.airBoostDb = 2.0f;

    auto render = [&] (int blockSize)
    {
        MainEq eq;
        eq.setSettings (s);
        eq.prepare (48000.0);
        auto signal = lionvoice::test::makeSine (48000.0, 1234.0, 0.2);
        processInBlocks (eq, signal, blockSize);
        return signal;
    };

    const auto reference = render (2048);
    CHECK (render (1) == reference);
    CHECK (render (33) == reference);
}

TEST_CASE ("Main EQ stays finite under wild parameter jumps", "[maineq][robustness]")
{
    MainEq eq;
    eq.prepare (44100.0);
    auto signal = lionvoice::test::makeSine (44100.0, 300.0, 1.0, 0.9f);

    for (std::size_t start = 0; start < signal.size(); start += 64)
    {
        const bool flip = (start / 64) % 2 == 0;
        MainEqSettings s;
        s.bodyFrequency = flip ? 50.0f : 120.0f;
        s.bodyBoostDb = flip ? 10.0f : 0.0f;
        s.bodyAttenDb = flip ? 0.0f : 10.0f;
        s.presenceFrequency = flip ? 3000.0f : 6000.0f;
        s.presenceQ = flip ? 0.5f : 3.0f;
        s.presenceBoostDb = flip ? 10.0f : 0.0f;
        s.airFrequency = flip ? 8000.0f : 16000.0f;
        s.airBoostDb = flip ? 8.0f : 0.0f;
        eq.setSettings (s);

        const auto n = std::min<std::size_t> (64, signal.size() - start);
        float* channels[] = { signal.data() + start };
        eq.process (channels, 1, static_cast<int> (n));
    }

    for (const auto sample : signal)
        REQUIRE (std::isfinite (sample));
}
