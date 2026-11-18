// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PluginEditor.h"
#include "gui/Layout.h"

namespace
{
namespace layout = lionvoice::gui::layout;

/** Property of the plugin state that remembers the window size (as a scale factor). */
constexpr auto uiScaleProperty = "uiScale";

int scaled (int size, float scale)
{
    return juce::roundToInt (static_cast<float> (size) * scale);
}
} // namespace

LionVoiceAudioProcessorEditor::LionVoiceAudioProcessorEditor (LionVoiceAudioProcessor& audioProcessor)
    : AudioProcessorEditor (audioProcessor)
    , processorRef (audioProcessor)
{
    // Read the saved size first: the resize-limit calls below already trigger resized(), which stores the size.
    const auto savedScale = static_cast<float> (
        static_cast<double> (processorRef.getParameters().state.getProperty (uiScaleProperty, 1.0)));
    const float scale = juce::jlimit (layout::minScale, layout::maxScale, savedScale);

    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (console);

    setResizable (true, true);
    setResizeLimits (scaled (layout::baseWidth, layout::minScale), scaled (layout::baseHeight, layout::minScale),
                     scaled (layout::baseWidth, layout::maxScale), scaled (layout::baseHeight, layout::maxScale));
    getConstrainer()->setFixedAspectRatio (static_cast<double> (layout::baseWidth) / layout::baseHeight);

    setSize (scaled (layout::baseWidth, scale), scaled (layout::baseHeight, scale));
}

LionVoiceAudioProcessorEditor::~LionVoiceAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LionVoiceAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black); // only visible for a moment while resizing
}

void LionVoiceAudioProcessorEditor::resized()
{
    const float scale = static_cast<float> (getWidth()) / static_cast<float> (layout::baseWidth);

    console.setBounds (0, 0, layout::baseWidth, layout::baseHeight);
    console.setTransform (juce::AffineTransform::scale (scale));

    processorRef.getParameters().state.setProperty (uiScaleProperty, scale, nullptr);
}
