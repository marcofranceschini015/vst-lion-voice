// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "InactivePanels.h"
#include "Layout.h"
#include "Panel.h"
#include "SubtractiveEqPanel.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    The whole console, always laid out at the design size (layout::baseWidth x baseHeight).
    The editor scales it as one piece with an AffineTransform, so everything stays proportional and sharp.

    Current stage: all module panels; only the Subtractive EQ is live. The chain lights, the logo and the
    IN/OUT faders arrive in the next steps.
*/
class ConsoleView final : public juce::Component
{
public:
    explicit ConsoleView (juce::AudioProcessorValueTreeState& state);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void paintTopBar (juce::Graphics& g) const;

    layout::Slots slots;

    LevelerPanel leveler;
    CharacterPanel character;
    Panel chain { "CHAIN", PanelSymbol::none, false };
    SubtractiveEqPanel subtractiveEq;
    MainEqPanel mainEq;
    DeEsserPanel deEsser;
    SaturationPanel saturation;
    Panel inOut { "IN / OUT", PanelSymbol::none, false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConsoleView)
};

} // namespace lionvoice::gui
