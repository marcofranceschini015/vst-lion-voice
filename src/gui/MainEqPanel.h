// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Knob.h"
#include "Palette.h"
#include "Panel.h"

#include <array>

namespace lionvoice::gui
{

/**
    Live Main EQ panel, three sections stacked as in the GUI sketch:
        BODY      FREQ  BOOST  ATTEN    (Pultec style)
        PRESENCE  FREQ  Q      BOOST
        AIR       FREQ  BOOST           (Baxandall style shelf)
*/
class MainEqPanel final : public Panel
{
public:
    explicit MainEqPanel (juce::AudioProcessorValueTreeState& state);

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;

    Knob bodyFreq { "FREQ", palette::capBrown };
    Knob bodyBoost { "BOOST", palette::capBrown };
    Knob bodyAtten { "ATTEN", palette::capBrown };
    Knob presenceFreq { "FREQ", palette::capGreen };
    Knob presenceQ { "Q", palette::capGrey };
    Knob presenceBoost { "BOOST", palette::capGreen };
    Knob airFreq { "FREQ", palette::capRed };
    Knob airBoost { "BOOST", palette::capRed };

    std::array<juce::Rectangle<int>, 3> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainEqPanel)
};

} // namespace lionvoice::gui
