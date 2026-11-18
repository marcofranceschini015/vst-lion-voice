// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "LionLookAndFeel.h"
#include "Palette.h"
#include "SteelBackground.h"


namespace lionvoice::gui
{

LionLookAndFeel::LionLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId, palette::capGrey);
    setColour (juce::ToggleButton::tickColourId, palette::ledRedOn);
    setColour (juce::ToggleButton::textColourId, palette::engraveInk);

    // Value bubble shown while turning a knob.
    setColour (juce::BubbleComponent::backgroundColourId, palette::keyBottom.withAlpha (0.92f));
    setColour (juce::BubbleComponent::outlineColourId, juce::Colours::black);
    setColour (juce::TooltipWindow::textColourId, juce::Colours::white);
}

void LionLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    drawKnob (g, juce::Rectangle<int> (x, y, width, height).toFloat(), sliderPos, rotaryStartAngle, rotaryEndAngle,
              slider.findColour (juce::Slider::rotarySliderFillColourId));
}

void LionLookAndFeel::drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion, float startAngle,
                                float endAngle, juce::Colour capColour)
{
    const float size = std::min (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float outer = size * 0.5f;
    const float knobRadius = outer * 0.74f;
    const float capRadius = knobRadius * 0.7f;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    // Printed scale on the faceplate: 11 ticks, longer at the ends and in the middle.
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + static_cast<float> (i) / 10.0f * (endAngle - startAngle);
        const bool major = i == 0 || i == 5 || i == 10;
        const float r0 = outer * (major ? 0.82f : 0.86f);
        const juce::Line<float> tick (centre.getPointOnCircumference (r0, a),
                                      centre.getPointOnCircumference (outer * 0.98f, a));
        // Engraved tick: light edge slightly below, dark ink on top.
        g.setColour (palette::engraveHighlight);
        g.drawLine ({ tick.getStart().translated (0.0f, 0.6f), tick.getEnd().translated (0.0f, 0.6f) },
                    major ? 1.6f : 1.1f);
        g.setColour (palette::engraveInkSoft);
        g.drawLine (tick, major ? 1.4f : 1.0f);
    }

    const auto knob = juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre);

    // Shadow cast on the steel.
    juce::Path knobPath;
    knobPath.addEllipse (knob);
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), juce::roundToInt (knobRadius * 0.35f),
                      { 0, juce::roundToInt (knobRadius * 0.12f) })
        .drawForPath (g, knobPath);

    // Dark skirt with knurling.
    g.setGradientFill (juce::ColourGradient (palette::knobSkirtTop, knob.getCentreX(), knob.getY(),
                                             palette::knobSkirtBottom, knob.getCentreX(), knob.getBottom(), false));
    g.fillEllipse (knob);

    constexpr int ridges = 36;
    for (int i = 0; i < ridges; ++i)
    {
        const float a = static_cast<float> (i) / static_cast<float> (ridges) * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawLine ({ centre.getPointOnCircumference (knobRadius * 0.8f, a),
                      centre.getPointOnCircumference (knobRadius * 0.98f, a) },
                    std::max (0.8f, knobRadius * 0.035f));
    }

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (knob, 1.0f);

    // Coloured cap, lit from the top-left.
    const auto cap = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (capColour.brighter (0.35f), cap.getX(), cap.getY(),
                                             capColour.darker (0.35f), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawEllipse (cap.reduced (0.5f), 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (cap, 0.8f);

    // Pointer line: white on dark caps, black on the white cap.
    const auto pointerColour = capColour.getPerceivedBrightness() > 0.7f ? palette::capBlack : juce::Colours::white;
    g.setColour (pointerColour);
    g.drawLine ({ centre.getPointOnCircumference (capRadius * 0.2f, angle),
                  centre.getPointOnCircumference (knobRadius * 0.93f, angle) },
                std::max (1.5f, knobRadius * 0.1f));
}

void LionLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    auto bounds = button.getLocalBounds().toFloat();
    const float keySize = std::min (bounds.getHeight(), 26.0f);
    const auto key = bounds.removeFromRight (keySize).withSizeKeepingCentre (keySize, keySize);

    if (button.getButtonText().isNotEmpty())
        steel::drawEngravedText (g, button.getButtonText(), bounds.withTrimmedRight (6.0f),
                                 juce::Justification::centredLeft, std::min (13.0f, bounds.getHeight() * 0.55f));

    drawKey (g, key, button.getToggleState(), shouldDrawButtonAsDown,
             button.findColour (juce::ToggleButton::tickColourId));
}

void LionLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
                                        juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    drawFader (g, juce::Rectangle<int> (x, y, width, height).toFloat(), sliderPos);
}

int LionLookAndFeel::getSliderThumbRadius (juce::Slider& slider)
{
    // Keeps the fader cap fully inside the component at both ends of its travel.
    if (slider.getSliderStyle() == juce::Slider::LinearVertical)
        return static_cast<int> (faderCapHeight * 0.5f);

    return LookAndFeel_V4::getSliderThumbRadius (slider);
}

void LionLookAndFeel::drawFader (juce::Graphics& g, juce::Rectangle<float> track, float capCentreY)
{
    const float centreX = track.getCentreX();

    // Scale: a tick every 1/8 of the travel (6 dB on a -24..+24 range), longer at the centre (0 dB).
    for (int i = 0; i <= 8; ++i)
    {
        const float ty = track.getBottom() - track.getHeight() * static_cast<float> (i) / 8.0f;
        const float length = i == 4 ? track.getWidth() * 0.5f : track.getWidth() * 0.32f;
        g.setColour (palette::engraveHighlight);
        g.fillRect (juce::Rectangle<float> (centreX - length, ty + 0.6f, length * 2.0f, 1.0f));
        g.setColour (palette::engraveInkSoft);
        g.fillRect (juce::Rectangle<float> (centreX - length, ty - 0.4f, length * 2.0f, 1.0f));
    }

    // Slot cut into the steel.
    const auto slot = juce::Rectangle<float> (5.0f, track.getHeight() + 8.0f).withCentre (track.getCentre());
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (slot, 2.5f);
    g.setColour (palette::engraveHighlight);
    g.drawRoundedRectangle (slot.translated (0.0f, 0.8f), 2.5f, 0.8f);

    // Cap.
    const float capWidth = std::min (track.getWidth() + 4.0f, 26.0f);
    const auto cap = juce::Rectangle<float> (capWidth, faderCapHeight).withCentre ({ centreX, capCentreY });

    juce::Path capPath;
    capPath.addRoundedRectangle (cap, 3.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 6, { 0, 3 }).drawForPath (g, capPath);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffdfe2e5), cap.getX(), cap.getY(), juce::Colour (0xff7f8489),
                                             cap.getX(), cap.getBottom(), false));
    g.fillPath (capPath);

    // Grip ridges and the centre line.
    for (int i = -2; i <= 2; ++i)
    {
        if (i == 0)
            continue;
        const float ry = cap.getCentreY() + static_cast<float> (i) * cap.getHeight() * 0.16f;
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRect (juce::Rectangle<float> (cap.getX() + 3.0f, ry, cap.getWidth() - 6.0f, 1.0f));
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.fillRect (juce::Rectangle<float> (cap.getX() + 3.0f, ry + 1.0f, cap.getWidth() - 6.0f, 1.0f));
    }

    g.setColour (palette::capBlack);
    g.fillRect (juce::Rectangle<float> (cap.getX() + 2.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 4.0f, 2.0f));

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (cap, 3.0f, 0.8f);
}

void LionLookAndFeel::drawKey (juce::Graphics& g, juce::Rectangle<float> bounds, bool isOn, bool isDown,
                               juce::Colour ledColour)
{
    const float corner = bounds.getWidth() * 0.12f;

    // Cut-out in the steel.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (bounds.expanded (1.5f), corner + 1.5f);

    auto key = bounds.reduced (1.0f);
    if (isDown)
        key = key.translated (0.0f, 1.0f);

    g.setGradientFill (juce::ColourGradient (palette::keyTop, key.getX(), key.getY(), palette::keyBottom, key.getX(),
                                             key.getBottom(), false));
    g.fillRoundedRectangle (key, corner);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (key.reduced (0.5f), corner, 1.0f);

    const float ledSize = key.getWidth() * 0.36f;
    drawLed (g, juce::Rectangle<float> (ledSize, ledSize).withCentre (key.getCentre()), isOn, ledColour);
}

void LionLookAndFeel::drawLed (juce::Graphics& g, juce::Rectangle<float> bounds, bool isOn, juce::Colour onColour)
{
    if (isOn)
    {
        // Glow around the lit LED.
        g.setGradientFill (juce::ColourGradient (onColour.withAlpha (0.55f), bounds.getCentre(),
                                                 onColour.withAlpha (0.0f),
                                                 bounds.getCentre().translated (bounds.getWidth() * 1.4f, 0.0f), true));
        g.fillEllipse (bounds.expanded (bounds.getWidth() * 1.4f));
    }

    const auto body = isOn ? onColour : palette::ledOff;
    g.setGradientFill (juce::ColourGradient (body.brighter (isOn ? 0.6f : 0.2f), bounds.getX(), bounds.getY(),
                                             body.darker (0.4f), bounds.getRight(), bounds.getBottom(), false));
    g.fillEllipse (bounds);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (bounds, 0.8f);
}

} // namespace lionvoice::gui
