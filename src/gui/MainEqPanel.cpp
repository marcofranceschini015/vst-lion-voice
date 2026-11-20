// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "MainEqPanel.h"
#include "Parameters.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{
namespace
{
constexpr int sectionLabelHeight = 14;

/** Three equal columns of a row. */
std::array<juce::Rectangle<int>, 3> threeColumns (juce::Rectangle<int> row)
{
    const int width = row.getWidth() / 3;
    std::array<juce::Rectangle<int>, 3> result;
    for (auto& column : result)
        column = row.removeFromLeft (width).reduced (6, 0);
    return result;
}
} // namespace

MainEqPanel::MainEqPanel (juce::AudioProcessorValueTreeState& state)
    : Panel ("MAIN EQ", PanelSymbol::mainEq, true)
{
    attachBypass (state, params::meqBypass);

    bodyFreq.attach (state, params::meqBodyFreq);
    bodyBoost.attach (state, params::meqBodyBoost);
    bodyAtten.attach (state, params::meqBodyAtten);
    presenceFreq.attach (state, params::meqPresenceFreq);
    presenceQ.attach (state, params::meqPresenceQ);
    presenceBoost.attach (state, params::meqPresenceBoost);
    airFreq.attach (state, params::meqAirFreq);
    airBoost.attach (state, params::meqAirBoost);

    for (auto* knob : { &bodyFreq, &bodyBoost, &bodyAtten, &presenceFreq, &presenceQ, &presenceBoost, &airFreq, &airBoost })
        addAndMakeVisible (*knob);
}

void MainEqPanel::layoutContent (juce::Rectangle<int> area)
{
    const int height = area.getHeight() / 3;
    for (auto& section : sections)
        section = area.removeFromTop (height);

    auto knobRow = [] (juce::Rectangle<int> section)
    { return threeColumns (section.withTrimmedTop (sectionLabelHeight).reduced (0, 4)); };

    const auto body = knobRow (sections[0]);
    bodyFreq.setBounds (body[0]);
    bodyBoost.setBounds (body[1]);
    bodyAtten.setBounds (body[2]);

    const auto presence = knobRow (sections[1]);
    presenceFreq.setBounds (presence[0]);
    presenceQ.setBounds (presence[1]);
    presenceBoost.setBounds (presence[2]);

    const auto air = knobRow (sections[2]);
    airFreq.setBounds (air[0]);
    airBoost.setBounds (air[1]);
}

void MainEqPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    const char* labels[] = { "BODY  50-120 HZ", "PRESENCE  3-6 KHZ", "AIR  8-16 KHZ" };

    for (std::size_t i = 0; i < sections.size(); ++i)
    {
        drawSectionLabel (g, labels[i], sections[i].withHeight (sectionLabelHeight).withTrimmedLeft (2));
        if (i > 0)
            steel::drawEngravedLine (g, static_cast<float> (sections[i].getY()) - 2.0f,
                                     static_cast<float> (sections[i].getX()), static_cast<float> (sections[i].getRight()));
    }
}

} // namespace lionvoice::gui
