// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/Biquad.h"
#include "dsp/BiquadDesign.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{

/** Combined gain of two filters in series (dB values simply add up). */
double cascadeDb (const BiquadCoefficients& a, const BiquadCoefficients& b, double fs, double f)
{
    return BiquadDesign::magnitudeDb (a, fs, f) + BiquadDesign::magnitudeDb (b, fs, f);
}

/** Runs a sine through the filter and measures the steady-state gain in dB (ignores the start-up transient). */
double measuredGainDb (Biquad& filter, double fs, double f)
{
    auto signal = lionvoice::test::makeSine (fs, f, 1.0);
    const auto inputRms = lionvoice::test::rms (signal, signal.size() / 2, signal.size());

    filter.reset();
    filter.processBlock (signal.data(), static_cast<int> (signal.size()));

    const auto outputRms = lionvoice::test::rms (signal, signal.size() / 2, signal.size());
    return 20.0 * std::log10 (outputRms / inputRms);
}

/** True when both poles are inside the unit circle, i.e. the filter can never blow up. */
bool isStable (const BiquadCoefficients& c)
{
    return std::abs (c.a2) < 1.0 && std::abs (c.a1) < 1.0 + c.a2;
}

bool allFinite (const BiquadCoefficients& c)
{
    return std::isfinite (c.b0) && std::isfinite (c.b1) && std::isfinite (c.b2) && std::isfinite (c.a1)
        && std::isfinite (c.a2);
}

} // namespace

TEST_CASE ("Default biquad passes audio unchanged", "[biquad]")
{
    Biquad filter;
    auto signal = lionvoice::test::makeSine (48000.0, 1000.0, 0.1);
    const auto original = signal;

    filter.processBlock (signal.data(), static_cast<int> (signal.size()));

    REQUIRE (signal == original);
}

TEST_CASE ("Peaking filter reaches its gain at the centre frequency", "[biquad][peaking]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const double f0 = GENERATE (250.0, 500.0, 900.0, 3000.0);
    const double gainDb = GENERATE (-15.0, -6.0, -1.0, 6.0);

    const auto c = BiquadDesign::peaking (fs, f0, 1.4, gainDb);

    CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, f0), WithinAbs (gainDb, 0.1));
    // Far away from the band the filter leaves the signal alone.
    CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, f0 / 16.0), WithinAbs (0.0, 0.3));
}

TEST_CASE ("Peaking filter at 0 dB is transparent", "[biquad][peaking]")
{
    Biquad filter;
    filter.setCoefficients (BiquadDesign::peaking (48000.0, 250.0, 1.4, 0.0));

    auto signal = lionvoice::test::makeSine (48000.0, 250.0, 0.2);
    const auto original = signal;
    filter.processBlock (signal.data(), static_cast<int> (signal.size()));

    for (std::size_t i = 0; i < signal.size(); ++i)
        REQUIRE_THAT (signal[i], WithinAbs (original[i], 1.0e-6));
}

TEST_CASE ("12 dB/oct high-pass: -3 dB at cutoff, about -12 dB one octave below", "[biquad][highpass]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const double fc = GENERATE (80.0, 120.0, 300.0);

    const auto c = BiquadDesign::highPass (fs, fc, BiquadDesign::butterworthQ12);

    CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, fc), WithinAbs (-3.01, 0.05));
    CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, fc / 2.0), WithinAbs (-12.3, 0.5));
    CHECK_THAT (BiquadDesign::magnitudeDb (c, fs, 1000.0), WithinAbs (0.0, 0.1));
}

TEST_CASE ("18 dB/oct high-pass cascade is a third-order Butterworth", "[biquad][highpass]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const double fc = GENERATE (80.0, 120.0, 300.0);

    const auto second = BiquadDesign::highPass (fs, fc, BiquadDesign::butterworthQ18);
    const auto first = BiquadDesign::highPass1stOrder (fs, fc);

    CHECK_THAT (cascadeDb (second, first, fs, fc), WithinAbs (-3.01, 0.05));
    CHECK_THAT (cascadeDb (second, first, fs, fc / 2.0), WithinAbs (-18.1, 0.5));
    CHECK_THAT (cascadeDb (second, first, fs, 1000.0), WithinAbs (0.0, 0.1));
}

TEST_CASE ("First-order high-pass: -3 dB at cutoff", "[biquad][highpass]")
{
    const auto c = BiquadDesign::highPass1stOrder (48000.0, 100.0);

    CHECK_THAT (BiquadDesign::magnitudeDb (c, 48000.0, 100.0), WithinAbs (-3.01, 0.05));
    CHECK_THAT (BiquadDesign::magnitudeDb (c, 48000.0, 50.0), WithinAbs (-6.99, 0.3));
}

TEST_CASE ("Low-pass: -3 dB at cutoff, flat well below", "[biquad][lowpass]")
{
    const auto c = BiquadDesign::lowPass (48000.0, 5000.0, BiquadDesign::butterworthQ12);

    CHECK_THAT (BiquadDesign::magnitudeDb (c, 48000.0, 5000.0), WithinAbs (-3.01, 0.05));
    CHECK_THAT (BiquadDesign::magnitudeDb (c, 48000.0, 200.0), WithinAbs (0.0, 0.1));
}

TEST_CASE ("Shelves reach their gain far from the corner", "[biquad][shelf]")
{
    const double fs = 48000.0;

    const auto low = BiquadDesign::lowShelf (fs, 100.0, BiquadDesign::butterworthQ12, 4.0);
    CHECK_THAT (BiquadDesign::magnitudeDb (low, fs, 10.0), WithinAbs (4.0, 0.1));
    CHECK_THAT (BiquadDesign::magnitudeDb (low, fs, 5000.0), WithinAbs (0.0, 0.1));

    const auto high = BiquadDesign::highShelf (fs, 10000.0, BiquadDesign::butterworthQ12, 3.0);
    CHECK_THAT (BiquadDesign::magnitudeDb (high, fs, 20000.0), WithinAbs (3.0, 0.3));
    CHECK_THAT (BiquadDesign::magnitudeDb (high, fs, 500.0), WithinAbs (0.0, 0.1));
}

TEST_CASE ("Processing real audio matches the computed response", "[biquad][signal]")
{
    const double fs = 48000.0;
    Biquad filter;

    SECTION ("peaking cut at its centre")
    {
        filter.setCoefficients (BiquadDesign::peaking (fs, 250.0, 1.4, -6.0));
        CHECK_THAT (measuredGainDb (filter, fs, 250.0), WithinAbs (-6.0, 0.1));
    }

    SECTION ("high-pass one octave below the cutoff")
    {
        const auto c = BiquadDesign::highPass (fs, 100.0, BiquadDesign::butterworthQ12);
        filter.setCoefficients (c);
        CHECK_THAT (measuredGainDb (filter, fs, 50.0), WithinAbs (BiquadDesign::magnitudeDb (c, fs, 50.0), 0.1));
    }

    SECTION ("first-order high-pass at its cutoff")
    {
        filter.setCoefficients (BiquadDesign::highPass1stOrder (fs, 100.0));
        CHECK_THAT (measuredGainDb (filter, fs, 100.0), WithinAbs (-3.01, 0.1));
    }
}

TEST_CASE ("Extreme settings stay finite and stable", "[biquad][robustness]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const double f = GENERATE (0.0, 5.0, 20.0, 20000.0, 30000.0, 100000.0);
    const double q = GENERATE (0.0, 0.1, 10.0);

    const BiquadCoefficients designs[] = {
        BiquadDesign::peaking (fs, f, q, -24.0),  BiquadDesign::peaking (fs, f, q, 24.0),
        BiquadDesign::lowShelf (fs, f, q, 12.0),  BiquadDesign::highShelf (fs, f, q, 12.0),
        BiquadDesign::highPass (fs, f, q),        BiquadDesign::lowPass (fs, f, q),
        BiquadDesign::highPass1stOrder (fs, f),
    };

    for (const auto& c : designs)
    {
        REQUIRE (allFinite (c));
        REQUIRE (isStable (c));
    }
}

TEST_CASE ("Low-frequency high-pass at 96 kHz settles cleanly", "[biquad][robustness]")
{
    const double fs = 96000.0;
    Biquad filter;
    filter.setCoefficients (BiquadDesign::highPass (fs, 20.0, BiquadDesign::butterworthQ12));

    auto signal = lionvoice::test::makeSine (fs, 1000.0, 2.0);
    filter.processBlock (signal.data(), static_cast<int> (signal.size()));

    for (const auto sample : signal)
        REQUIRE (std::isfinite (sample));

    CHECK_THAT (lionvoice::test::rms (signal, signal.size() / 2, signal.size()), WithinAbs (0.5 / std::sqrt (2.0), 0.001));
}
