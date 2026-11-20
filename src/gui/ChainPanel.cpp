// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "ChainPanel.h"
#include "LionLookAndFeel.h"
#include "Palette.h"
#include "Parameters.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{

ChainPanel::ChainPanel (juce::AudioProcessorValueTreeState& state)
    : Panel ("CHAIN", PanelSymbol::none, false)
{
    auto* subBypass = state.getRawParameterValue (lionvoice::params::subBypass);
    auto* levBypass = state.getRawParameterValue (lionvoice::params::levBypass);
    auto* fetBypass = state.getRawParameterValue (lionvoice::params::fetBypass);

    // Signal order. Future modules get their bypass parameter here when they are implemented.
    stages = { {
        { "IN", nullptr, true, true },
        { "SUB EQ", subBypass, true, true },
        { "LEVELER", levBypass, true, true },
        { "CHARACTER", fetBypass, true, true },
        { "MAIN EQ", nullptr, false, false },
        { "SATURATION", nullptr, false, false },
        { "DE-ESSER", nullptr, false, false },
        { "OUT", nullptr, true, true },
    } };

    updateLeds();
    startTimerHz (15);
}

ChainPanel::~ChainPanel()
{
    stopTimer();
}

bool ChainPanel::updateLeds()
{
    bool changed = false;

    for (auto& stage : stages)
    {
        if (stage.bypassParameter == nullptr)
            continue;

        const bool lit = stage.implemented && stage.bypassParameter->load (std::memory_order_relaxed) < 0.5f;
        changed = changed || lit != stage.lit;
        stage.lit = lit;
    }

    return changed;
}

void ChainPanel::timerCallback()
{
    if (updateLeds())
        repaint();
}

void ChainPanel::paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea)
{
    auto area = contentArea.toFloat().reduced (2.0f, 2.0f);
    const float rowHeight = area.getHeight() / static_cast<float> (stages.size());
    constexpr float ledSize = 9.0f;

    for (std::size_t i = 0; i < stages.size(); ++i)
    {
        const auto& stage = stages[i];
        auto row = area.removeFromTop (rowHeight);
        const float ledX = row.getX() + 4.0f;

        // Arrow joining this stage to the next one.
        if (i + 1 < stages.size())
        {
            const float x = ledX + ledSize * 0.5f;
            const float y0 = row.getCentreY() + ledSize * 0.5f + 2.0f;
            const float y1 = row.getBottom() + rowHeight * 0.5f - ledSize * 0.5f - 2.0f;
            g.setColour (palette::engraveInkSoft);
            g.drawLine (x, y0, x, y1, 1.0f);
            g.fillPath ([&] {
                juce::Path head;
                head.addTriangle (x - 2.5f, y1 - 3.5f, x + 2.5f, y1 - 3.5f, x, y1);
                return head;
            }());
        }

        LionLookAndFeel::drawLed (g, { ledX, row.getCentreY() - ledSize * 0.5f, ledSize, ledSize }, stage.lit,
                                  palette::ledGreenOn);

        // Modules that don't exist yet are printed fainter.
        if (! stage.implemented)
            g.beginTransparencyLayer (0.45f);

        steel::drawEngravedText (g, stage.name, row.withTrimmedLeft (ledSize + 12.0f),
                                 juce::Justification::centredLeft, 10.5f, true, 0.08f);

        if (! stage.implemented)
            g.endTransparencyLayer();
    }
}

} // namespace lionvoice::gui
