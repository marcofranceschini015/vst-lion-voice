// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/BiquadDesign.h"
#include "dsp/Leveler.h"
#include "dsp/dynamics/OptoCell.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <memory>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{
constexpr double fs = 48000.0;

float runCell (OptoCell& cell, double seconds, float target)
{
    float value = cell.getReductionDb();
    const auto n = static_cast<int> (seconds * fs);
    for (int i = 0; i < n; ++i)
        value = cell.process (target);
    return value;
}

/** Runs a stereo signal through the leveler in 512-sample blocks. */
void run (Leveler& leveler, std::vector<float>& left, std::vector<float>& right)
{
    for (std::size_t start = 0; start < left.size(); start += 512)
    {
        const auto n = std::min<std::size_t> (512, left.size() - start);
        float* channels[] = { left.data() + start, right.data() + start };
        leveler.process (channels, 2, static_cast<int> (n));
    }
}

std::unique_ptr<Leveler> makeLeveler (float peakReduction, bool limit = false, float gainDb = 0.0f)
{
    auto leveler = std::make_unique<Leveler>(); // not copyable (holds an atomic for the GUI)
    leveler->setSettings ({ peakReduction, gainDb, limit });
    leveler->prepare (fs, 512);
    return leveler;
}

/** Steady gain reduction for a sine of the given peak level and frequency. */
float steadyReductionDb (float peakReduction, bool limit, float peakDbfs, double frequency)
{
    auto leveler = makeLeveler (peakReduction, limit);
    const auto amplitude = static_cast<float> (std::pow (10.0, peakDbfs / 20.0));
    auto left = lionvoice::test::makeSine (fs, frequency, 3.0, amplitude);
    auto right = left;
    run (*leveler, left, right);
    return leveler->getGainReductionDb();
}
} // namespace

// --- Opto cell ---------------------------------------------------------------------------------

TEST_CASE ("Opto cell attacks in about 10 ms", "[leveler][opto]")
{
    OptoCell cell;
    cell.prepare (fs);
    CHECK_THAT (runCell (cell, 0.010, -10.0f), WithinAbs (-6.32, 0.1));
    CHECK_THAT (runCell (cell, 0.090, -10.0f), WithinAbs (-10.0, 0.05));
}

TEST_CASE ("Opto cell releases in two stages", "[leveler][opto]")
{
    OptoCell cell;
    cell.prepare (fs);
    runCell (cell, 0.5, -8.0f); // short compression: memory almost empty

    const float after150ms = runCell (cell, 0.150, 0.0f);
    const float recovered = 1.0f - after150ms / -8.0f;
    CHECK (recovered > 0.4f); // the fast half is back...
    CHECK (recovered < 0.8f); // ...the slow half is still holding

    const float after1s = runCell (cell, 0.850, 0.0f);
    CHECK (after1s < -8.0f * 0.03f); // still some reduction after a second: the slow tail
}

TEST_CASE ("Opto cell releases slower after long, heavy compression", "[leveler][opto]")
{
    auto remainingAfterRelease = [] (double compressSeconds)
    {
        OptoCell cell;
        cell.prepare (fs);
        runCell (cell, compressSeconds, -8.0f);
        return runCell (cell, 1.0, 0.0f);
    };

    const float shortBurst = remainingAfterRelease (0.5);
    const float longPhrase = remainingAfterRelease (5.0);
    CHECK (longPhrase < shortBurst - 0.2f); // more reduction left = slower release
}

// --- Leveler --------------------------------------------------------------------------------------

TEST_CASE ("Peak reduction maps to the threshold", "[leveler]")
{
    CHECK (Leveler::thresholdForPeakReduction (0.0f) == 10.0f);
    CHECK (Leveler::thresholdForPeakReduction (100.0f) == -40.0f);
    CHECK (Leveler::thresholdForPeakReduction (50.0f) == -15.0f);
}

TEST_CASE ("Steady sine settles to the gain reduction predicted by the curve", "[leveler]")
{
    const bool limit = GENERATE (false, true);
    const float peakReduction = 60.0f;  // threshold -20 dB
    const float peakDbfs = -6.0f;
    const double frequency = 1000.0;

    // Predicted detector level: RMS of the sine (peak - 3.01 dB) plus the sidechain filters' gain at 1 kHz.
    const double sidechainDb =
        BiquadDesign::magnitudeDb (BiquadDesign::highPass1stOrder (fs, 80.0), fs, frequency)
        + BiquadDesign::magnitudeDb (BiquadDesign::highShelf (fs, 3000.0, BiquadDesign::butterworthQ12, 2.0), fs, frequency);
    const auto levelDb = static_cast<float> (peakDbfs - 3.0103 + sidechainDb);
    const float expected =
        Leveler::curveFor (limit, Leveler::thresholdForPeakReduction (peakReduction)).gainReductionDb (levelDb);

    CHECK_THAT (steadyReductionDb (peakReduction, limit, peakDbfs, frequency), WithinAbs (expected, 0.5));
}

TEST_CASE ("LIMIT compresses harder than COMP", "[leveler]")
{
    const float comp = steadyReductionDb (60.0f, false, -6.0f, 1000.0);
    const float limit = steadyReductionDb (60.0f, true, -6.0f, 1000.0);
    CHECK (comp < -1.0f);
    CHECK (limit < comp - 2.0f);
}

TEST_CASE ("Peak reduction 0 never compresses a normal signal", "[leveler]")
{
    CHECK (steadyReductionDb (0.0f, false, -1.0f, 1000.0) == 0.0f);
}

TEST_CASE ("The sidechain high-pass ignores low rumble", "[leveler]")
{
    const float bass = steadyReductionDb (60.0f, false, -6.0f, 40.0);
    const float mid = steadyReductionDb (60.0f, false, -6.0f, 1000.0);
    CHECK (bass > mid + 1.0f); // same level, much less reduction at 40 Hz
}

TEST_CASE ("Stereo link: both channels get the same gain", "[leveler]")
{
    auto leveler = makeLeveler (70.0f);
    auto left = lionvoice::test::makeSine (fs, 500.0, 1.0, 0.8f);
    std::vector<float> right (left.size());
    for (std::size_t i = 0; i < left.size(); ++i)
        right[i] = left[i] * 0.25f; // right channel 12 dB quieter

    run (*leveler, left, right);

    // Same gain on both sides: the right channel stays exactly a quarter of the left.
    for (std::size_t i = 0; i < left.size(); i += 97)
        REQUIRE_THAT (right[i], WithinAbs (left[i] * 0.25f, 1.0e-6));
}

TEST_CASE ("GAIN is applied as make-up after the compression", "[leveler]")
{
    auto leveler = makeLeveler (0.0f, false, 6.0f);
    std::vector<float> left (4800, 0.1f), right (4800, 0.1f);
    run (*leveler, left, right);

    CHECK_THAT (left.back(), WithinAbs (0.1f * dbToGain (6.0f), 1.0e-5));
}

TEST_CASE ("Leveler output stays finite for extreme settings", "[leveler][robustness]")
{
    const float peakReduction = GENERATE (0.0f, 100.0f);
    const bool limit = GENERATE (false, true);
    auto leveler = makeLeveler (peakReduction, limit, 24.0f);

    auto left = lionvoice::test::makeSine (fs, 60.0, 1.0, 1.0f);
    std::vector<float> right (left.size(), 0.0f);
    left[1000] = 50.0f; // absurd spike
    run (*leveler, left, right);

    for (const auto sample : left)
        REQUIRE (std::isfinite (sample));
    for (const auto sample : right)
        REQUIRE (std::isfinite (sample));
}
