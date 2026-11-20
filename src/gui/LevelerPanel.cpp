// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "LevelerPanel.h"
#include "Parameters.h"

namespace lionvoice::gui
{

LevelerPanel::LevelerPanel (juce::AudioProcessorValueTreeState& state, const dsp::Leveler& levelerToShow)
    : Panel ("LEVELER", PanelSymbol::leveler, true)
    , leveler (levelerToShow)
{
    attachBypass (state, params::levBypass);

    gain.attach (state, params::levGain);
    peakReduction.attach (state, params::levPeakReduction);

    limitKey.setColour (juce::ToggleButton::tickColourId, palette::ledAmberOn);
    limitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, params::levLimit,
                                                                                             limitKey);

    for (auto* component : std::initializer_list<juce::Component*> { &meter, &gain, &peakReduction, &limitKey })
        addAndMakeVisible (*component);

    lastUpdateMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

LevelerPanel::~LevelerPanel()
{
    stopTimer();
}

void LevelerPanel::layoutContent (juce::Rectangle<int> area)
{
    meter.setBounds (area.removeFromLeft (230).withSizeKeepingCentre (220, 130));
    area.removeFromLeft (20);

    auto keyColumn = area.removeFromRight (90);
    limitKey.setBounds (keyColumn.withSizeKeepingCentre (80, 26));

    const int half = area.getWidth() / 2;
    gain.setBounds (area.removeFromLeft (half).withSizeKeepingCentre (100, 115));
    peakReduction.setBounds (area.withSizeKeepingCentre (100, 115));
}

void LevelerPanel::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double elapsed = (now - lastUpdateMs) / 1000.0;
    lastUpdateMs = now;

    // Gain-reduction mode: 0 VU = no reduction, the needle falls left by the dB being taken off.
    meter.setTargetDb (leveler.getGainReductionDb());
    meter.advance (std::min (elapsed, 0.1)); // after a stall, don't jump the simulation too far
}

} // namespace lionvoice::gui
