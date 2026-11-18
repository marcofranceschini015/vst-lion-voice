// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    The SSL-console look of Lion Voice.

    - Rotary sliders are drawn as SSL knobs: printed scale on the steel, dark knurled skirt, coloured cap
      (Slider::rotarySliderFillColourId) and a pointer line.
    - Toggle buttons are drawn as square console keys with a built-in LED
      (ToggleButton::tickColourId = LED colour when on).
*/
class LionLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LionLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    /** Draws one knob; also used directly by previews. */
    static void drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion, float startAngle,
                          float endAngle, juce::Colour capColour);

    /** Draws a square console key with an LED. */
    static void drawKey (juce::Graphics& g, juce::Rectangle<float> bounds, bool isOn, bool isDown, juce::Colour ledColour);

    /** A round LED. */
    static void drawLed (juce::Graphics& g, juce::Rectangle<float> bounds, bool isOn, juce::Colour onColour);
};

} // namespace lionvoice::gui
