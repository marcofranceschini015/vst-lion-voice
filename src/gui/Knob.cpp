// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "Knob.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{

Knob::Knob (juce::String captionText, juce::Colour capColour)
    : caption (std::move (captionText))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, capColour);
    addAndMakeVisible (slider);
}

void Knob::attach (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterId, slider);

    if (auto* parameter = state.getParameter (parameterId))
        slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
}

void Knob::setPlaceholderPosition (double proportion)
{
    slider.setRange (0.0, 1.0);
    slider.setValue (proportion, juce::dontSendNotification);
}

void Knob::paint (juce::Graphics& g)
{
    steel::drawEngravedText (g, caption, getLocalBounds().removeFromBottom (captionHeight).toFloat(),
                             juce::Justification::centred, 10.0f, true, 0.06f);
}

void Knob::resized()
{
    auto area = getLocalBounds();
    area.removeFromBottom (captionHeight);
    const int size = std::min (area.getWidth(), area.getHeight());
    slider.setBounds (area.withSizeKeepingCentre (size, size));
}

void Knob::parentHierarchyChanged()
{
    // Show the value bubble inside the plugin window (not as a separate desktop window).
    slider.setPopupDisplayEnabled (true, true, getTopLevelComponent(), 1500);
}

} // namespace lionvoice::gui
