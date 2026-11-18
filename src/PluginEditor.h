// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "PluginProcessor.h"

/**
    The plugin window. Runs on the message (GUI) thread, never on the audio thread.

    Current stage: placeholder window. The SSL-style interface arrives in later steps.
*/
class LionVoiceAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LionVoiceAudioProcessorEditor (LionVoiceAudioProcessor& audioProcessor);
    ~LionVoiceAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LionVoiceAudioProcessorEditor)
};
