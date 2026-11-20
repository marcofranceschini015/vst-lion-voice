// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Knob.h"
#include "Palette.h"
#include "Panel.h"
#include "VuMeter.h"
#include "dsp/Leveler.h"

#include <memory>

namespace lionvoice::gui
{

/**
    Live Leveler panel (LA-2A style): gain-reduction VU, GAIN, PEAK REDUCTION, LIMIT key and BYPASS.
    The VU reads the leveler's gain reduction 30 times per second.
*/
class LevelerPanel final : public Panel, private juce::Timer
{
public:
    LevelerPanel (juce::AudioProcessorValueTreeState& state, const dsp::Leveler& levelerToShow);
    ~LevelerPanel() override;

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void timerCallback() override;

    const dsp::Leveler& leveler;

    VuMeter meter;
    Knob gain { "GAIN", palette::capGrey };
    Knob peakReduction { "PEAK REDUCTION", palette::capGrey };
    juce::ToggleButton limitKey { "LIMIT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limitAttachment;

    double lastUpdateMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelerPanel)
};

} // namespace lionvoice::gui
