// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "ChainPanel.h"
#include "CharacterPanel.h"
#include "InOutPanel.h"
#include "InactivePanels.h"
#include "LevelerPanel.h"
#include "MainEqPanel.h"
#include "Layout.h"
#include "Panel.h"
#include "PluginProcessor.h"
#include "SubtractiveEqPanel.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    The whole console, always laid out at the design size (layout::baseWidth x baseHeight).
    The editor scales it as one piece with an AffineTransform, so everything stays proportional and sharp.

    Live: IN/OUT faders and meters, Subtractive EQ, Leveler, Character, Main EQ, chain LEDs. The other modules are shown as
    "coming soon" placeholders.
*/
class ConsoleView final : public juce::Component
{
public:
    explicit ConsoleView (LionVoiceAudioProcessor& processor);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void paintTopBar (juce::Graphics& g) const;

    layout::Slots slots;

    LevelerPanel leveler;
    CharacterPanel character;
    ChainPanel chain;
    SubtractiveEqPanel subtractiveEq;
    MainEqPanel mainEq;
    DeEsserPanel deEsser;
    SaturationPanel saturation;
    InOutPanel inOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConsoleView)
};

} // namespace lionvoice::gui
