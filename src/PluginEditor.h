// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "PluginProcessor.h"
#include "gui/ConsoleView.h"
#include "gui/LionLookAndFeel.h"

/**
    The plugin window. Runs on the message (GUI) thread, never on the audio thread.

    Hosts the console (designed at a fixed size) and scales it uniformly when the window is resized.
    The chosen size is stored in the plugin state, so it is restored with the project.
*/
class LionVoiceAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LionVoiceAudioProcessorEditor (LionVoiceAudioProcessor& audioProcessor);
    ~LionVoiceAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    LionVoiceAudioProcessor& processorRef;
    lionvoice::gui::LionLookAndFeel lookAndFeel; // must outlive the components that use it
    lionvoice::gui::ConsoleView console;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LionVoiceAudioProcessorEditor)
};
