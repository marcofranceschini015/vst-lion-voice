// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "GainReductionBar.h"
#include "Knob.h"
#include "Palette.h"
#include "Panel.h"
#include "RadioKeysAttachment.h"
#include "dsp/CharacterCompressor.h"

#include <array>
#include <memory>

namespace lionvoice::gui
{

/**
    Live Character panel (1176 style):
        INPUT  OUTPUT  ATTACK  RELEASE  MIX
        RATIO [4] [8] [12] [20] [ALL]
        GR    ■■■■■□□□□            SC HPF [ ]
*/
class CharacterPanel final : public Panel, private juce::Timer
{
public:
    CharacterPanel (juce::AudioProcessorValueTreeState& state, const dsp::CharacterCompressor& compressorToShow);
    ~CharacterPanel() override;

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;
    void timerCallback() override;

    const dsp::CharacterCompressor& compressor;

    Knob input { "INPUT", palette::capBlack };
    Knob output { "OUTPUT", palette::capBlack };
    Knob attack { "ATTACK", palette::capBlack };
    Knob release { "RELEASE", palette::capBlack };
    Knob mix { "MIX", palette::capGrey };

    std::array<std::unique_ptr<juce::ToggleButton>, 5> ratioKeys;
    std::unique_ptr<RadioKeysAttachment> ratioAttachment;

    juce::ToggleButton sidechainKey { "SC HPF" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sidechainAttachment;

    GainReductionBar reductionBar;

    juce::Rectangle<int> ratioLabelArea, reductionLabelArea;
    double lastUpdateMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CharacterPanel)
};

} // namespace lionvoice::gui
