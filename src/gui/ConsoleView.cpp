// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "ConsoleView.h"
#include "LionLogo.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{

ConsoleView::ConsoleView (LionVoiceAudioProcessor& processor)
    : slots (layout::computeSlots())
    , leveler (processor.getParameters(), processor.getLeveler())
    , character (processor.getParameters(), processor.getCharacter())
    , chain (processor.getParameters())
    , subtractiveEq (processor.getParameters())
    , mainEq (processor.getParameters())
    , inOut (processor.getParameters(), processor.getInputMeter(), processor.getOutputMeter())
{
    for (auto* panel : std::initializer_list<juce::Component*> { &leveler, &character, &chain, &subtractiveEq, &mainEq,
                                                                  &deEsser, &saturation, &inOut })
        addAndMakeVisible (*panel);

    setSize (layout::baseWidth, layout::baseHeight);
    setOpaque (true);
}

void ConsoleView::resized()
{
    leveler.setBounds (slots.leveler);
    character.setBounds (slots.character);
    chain.setBounds (slots.chain);
    subtractiveEq.setBounds (slots.subEq);
    mainEq.setBounds (slots.mainEq);
    deEsser.setBounds (slots.deEsser);
    saturation.setBounds (slots.saturation);
    inOut.setBounds (slots.inOut);
}

void ConsoleView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    steel::drawBrushedSteel (g, bounds);

    paintTopBar (g);

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

    // Engraved lioness, then the name. The right side of the bar stays free (future AI assistant button).
    logo::drawEngravedLioness (g, bar.removeFromLeft (52.0f).reduced (0.0f, 2.0f));
    bar.removeFromLeft (14.0f);

    steel::drawEngravedText (g, "LION VOICE", bar.removeFromLeft (260.0f), juce::Justification::centredLeft, 30.0f,
                             true, 0.18f);
    steel::drawEngravedText (g, "VOCAL CHANNEL STRIP", bar.removeFromLeft (260.0f).translated (0.0f, 4.0f),
                             juce::Justification::centredLeft, 12.0f, true, 0.25f);

    // Engraved rule under the top bar.
    steel::drawEngravedLine (g, static_cast<float> (slots.topBar.getBottom()) + 2.0f,
                             static_cast<float> (slots.topBar.getX()), static_cast<float> (slots.topBar.getRight()));
}

} // namespace lionvoice::gui
