// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace lionvoice::gui
{

/**
    An SSL knob with an engraved caption underneath.

    - attach(): connects it to a plugin parameter (two-way: moving the knob changes the parameter,
      automation moves the knob). Double-click returns to the default value.
    - The value appears in a small bubble while dragging or hovering.
    - Without attach() it is a placeholder for modules that are not available yet.
*/
class Knob final : public juce::Component
{
public:
    Knob (juce::String captionText, juce::Colour capColour);

    void attach (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId);

    /** For placeholders: shows the pointer at a position between 0 and 1. */
    void setPlaceholderPosition (double proportion);

    juce::Slider& getSlider() noexcept { return slider; }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;

    static constexpr int captionHeight = 13;

private:
    juce::Slider slider;
    juce::String caption;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};

} // namespace lionvoice::gui
