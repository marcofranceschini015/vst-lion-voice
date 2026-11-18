// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "Panel.h"
#include "Palette.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{
namespace
{
constexpr float unavailableAlpha = 0.35f;
constexpr float symbolSize = 18.0f;
} // namespace

Panel::Panel (juce::String titleText, PanelSymbol symbolToShow, bool hasBypassKey)
    : title (std::move (titleText))
    , symbol (symbolToShow)
{
    if (hasBypassKey)
    {
        bypassKey = std::make_unique<juce::ToggleButton> ("BYPASS");
        bypassKey->setColour (juce::ToggleButton::tickColourId, palette::ledRedOn);
        addAndMakeVisible (*bypassKey);
    }
}

Panel::~Panel() = default;

void Panel::attachBypass (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId)
{
    jassert (bypassKey != nullptr);
    if (bypassKey != nullptr)
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, parameterId,
                                                                                                  *bypassKey);
}

void Panel::setAvailable (bool shouldBeAvailable)
{
    available = shouldBeAvailable;

    for (auto* child : getChildren())
    {
        child->setEnabled (available);
        child->setAlpha (available ? 1.0f : unavailableAlpha);
    }

    repaint();
}

void Panel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    steel::drawEngravedPlate (g, bounds);

    auto titleArea = getLocalBounds().removeFromTop (titleHeight).reduced (12, 0).toFloat();

    const auto symbolArea = titleArea.removeFromRight (symbolSize).withSizeKeepingCentre (symbolSize, symbolSize * 0.8f);
    // (setOpacity() would be undone by the colours set inside the helpers: use transparency layers.)
    if (symbol != PanelSymbol::none)
    {
        if (! available)
            g.beginTransparencyLayer (0.6f);

        drawPanelSymbol (g, symbol, symbolArea);

        if (! available)
            g.endTransparencyLayer();
    }

    steel::drawEngravedText (g, title, titleArea, juce::Justification::centredLeft, 13.0f);

    if (! available)
    {
        g.beginTransparencyLayer (0.75f);
        steel::drawEngravedText (g, "COMING SOON", titleArea.withTrimmedRight (8.0f), juce::Justification::centredRight,
                                 9.0f, true, 0.12f);
        g.endTransparencyLayer();
    }

    if (bypassKey != nullptr)
    {
        const auto y = static_cast<float> (getHeight() - bypassHeight);
        steel::drawEngravedLine (g, y, 8.0f, bounds.getWidth() - 8.0f);
    }

    const auto content = getContentArea();
    if (available)
    {
        paintContent (g, content);
    }
    else
    {
        g.beginTransparencyLayer (unavailableAlpha);
        paintContent (g, content);
        g.endTransparencyLayer();
    }
}

void Panel::resized()
{
    if (bypassKey != nullptr)
        bypassKey->setBounds (getLocalBounds().removeFromBottom (bypassHeight).reduced (12, 6));

    layoutContent (getContentArea());
}

juce::Rectangle<int> Panel::getContentArea() const
{
    auto area = getLocalBounds();
    area.removeFromTop (titleHeight);
    if (bypassKey != nullptr)
        area.removeFromBottom (bypassHeight);
    return area.reduced (padding, 4);
}

void Panel::drawSectionLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                              juce::Justification justification)
{
    steel::drawEngravedText (g, text, area.toFloat(), justification, 10.5f, true, 0.1f);
}

} // namespace lionvoice::gui
