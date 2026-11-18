// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "dsp/Decibels.h"
#include "dsp/Smoother.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace lionvoice::dsp;
using Catch::Matchers::WithinAbs;

TEST_CASE ("Smoother reaches the target in exactly the ramp length", "[smoother]")
{
    LinearSmoother s;
    s.reset (48000.0, 0.020); // 960 samples
    s.setCurrentAndTarget (0.0f);
    s.setTarget (1.0f);

    for (int i = 0; i < 959; ++i)
        s.getNext();

    REQUIRE (s.isSmoothing());
    REQUIRE (s.getCurrent() < 1.0f);

    REQUIRE (s.getNext() == 1.0f);
    REQUIRE_FALSE (s.isSmoothing());
    REQUIRE (s.getNext() == 1.0f);
}

TEST_CASE ("Smoother moves monotonically and never overshoots", "[smoother]")
{
    LinearSmoother s;
    s.reset (44100.0, 0.050);

    SECTION ("upwards")
    {
        s.setCurrentAndTarget (-24.0f);
        s.setTarget (24.0f);

        float previous = s.getCurrent();
        while (s.isSmoothing())
        {
            const float v = s.getNext();
            REQUIRE (v >= previous);
            REQUIRE (v <= 24.0f);
            previous = v;
        }
    }

    SECTION ("downwards")
    {
        s.setCurrentAndTarget (1.0f);
        s.setTarget (0.1f);

        float previous = s.getCurrent();
        while (s.isSmoothing())
        {
            const float v = s.getNext();
            REQUIRE (v <= previous);
            REQUIRE (v >= 0.1f);
            previous = v;
        }
    }
}

TEST_CASE ("Smoother: re-targeting mid-ramp starts from the current value", "[smoother]")
{
    LinearSmoother s;
    s.reset (1000.0, 0.010); // 10 samples
    s.setCurrentAndTarget (0.0f);
    s.setTarget (10.0f);

    for (int i = 0; i < 5; ++i)
        s.getNext();

    REQUIRE_THAT (s.getCurrent(), WithinAbs (5.0, 1.0e-5));

    s.setTarget (0.0f); // turn back
    REQUIRE_THAT (s.getNext(), WithinAbs (4.5, 1.0e-5));
}

TEST_CASE ("Smoother: setting the same target does not start a ramp", "[smoother]")
{
    LinearSmoother s;
    s.reset (48000.0, 0.020);
    s.setCurrentAndTarget (0.5f);
    s.setTarget (0.5f);

    REQUIRE_FALSE (s.isSmoothing());
}

TEST_CASE ("Smoother: skip() advances like repeated getNext()", "[smoother]")
{
    LinearSmoother a, b;
    for (auto* s : { &a, &b })
    {
        s->reset (48000.0, 0.010);
        s->setCurrentAndTarget (0.0f);
        s->setTarget (2.0f);
    }

    a.skip (100);
    for (int i = 0; i < 100; ++i)
        b.getNext();

    REQUIRE_THAT (a.getCurrent(), WithinAbs (b.getCurrent(), 1.0e-5));

    a.skip (100000);
    REQUIRE (a.getCurrent() == 2.0f);
    REQUIRE_FALSE (a.isSmoothing());
}

TEST_CASE ("Decibel conversions", "[decibels]")
{
    CHECK (dbToGain (0.0f) == 1.0f);
    CHECK_THAT (dbToGain (-6.0f), WithinAbs (0.501187, 1.0e-5));
    CHECK_THAT (dbToGain (6.0f), WithinAbs (1.995262, 1.0e-5));
    CHECK (dbToGain (minusInfinityDb) == 0.0f);

    CHECK (gainToDb (0.0f) == minusInfinityDb);
    CHECK (gainToDb (-1.0f) == minusInfinityDb);
    CHECK (gainToDb (1.0e-9f) == minusInfinityDb);

    for (float db = -60.0f; db <= 24.0f; db += 0.5f)
        CHECK_THAT (gainToDb (dbToGain (db)), WithinAbs (db, 1.0e-3));
}
