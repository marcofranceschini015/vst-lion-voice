// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "LevelMeterDisplay.h"
#include "Panel.h"

#include <memory>

namespace lionvoice::gui
{

/**
    IN / OUT section: input and output gain faders, each next to its stereo meter, with a shared dB scale
    in the middle:   [IN fader][IN meter]  scale  [OUT meter][OUT fader]

    The IN meter shows the -12 ... -10 dBFS target zone for vocal peaks.
*/
class InOutPanel final : public Panel, private juce::Timer
{
public:
    InOutPanel (juce::AudioProcessorValueTreeState& state, dsp::LevelMeter& inputMeter, dsp::LevelMeter& outputMeter);
    ~InOutPanel() override;

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;
    void timerCallback() override;

    static void setUpFader (juce::Slider& fader);

    juce::Slider inFader, outFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAttachment, outAttachment;
    LevelMeterDisplay inMeter, outMeter;

    juce::Rectangle<int> scaleArea, inLabelArea, outLabelArea;
    double lastUpdateMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InOutPanel)
};

} // namespace lionvoice::gui
