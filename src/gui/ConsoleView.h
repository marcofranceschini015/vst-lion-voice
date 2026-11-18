// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Layout.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    The whole console, always laid out at the design size (layout::baseWidth x baseHeight).
    The editor scales it as one piece with an AffineTransform, so everything stays proportional and sharp.

    Current stage: faceplate, top bar and the empty module plates. Controls arrive in later steps.
*/
class ConsoleView final : public juce::Component
{
public:
    ConsoleView();

    void paint (juce::Graphics& g) override;

private:
    void paintTopBar (juce::Graphics& g) const;
    static void paintPlate (juce::Graphics& g, juce::Rectangle<int> slot, const juce::String& title);

    layout::Slots slots;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConsoleView)
};

} // namespace lionvoice::gui
