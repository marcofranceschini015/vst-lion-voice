// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/Decibels.h"
#include "dsp/GainStage.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
constexpr double fs = 48000.0;
constexpr int rampSamples = 960; // 20 ms at 48 kHz

/** Processes a mono signal in blocks of blockSize samples. */
void processInBlocks (GainStage& stage, std::vector<float>& signal, int blockSize)
{
    for (std::size_t start = 0; start < signal.size(); start += static_cast<std::size_t> (blockSize))
    {
        const auto n = std::min (static_cast<std::size_t> (blockSize), signal.size() - start);
        float* channels[] = { signal.data() + start };
        stage.process (channels, 1, static_cast<int> (n));
    }
}
} // namespace

TEST_CASE ("Gain stage applies the requested gain", "[gain]")
{
    const float gainDb = GENERATE (-24.0f, -6.0f, 6.0f, 24.0f);

    GainStage stage;
    stage.setGainDb (gainDb);
    stage.prepare (fs, 512); // jumps straight to the target

    std::vector<float> signal (2048, 0.25f);
    processInBlocks (stage, signal, 512);

    for (const auto sample : signal)
        REQUIRE_THAT (sample, WithinRel (0.25f * dbToGain (gainDb), 1.0e-6f));
}

TEST_CASE ("Gain stage at 0 dB is a bit-exact passthrough", "[gain]")
{
    GainStage stage;
    stage.setGainDb (0.0f);
    stage.prepare (fs, 256);

    auto signal = lionvoice::test::makeSine (fs, 440.0, 0.1);
    const auto original = signal;
    processInBlocks (stage, signal, 256);

    REQUIRE (signal == original);
}

TEST_CASE ("Gain changes ramp smoothly over 20 ms", "[gain]")
{
    GainStage stage;
    stage.setGainDb (0.0f);
    stage.prepare (fs, 512);

    stage.setGainDb (12.0f);

    std::vector<float> signal (4096, 1.0f);
    processInBlocks (stage, signal, 512);

    const float target = dbToGain (12.0f);
    const float maxStep = (target - 1.0f) / static_cast<float> (rampSamples);

    // No jump bigger than one ramp step between consecutive samples: no click.
    for (std::size_t i = 1; i < signal.size(); ++i)
        REQUIRE (signal[i] - signal[i - 1] <= maxStep * 1.001f);

    // Still ramping just before 20 ms, at the target from 20 ms on.
    CHECK (signal[rampSamples - 2] < target);
    CHECK (signal[rampSamples - 1] == target);
    CHECK (signal.back() == target);
}

TEST_CASE ("Both channels get exactly the same gain", "[gain]")
{
    GainStage stage;
    stage.prepare (fs, 128);
    stage.setGainDb (-9.0f); // ramp in progress during the whole block

    auto left = lionvoice::test::makeSine (fs, 300.0, 0.01);
    auto right = left;
    float* channels[] = { left.data(), right.data() };
    stage.process (channels, 2, static_cast<int> (left.size()));

    REQUIRE (left == right);
}

TEST_CASE ("Blocks larger than announced are processed correctly", "[gain]")
{
    GainStage small, large;
    for (auto* s : { &small, &large })
        s->setGainDb (0.0f);

    small.prepare (fs, 64);   // host announced 64 samples...
    large.prepare (fs, 4096); // ...reference with plenty of room
    small.setGainDb (-12.0f);
    large.setGainDb (-12.0f);

    std::vector<float> a (2000, 0.5f), b (2000, 0.5f);
    float* chA[] = { a.data() };
    float* chB[] = { b.data() };
    small.process (chA, 1, 2000); // ...but sends 2000 at once
    large.process (chB, 1, 2000);

    for (std::size_t i = 0; i < a.size(); ++i)
        REQUIRE_THAT (a[i], WithinAbs (b[i], 1.0e-6));
}

TEST_CASE ("Unprepared gain stage leaves audio untouched", "[gain]")
{
    GainStage stage;
    stage.setGainDb (12.0f);

    std::vector<float> signal (100, 0.3f);
    const auto original = signal;
    float* channels[] = { signal.data() };
    stage.process (channels, 1, 100);

    REQUIRE (signal == original);
}
