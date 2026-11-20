// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "InactivePanels.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{
namespace
{

/** Splits an area into equal columns. */
template <std::size_t N>
std::array<juce::Rectangle<int>, N> columns (juce::Rectangle<int> area)
{
    std::array<juce::Rectangle<int>, N> result;
    const int width = area.getWidth() / static_cast<int> (N);
    for (auto& column : result)
        column = area.removeFromLeft (width);
    return result;
}


} // namespace

// ---------------------------------------------------------------------------------------------
DeEsserPanel::DeEsserPanel()
    : Panel ("DE-ESSER", PanelSymbol::deEsser, true)
{
    frequency.setPlaceholderPosition (0.45);
    threshold.setPlaceholderPosition (0.6);
    range.setPlaceholderPosition (0.4);
    listenKey.setColour (juce::ToggleButton::tickColourId, palette::ledGreenOn);

    for (auto* component : std::initializer_list<juce::Component*> { &frequency, &threshold, &range, &listenKey })
        addAndMakeVisible (*component);

    setAvailable (false);
}

void DeEsserPanel::layoutContent (juce::Rectangle<int> area)
{
    frequency.setBounds (area.removeFromTop (area.getHeight() * 2 / 5).withSizeKeepingCentre (110, 125));

    auto middle = area.removeFromTop (area.getHeight() * 2 / 3);
    const auto knobs = columns<2> (middle);
    threshold.setBounds (knobs[0].withSizeKeepingCentre (80, 92));
    range.setBounds (knobs[1].withSizeKeepingCentre (80, 92));

    listenKey.setBounds (area.withSizeKeepingCentre (110, 26));
}

// ---------------------------------------------------------------------------------------------
SaturationPanel::SaturationPanel()
    : Panel ("SATURATION", PanelSymbol::saturation, true)
{
    setAvailable (false);
}

} // namespace lionvoice::gui
