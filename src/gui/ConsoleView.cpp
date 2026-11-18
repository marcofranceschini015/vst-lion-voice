// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "ConsoleView.h"
#include "Palette.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{

ConsoleView::ConsoleView()
    : slots (layout::computeSlots())
{
    setSize (layout::baseWidth, layout::baseHeight);
    setOpaque (true);
}

void ConsoleView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    steel::drawBrushedSteel (g, bounds);

    paintTopBar (g);

    paintPlate (g, slots.leveler, "LEVELER");
    paintPlate (g, slots.character, "CHARACTER");
    paintPlate (g, slots.chain, "CHAIN");
    paintPlate (g, slots.subEq, "SUBTRACTIVE EQ");
    paintPlate (g, slots.mainEq, "MAIN EQ");
    paintPlate (g, slots.deEsser, "DE-ESSER");
    paintPlate (g, slots.saturation, "SATURATION");
    paintPlate (g, slots.inOut, "IN / OUT");

    // Corner screws holding the faceplate.
    const float inset = 7.0f;
    const float r = 4.5f;
    steel::drawScrew (g, { inset + r - 2.0f, inset + r - 2.0f }, r, 0.4f);
    steel::drawScrew (g, { bounds.getRight() - inset - r + 2.0f, inset + r - 2.0f }, r, 1.1f);
    steel::drawScrew (g, { inset + r - 2.0f, bounds.getBottom() - inset - r + 2.0f }, r, 0.9f);
    steel::drawScrew (g, { bounds.getRight() - inset - r + 2.0f, bounds.getBottom() - inset - r + 2.0f }, r, 0.2f);
}

void ConsoleView::paintTopBar (juce::Graphics& g) const
{
    auto bar = slots.topBar.toFloat().reduced (10.0f, 0.0f);

    // Space for the engraved lioness logo (added in a later step).
    bar.removeFromLeft (52.0f);

    steel::drawEngravedText (g, "LION VOICE", bar.removeFromLeft (260.0f), juce::Justification::centredLeft, 30.0f,
                             true, 0.18f);
    steel::drawEngravedText (g, "VOCAL CHANNEL STRIP", bar.removeFromLeft (260.0f).translated (0.0f, 4.0f),
                             juce::Justification::centredLeft, 12.0f, true, 0.25f);

    // Engraved rule under the top bar.
    const float y = static_cast<float> (slots.topBar.getBottom()) + 2.0f;
    g.setColour (palette::engraveShadow);
    g.drawHorizontalLine (juce::roundToInt (y), static_cast<float> (slots.topBar.getX()),
                          static_cast<float> (slots.topBar.getRight()));
    g.setColour (palette::engraveHighlight);
    g.drawHorizontalLine (juce::roundToInt (y) + 1, static_cast<float> (slots.topBar.getX()),
                          static_cast<float> (slots.topBar.getRight()));
}

void ConsoleView::paintPlate (juce::Graphics& g, juce::Rectangle<int> slot, const juce::String& title)
{
    const auto area = slot.toFloat();
    steel::drawEngravedPlate (g, area);
    steel::drawEngravedText (g, title, area.reduced (12.0f, 0.0f).removeFromTop (30.0f),
                             juce::Justification::centredLeft, 13.0f);
}

} // namespace lionvoice::gui
