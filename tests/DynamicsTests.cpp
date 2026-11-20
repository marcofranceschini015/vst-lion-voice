// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "dsp/dynamics/Ballistics.h"
#include "dsp/dynamics/GainComputer.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

// --- Gain computer -------------------------------------------------------------------------------

TEST_CASE ("Below the knee the signal is untouched", "[dynamics][curve]")
{
    const GainComputer gc { -20.0f, 4.0f, 6.0f };

    for (float x = -80.0f; x <= -23.0f; x += 0.5f)
        REQUIRE (gc.gainReductionDb (x) == 0.0f);
}

TEST_CASE ("Above the knee the slope is exactly 1/ratio", "[dynamics][curve]")
{
    const float ratio = GENERATE (2.0f, 3.0f, 4.0f, 8.0f, 20.0f);
    const GainComputer gc { -18.0f, ratio, 6.0f };

    for (float x = -14.0f; x <= 6.0f; x += 1.0f)
    {
        CHECK_THAT (gc.outputLevelDb (x), WithinAbs (-18.0f + (x + 18.0f) / ratio, 1.0e-4));
        // 1 dB more in -> 1/R dB more out.
        CHECK_THAT (gc.outputLevelDb (x + 1.0f) - gc.outputLevelDb (x), WithinAbs (1.0f / ratio, 1.0e-4));
    }
}

TEST_CASE ("Soft knee: value at the threshold and smooth joins", "[dynamics][curve]")
{
    const float ratio = GENERATE (3.0f, 10.0f);
    const float knee = GENERATE (2.0f, 6.0f, 14.0f);
    const float threshold = -24.0f;
    const GainComputer gc { threshold, ratio, knee };

    // In the middle of the knee the reduction is (1/R - 1) * W / 8.
    CHECK_THAT (gc.gainReductionDb (threshold), WithinAbs ((1.0f / ratio - 1.0f) * knee / 8.0f, 1.0e-4));

    // No jumps and no corners where the knee starts and ends (value and slope continuous).
    constexpr float h = 0.001f;
    for (const float edge : { threshold - knee * 0.5f, threshold + knee * 0.5f })
    {
        CHECK_THAT (gc.outputLevelDb (edge + h), WithinAbs (gc.outputLevelDb (edge - h), 1.0e-2));

        const float slopeBefore = (gc.outputLevelDb (edge - h) - gc.outputLevelDb (edge - 2.0f * h)) / h;
        const float slopeAfter = (gc.outputLevelDb (edge + 2.0f * h) - gc.outputLevelDb (edge + h)) / h;
        CHECK_THAT (slopeAfter, WithinAbs (slopeBefore, 0.02));
    }
}

TEST_CASE ("The curve never turns a louder input into a quieter output", "[dynamics][curve]")
{
    const float ratio = GENERATE (1.5f, 4.0f, 20.0f, 1000.0f);
    const float knee = GENERATE (0.0f, 4.0f, 14.0f);
    const GainComputer gc { -20.0f, ratio, knee };

    float previous = gc.outputLevelDb (-80.0f);
    for (float x = -79.9f; x <= 12.0f; x += 0.1f)
    {
        const float y = gc.outputLevelDb (x);
        REQUIRE (y >= previous - 1.0e-5f);
        REQUIRE (gc.gainReductionDb (x) <= 0.0f);
        previous = y;
    }
}

TEST_CASE ("Edge cases: hard knee, ratio 1, limiter", "[dynamics][curve]")
{
    SECTION ("hard knee has no reduction at the threshold and no NaN")
    {
        const GainComputer gc { -10.0f, 4.0f, 0.0f };
        CHECK (gc.gainReductionDb (-10.0f) == 0.0f);
        CHECK_THAT (gc.gainReductionDb (-6.0f), WithinAbs (-3.0, 1.0e-5));
    }

    SECTION ("ratio 1 never compresses")
    {
        const GainComputer gc { -30.0f, 1.0f, 6.0f };
        for (float x = -60.0f; x <= 10.0f; x += 1.0f)
            CHECK (gc.gainReductionDb (x) == 0.0f);
    }

    SECTION ("a huge ratio holds the output at the threshold")
    {
        const GainComputer gc { -6.0f, 1000.0f, 0.0f };
        CHECK_THAT (gc.outputLevelDb (12.0f), WithinAbs (-6.0, 0.05));
    }
}

// --- Ballistics ------------------------------------------------------------------------------------

namespace
{
/** Runs the ballistics for a number of seconds against a constant target. */
float runFor (Ballistics& b, double seconds, double fs, float target)
{
    const auto samples = static_cast<int> (std::lround (seconds * fs));
    float value = b.getCurrent();
    for (int i = 0; i < samples; ++i)
        value = b.process (target);
    return value;
}

constexpr float oneTimeConstant = 0.6321206f; // 1 - e^-1
} // namespace

TEST_CASE ("Attack covers 63 % of the way in one attack time", "[dynamics][ballistics]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);

    Ballistics b;
    b.prepare (fs);
    b.setTimes (0.010, 0.200);

    CHECK_THAT (runFor (b, 0.010, fs, -10.0f), WithinAbs (-10.0f * oneTimeConstant, 0.01));
    CHECK_THAT (runFor (b, 0.090, fs, -10.0f), WithinAbs (-10.0, 0.01)); // ~10 time constants: settled
}

TEST_CASE ("Release recovers 63 % of the way in one release time", "[dynamics][ballistics]")
{
    const double fs = 48000.0;

    Ballistics b;
    b.prepare (fs);
    b.setTimes (0.001, 0.300);
    b.reset (-12.0f);

    CHECK_THAT (runFor (b, 0.300, fs, 0.0f), WithinAbs (-12.0f * (1.0f - oneTimeConstant), 0.02));
}

TEST_CASE ("A new peak during release switches straight back to the attack time", "[dynamics][ballistics]")
{
    const double fs = 48000.0;

    Ballistics b;
    b.prepare (fs);
    b.setTimes (0.001, 1.0);
    b.reset (-6.0f);

    runFor (b, 0.05, fs, 0.0f); // releasing slowly...
    const float before = b.getCurrent();
    CHECK (before > -6.0f);

    // ...then a louder peak: with a 1 ms attack it must reach -12 dB within a few milliseconds.
    CHECK_THAT (runFor (b, 0.01, fs, -12.0f), WithinAbs (-12.0, 0.01));
}

TEST_CASE ("Zero times are instant and the output never overshoots", "[dynamics][ballistics]")
{
    Ballistics instant;
    instant.prepare (48000.0);
    instant.setTimes (0.0, 0.0);
    CHECK (instant.process (-7.0f) == -7.0f);
    CHECK (instant.process (0.0f) == 0.0f);

    Ballistics b;
    b.prepare (48000.0);
    b.setTimes (0.005, 0.050);
    for (int i = 0; i < 48000; ++i)
    {
        const float v = b.process (i < 24000 ? -9.0f : 0.0f);
        REQUIRE (v >= -9.0f);
        REQUIRE (v <= 0.0f);
    }
}
