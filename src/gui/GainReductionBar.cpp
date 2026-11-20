// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "GainReductionBar.h"
#include "Palette.h"
#include "SteelBackground.h"

#include <array>

namespace lionvoice::gui
{
namespace
{
constexpr std::array<int, 9> stepsDb { 1, 2, 3, 4, 6, 8, 10, 14, 20 };
constexpr float fallbackDbPerSecond = 30.0f;
constexpr float labelHeight = 10.0f;
} // namespace

void GainReductionBar::update (float reductionDb, double elapsedSeconds)
{
    const float recovered = displayedDb + fallbackDbPerSecond * static_cast<float> (elapsedSeconds);
    const float next = std::min (reductionDb, std::min (0.0f, recovered));

    if (std::abs (next - displayedDb) > 0.05f)
    {
        displayedDb = next;
        repaint();
    }
}

void GainReductionBar::paint (juce::Graphics& g)
{
    drawBar (g, getLocalBounds().toFloat(), displayedDb);
}

void GainReductionBar::drawBar (juce::Graphics& g, juce::Rectangle<float> area, float reductionDb)
{
    auto labels = area.removeFromBottom (labelHeight);
    const auto slot = area.reduced (0.0f, 1.0f);

    // Dark window cut into the steel.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (slot.expanded (1.5f), 3.0f);
    g.setColour (juce::Colour (0xff0d0e10));
    g.fillRoundedRectangle (slot, 2.0f);

    const auto segments = slot.reduced (3.0f, 2.5f);
    const float gap = 3.0f;
    const auto count = static_cast<float> (stepsDb.size());
    const float width = (segments.getWidth() - gap * (count - 1.0f)) / count;

    for (std::size_t i = 0; i < stepsDb.size(); ++i)
    {
        const auto segment = juce::Rectangle<float> (segments.getX() + static_cast<float> (i) * (width + gap),
                                                     segments.getY(), width, segments.getHeight());
        const bool lit = reductionDb <= -static_cast<float> (stepsDb[i]) + 0.01f;
        const auto colour = stepsDb[i] >= 10 ? palette::ledRedOn : palette::ledAmberOn;

        if (lit)
        {
            g.setColour (colour.withAlpha (0.35f));
            g.fillRoundedRectangle (segment.expanded (1.5f), 2.0f);
        }
        g.setColour (lit ? colour : colour.withAlpha (0.12f));
        g.fillRoundedRectangle (segment, 1.5f);

        steel::drawEngravedText (g, juce::String (stepsDb[i]),
                                 juce::Rectangle<float> (segment.getX() - 4.0f, labels.getY(), width + 8.0f, labelHeight),
                                 juce::Justification::centred, 8.5f, true, 0.0f);
    }
}

} // namespace lionvoice::gui
