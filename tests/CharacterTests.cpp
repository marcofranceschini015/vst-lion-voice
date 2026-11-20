// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/CharacterCompressor.h"
#include "dsp/Decibels.h"

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

std::unique_ptr<CharacterCompressor> makeCompressor (const CharacterSettings& settings)
{
    auto compressor = std::make_unique<CharacterCompressor>(); // not copyable (atomic for the GUI)
    compressor->setSettings (settings);
    compressor->prepare (fs, 512);
    return compressor;
}

void run (CharacterCompressor& compressor, std::vector<float>& left, std::vector<float>& right)
{
    for (std::size_t start = 0; start < left.size(); start += 512)
    {
        const auto n = std::min<std::size_t> (512, left.size() - start);
        float* channels[] = { left.data() + start, right.data() + start };
        compressor.process (channels, 2, static_cast<int> (n));
    }
}

/** Settings that make the timing irrelevant for steady-state measurements. */
CharacterSettings steadySettings (FetRatio ratio)
{
    CharacterSettings s;
    s.ratio = ratio;
    s.attackMs = 0.02f;
    s.releaseMs = 1100.0f; // slow release: almost no ripple on a steady tone
    return s;
}

/** Output RMS (dB) of a steady 1 kHz sine at the given peak level, measured after it has settled. */
double steadyOutputDb (const CharacterSettings& settings, float peakDbfs)
{
    auto compressor = makeCompressor (settings);
    auto left = lionvoice::test::makeSine (fs, 1000.0, 2.0, dbToGain (peakDbfs));
    auto right = left;
    run (*compressor, left, right);
    return 20.0 * std::log10 (lionvoice::test::rms (left, left.size() / 2, left.size()));
}

float steadyReductionDb (const CharacterSettings& settings, float peakDbfs, double frequency = 1000.0)
{
    auto compressor = makeCompressor (settings);
    auto left = lionvoice::test::makeSine (fs, frequency, 2.0, dbToGain (peakDbfs));
    auto right = left;
    run (*compressor, left, right);
    return compressor->getGainReductionDb();
}
} // namespace

TEST_CASE ("Character: steady reduction follows the static curve", "[character]")
{
    const auto ratio = static_cast<FetRatio> (GENERATE (0, 1, 2, 3, 4));
    const float peakDbfs = -4.0f; // 12 dB over the threshold

    CHECK_THAT (steadyReductionDb (steadySettings (ratio), peakDbfs),
                WithinAbs (CharacterCompressor::staticGainReductionDb (ratio, peakDbfs), 0.5));
}

TEST_CASE ("Character: above the threshold, 6 dB more in gives about 6/R dB more out", "[character]")
{
    struct Case
    {
        FetRatio ratio;
        float r;
    };
    const auto c = GENERATE (Case { FetRatio::r4, 4.0f }, Case { FetRatio::r8, 8.0f }, Case { FetRatio::r20, 20.0f });

    const double quiet = steadyOutputDb (steadySettings (c.ratio), -6.0f);
    const double loud = steadyOutputDb (steadySettings (c.ratio), 0.0f);
    CHECK_THAT (loud - quiet, WithinAbs (6.0 / c.r, 0.3));
}

TEST_CASE ("Character: ALL compresses harder than 20:1", "[character]")
{
    CHECK (steadyReductionDb (steadySettings (FetRatio::allButtons), -8.0f)
           < steadyReductionDb (steadySettings (FetRatio::r20), -8.0f) - 1.0f);
}

TEST_CASE ("Character: INPUT drives the compression, OUTPUT sets the level", "[character]")
{
    auto settings = steadySettings (FetRatio::r4);
    const float base = steadyReductionDb (settings, -14.0f);

    settings.inputDb = 8.0f;
    const float driven = steadyReductionDb (settings, -14.0f);
    CHECK (driven < base - 4.0f);

    settings = steadySettings (FetRatio::r4);
    const double before = steadyOutputDb (settings, -20.0f); // below threshold: no compression
    settings.outputDb = -6.0f;
    CHECK_THAT (steadyOutputDb (settings, -20.0f), WithinAbs (before - 6.0, 0.05));
}

TEST_CASE ("Character: faster attack grabs the transient sooner", "[character]")
{
    auto reductionAfter = [] (float attackMs, double seconds)
    {
        CharacterSettings s;
        s.attackMs = attackMs;
        s.releaseMs = 1100.0f;
        s.sidechainHighPass = false; // the test signal is a DC step, which a high-pass would remove
        auto compressor = makeCompressor (s);

        std::vector<float> left (static_cast<std::size_t> (fs * seconds), 0.8f); // sudden loud DC step
        auto right = left;
        run (*compressor, left, right);
        return compressor->getGainReductionDb();
    };

    // 0.2 ms after the onset: the fastest attack is almost done, the slowest has barely started.
    CHECK (reductionAfter (0.02f, 0.0002) < reductionAfter (0.8f, 0.0002) - 2.0f);
}

TEST_CASE ("Character: release recovers about 63 % in one release time", "[character]")
{
    CharacterSettings s;
    s.attackMs = 0.02f;
    s.releaseMs = 200.0f;
    s.sidechainHighPass = false; // the test signal is a DC step, which a high-pass would remove
    auto compressor = makeCompressor (s);

    std::vector<float> loud (24000, 0.8f), loudR (24000, 0.8f);
    run (*compressor, loud, loudR);
    const float compressed = compressor->getGainReductionDb();

    std::vector<float> silence (static_cast<std::size_t> (fs * 0.2), 0.0f), silenceR (silence.size(), 0.0f);
    float lastReduction = 0.0f;
    for (std::size_t start = 0; start < silence.size(); start += 480) // read the reduction at the very end
    {
        float* channels[] = { silence.data() + start, silenceR.data() + start };
        compressor->process (channels, 2, 480);
        lastReduction = compressor->getGainReductionDb();
    }

    CHECK_THAT (lastReduction / compressed, WithinAbs (0.368, 0.06));
}

TEST_CASE ("Character: MIX blends dry and compressed signals", "[character]")
{
    auto render = [] (float mix)
    {
        auto s = steadySettings (FetRatio::r8);
        s.mix = mix;
        auto compressor = makeCompressor (s);
        auto left = lionvoice::test::makeSine (fs, 700.0, 0.5, 0.9f);
        auto right = left;
        run (*compressor, left, right);
        return left;
    };

    const auto dry = lionvoice::test::makeSine (fs, 700.0, 0.5, 0.9f);
    const auto wet = render (1.0f);
    const auto half = render (0.5f);
    const auto none = render (0.0f);

    REQUIRE (none == dry); // 0 % = untouched, bit for bit
    for (std::size_t i = 0; i < dry.size(); i += 53)
        REQUIRE_THAT (half[i], WithinAbs (0.5f * (dry[i] + wet[i]), 1.0e-5));
}

TEST_CASE ("Character: the sidechain HPF stops low end from triggering compression", "[character]")
{
    auto settings = steadySettings (FetRatio::r8);

    settings.sidechainHighPass = true;
    const float filtered = steadyReductionDb (settings, -6.0f, 50.0);
    settings.sidechainHighPass = false;
    const float unfiltered = steadyReductionDb (settings, -6.0f, 50.0);

    CHECK (filtered > unfiltered + 3.0f);
}

TEST_CASE ("Character: stereo link keeps the image", "[character]")
{
    auto compressor = makeCompressor (steadySettings (FetRatio::r12));
    auto left = lionvoice::test::makeSine (fs, 300.0, 0.5, 0.9f);
    std::vector<float> right (left.size());
    for (std::size_t i = 0; i < left.size(); ++i)
        right[i] = left[i] * 0.5f;

    run (*compressor, left, right);

    for (std::size_t i = 0; i < left.size(); i += 61)
        REQUIRE_THAT (right[i], WithinAbs (left[i] * 0.5f, 1.0e-6));
}

TEST_CASE ("Character: output stays finite for extreme settings", "[character][robustness]")
{
    CharacterSettings s;
    s.inputDb = 24.0f;
    s.outputDb = 12.0f;
    s.ratio = static_cast<FetRatio> (GENERATE (0, 4));
    s.attackMs = GENERATE (0.02f, 0.8f);
    s.releaseMs = 50.0f;
    s.mix = 0.5f;
    auto compressor = makeCompressor (s);

    auto left = lionvoice::test::makeSine (fs, 80.0, 0.5, 1.0f);
    std::vector<float> right (left.size(), 0.0f);
    left[100] = 40.0f;
    run (*compressor, left, right);

    for (const auto sample : left)
        REQUIRE (std::isfinite (sample));
}
