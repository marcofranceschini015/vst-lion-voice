// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PluginEditor.h"

namespace
{
constexpr int defaultWidth = 1280;
constexpr int defaultHeight = 760;
} // namespace

LionVoiceAudioProcessorEditor::LionVoiceAudioProcessorEditor (LionVoiceAudioProcessor& audioProcessor)
    : AudioProcessorEditor (audioProcessor)
{
    setSize (defaultWidth, defaultHeight);
}

void LionVoiceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff3a3d41)); // dark steel grey placeholder

    auto area = getLocalBounds();

    g.setColour (juce::Colour (0xffd8dadc));
    g.setFont (juce::FontOptions (48.0f, juce::Font::bold));
    g.drawText ("LION VOICE", area.removeFromTop (area.getHeight() / 2 + 24), juce::Justification::centredBottom);

    g.setColour (juce::Colour (0xff9aa0a6));
    g.setFont (juce::FontOptions (18.0f));
    g.drawText ("MVP 1 - passthrough build", area.withTrimmedTop (12), juce::Justification::centredTop);
}

void LionVoiceAudioProcessorEditor::resized() {}
