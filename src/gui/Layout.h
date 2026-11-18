// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_graphics/juce_graphics.h>

/**
    Geometry of the console, in "design units" at 100 % size (baseWidth x baseHeight).
    The editor scales the whole console uniformly, so these numbers never change with the window size.

    Arrangement (from the GUI sketch):

        [ top bar: logo + name                                                        ]
        [ LEVELER (LA-2A)                ][ CHARACTER (FET)             ][ CHAIN     ]
        [ SUB EQ ][ MAIN EQ             ][ DE-ESSER    ][ SATURATION   ][ IN / OUT  ]
*/
namespace lionvoice::gui::layout
{

inline constexpr int baseWidth = 1180;
inline constexpr int baseHeight = 780;

inline constexpr int margin = 14;
inline constexpr int gap = 10;
inline constexpr int topBarHeight = 50;
inline constexpr int rightColumnWidth = 140;
inline constexpr int topRowHeight = 268;
inline constexpr int subEqWidth = 236;
inline constexpr int mainEqWidth = 300;

inline constexpr float minScale = 0.7f;
inline constexpr float maxScale = 1.5f;

struct Slots
{
    juce::Rectangle<int> topBar;
    juce::Rectangle<int> leveler, character, chain;
    juce::Rectangle<int> subEq, mainEq, deEsser, saturation, inOut;
};

inline Slots computeSlots()
{
    Slots s;
    auto area = juce::Rectangle<int> (0, 0, baseWidth, baseHeight).reduced (margin);

    s.topBar = area.removeFromTop (topBarHeight);
    area.removeFromTop (gap);

    auto right = area.removeFromRight (rightColumnWidth);
    area.removeFromRight (gap);
    s.chain = right.removeFromTop (topRowHeight);
    right.removeFromTop (gap);
    s.inOut = right;

    auto topRow = area.removeFromTop (topRowHeight);
    area.removeFromTop (gap);
    auto bottomRow = area;

    s.leveler = topRow.removeFromLeft (subEqWidth + gap + mainEqWidth);
    topRow.removeFromLeft (gap);
    s.character = topRow;

    s.subEq = bottomRow.removeFromLeft (subEqWidth);
    bottomRow.removeFromLeft (gap);
    s.mainEq = bottomRow.removeFromLeft (mainEqWidth);
    bottomRow.removeFromLeft (gap);
    const int remaining = (bottomRow.getWidth() - gap) / 2;
    s.deEsser = bottomRow.removeFromLeft (remaining);
    bottomRow.removeFromLeft (gap);
    s.saturation = bottomRow;

    return s;
}

} // namespace lionvoice::gui::layout
