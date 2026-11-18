// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "TestSignals.h"
#include "dsp/LevelMeter.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <vector>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

namespace
{
constexpr double fs = 48000.0;

void feed (LevelMeter& meter, std::vector<float>& left, std::vector<float>& right, int blockSize = 512)
{
    for (std::size_t start = 0; start < left.size(); start += static_cast<std::size_t> (blockSize))
    {
        const auto n = std::min (static_cast<std::size_t> (blockSize), left.size() - start);
        const float* channels[] = { left.data() + start, right.data() + start };
        meter.process (channels, 2, static_cast<int> (n));
    }
}

double toDb (float linear)
{
    return 20.0 * std::log10 (static_cast<double> (linear));
}
} // namespace

TEST_CASE ("Sine at -6 dBFS: peak -6.0 dB, RMS -9.0 dB", "[meter]")
{
    LevelMeter meter;
    meter.prepare (fs);

    auto left = lionvoice::test::makeSine (fs, 1000.0, 2.0, 0.5f);
    std::vector<float> right (left.size(), 0.0f);
    feed (meter, left, right);

    CHECK_THAT (toDb (meter.takePeak (0)), WithinAbs (-6.02, 0.05));
    CHECK_THAT (toDb (meter.getRms (0)), WithinAbs (-9.03, 0.1));

    // The silent channel reads silence.
    CHECK (meter.takePeak (1) == 0.0f);
    CHECK (meter.getRms (1) == 0.0f);
}

TEST_CASE ("RMS follows a 300 ms time constant", "[meter]")
{
    LevelMeter meter;
    meter.prepare (fs);

    std::vector<float> left (static_cast<std::size_t> (fs * 0.3), 1.0f); // DC at full scale for exactly one tau
    std::vector<float> right (left.size(), 0.0f);
    feed (meter, left, right);

    // After one time constant the mean square reaches 1 - e^-1 = 63 %.
    CHECK_THAT (meter.getRms (0), WithinAbs (std::sqrt (1.0 - std::exp (-1.0)), 0.01));
}

TEST_CASE ("Peak is kept until the GUI reads it, then cleared", "[meter]")
{
    LevelMeter meter;
    meter.prepare (fs);

    std::vector<float> left (512, 0.0f), right (512, 0.0f);
    left[100] = 0.8f; // one short spike
    feed (meter, left, right);

    std::vector<float> quietLeft (4096, 0.01f), quietRight (4096, 0.0f);
    feed (meter, quietLeft, quietRight); // later, quieter blocks must not hide the spike

    CHECK (meter.takePeak (0) == 0.8f);
    CHECK (meter.takePeak (0) == 0.0f); // cleared by the read
}

TEST_CASE ("Clip flag is sticky until reset", "[meter]")
{
    LevelMeter meter;
    meter.prepare (fs);

    std::vector<float> left (256, 0.2f), right (256, 0.2f);
    feed (meter, left, right);
    CHECK_FALSE (meter.hasClipped (0));

    right[10] = -1.0f; // negative full-scale sample also clips
    feed (meter, left, right);
    CHECK_FALSE (meter.hasClipped (0));
    CHECK (meter.hasClipped (1));

    std::vector<float> quiet (256, 0.0f), quiet2 (256, 0.0f);
    feed (meter, quiet, quiet2);
    CHECK (meter.hasClipped (1)); // still lit

    meter.resetClip();
    CHECK_FALSE (meter.hasClipped (1));
}

TEST_CASE ("Mono input is reported as one channel", "[meter]")
{
    LevelMeter meter;
    meter.prepare (fs);

    std::vector<float> mono (256, 0.25f);
    const float* channels[] = { mono.data() };
    meter.process (channels, 1, 256);

    CHECK (meter.getNumChannels() == 1);
    CHECK (meter.takePeak (0) == 0.25f);
}
