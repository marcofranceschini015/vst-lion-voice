// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/ModuleBypass.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{

constexpr double fs = 48000.0;
constexpr int fadeSamples = 480; // 10 ms at 48 kHz

/** A fake module that halves the signal and counts how often it is used. */
struct HalfGainModule
{
    int processCalls = 0;
    int resetCalls = 0;

    void process (float* const* channels, int numChannels, int numSamples)
    {
        ++processCalls;
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < numSamples; ++i)
                channels[ch][i] *= 0.5f;
    }
};

void run (ModuleBypass& bypass, HalfGainModule& module, std::vector<float>& signal, int blockSize)
{
    for (std::size_t start = 0; start < signal.size(); start += static_cast<std::size_t> (blockSize))
    {
        const auto n = std::min (static_cast<std::size_t> (blockSize), signal.size() - start);
        float* channels[] = { signal.data() + start };
        bypass.process (
            channels, 1, static_cast<int> (n), [&] (float* const* c, int nc, int ns) { module.process (c, nc, ns); },
            [&] { ++module.resetCalls; });
    }
}

} // namespace

TEST_CASE ("Bypassed module is not called and audio is bit-exact", "[bypass]")
{
    ModuleBypass bypass;
    HalfGainModule module;
    bypass.setBypassed (true);
    bypass.prepare (fs, 256, 2);

    auto signal = lionvoice::test::makeSine (fs, 440.0, 0.1);
    const auto original = signal;
    run (bypass, module, signal, 256);

    REQUIRE (signal == original);
    CHECK (module.processCalls == 0);
    CHECK (module.resetCalls == 1); // reset once, not on every block
}

TEST_CASE ("Active module processes audio directly", "[bypass]")
{
    ModuleBypass bypass;
    HalfGainModule module;
    bypass.prepare (fs, 256, 2);

    std::vector<float> signal (1024, 0.8f);
    run (bypass, module, signal, 256);

    for (const auto sample : signal)
        REQUIRE (sample == 0.4f);
}

TEST_CASE ("Toggling bypass crossfades over 10 ms without clicks", "[bypass]")
{
    ModuleBypass bypass;
    HalfGainModule module;
    bypass.prepare (fs, 512, 2);

    std::vector<float> signal (4096, 1.0f); // constant input: any click shows up as a jump

    std::vector<float> before (signal.begin(), signal.begin() + 1024);
    run (bypass, module, before, 512);

    bypass.setBypassed (true);
    std::vector<float> after (signal.begin() + 1024, signal.end());
    run (bypass, module, after, 512);

    // Module output 0.5, dry 1.0: the fade moves 0.5 over 480 samples.
    const float maxAllowedStep = 0.5f / static_cast<float> (fadeSamples) * 1.01f;
    before.insert (before.end(), after.begin(), after.end());
    for (std::size_t i = 1; i < before.size(); ++i)
        REQUIRE (std::abs (before[i] - before[i - 1]) <= maxAllowedStep);

    CHECK (before[1023] == 0.5f);                                       // fully active before
    CHECK_THAT (before[1024 + fadeSamples / 2], WithinAbs (0.75, 0.01)); // halfway through the fade
    CHECK (before.back() == 1.0f);                                      // fully bypassed after
}

TEST_CASE ("Re-enabling after bypass fades back in", "[bypass]")
{
    ModuleBypass bypass;
    HalfGainModule module;
    bypass.setBypassed (true);
    bypass.prepare (fs, 256, 1);

    std::vector<float> signal (2048, 1.0f);
    std::vector<float> first (signal.begin(), signal.begin() + 512);
    run (bypass, module, first, 256);

    bypass.setBypassed (false);
    std::vector<float> second (signal.begin() + 512, signal.end());
    run (bypass, module, second, 256);

    CHECK (first.back() == 1.0f);
    CHECK (second.front() < 1.0f);
    CHECK (second.front() > 0.99f);
    CHECK (second.back() == 0.5f);
}

TEST_CASE ("Crossfade works with blocks larger than announced", "[bypass]")
{
    ModuleBypass bypass;
    HalfGainModule module;
    bypass.prepare (fs, 64, 1); // announced 64...

    bypass.setBypassed (true);
    std::vector<float> signal (2000, 1.0f);
    float* channels[] = { signal.data() };
    bypass.process (
        channels, 1, 2000, [&] (float* const* c, int nc, int ns) { module.process (c, nc, ns); }, [] {}); // ...got 2000

    for (std::size_t i = 1; i < signal.size(); ++i)
        REQUIRE (signal[i] >= signal[i - 1]); // smooth monotonic fade from 0.5 up to 1.0

    CHECK (signal.back() == 1.0f);
}
