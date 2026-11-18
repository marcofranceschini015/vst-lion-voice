// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "LevelMeterDisplay.h"
#include "LionLookAndFeel.h"
#include "Palette.h"
#include "dsp/Decibels.h"

namespace lionvoice::gui
{
namespace
{
constexpr float peakFallDbPerSecond = 24.0f;
constexpr double holdTimeSeconds = 1.5;
} // namespace

LevelMeterDisplay::LevelMeterDisplay (dsp::LevelMeter& meterToShow, bool showTargetZone)
    : meter (meterToShow)
    , targetZone (showTargetZone)
{
}

void LevelMeterDisplay::update (double elapsedSeconds)
{
    numChannels = std::clamp (meter.getNumChannels(), 1, dsp::LevelMeter::maxChannels);
    const auto fall = peakFallDbPerSecond * static_cast<float> (elapsedSeconds);

    for (int c = 0; c < numChannels; ++c)
    {
        auto& state = channels[static_cast<std::size_t> (c)];
        const float newPeak = std::max (floorDb, dsp::gainToDb (meter.takePeak (c)));

        state.peakDb = std::max (newPeak, state.peakDb - fall);
        state.rmsDb = std::max (floorDb, dsp::gainToDb (meter.getRms (c)));
        state.clip = meter.hasClipped (c);

        if (newPeak >= state.holdDb)
        {
            state.holdDb = newPeak;
            state.holdSeconds = 0.0;
        }
        else
        {
            state.holdSeconds += elapsedSeconds;
            if (state.holdSeconds > holdTimeSeconds)
                state.holdDb = state.peakDb;
        }
    }

    repaint();
}

void LevelMeterDisplay::paint (juce::Graphics& g)
{
    drawMeter (g, getLocalBounds().toFloat(), channels.data(), numChannels, targetZone);
}

void LevelMeterDisplay::mouseDown (const juce::MouseEvent&)
{
    meter.resetClip();
    for (auto& state : channels)
        state.clip = false;
    repaint();
}

float LevelMeterDisplay::dbToProportion (float db) noexcept
{
    return juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
}

void LevelMeterDisplay::drawMeter (juce::Graphics& g, juce::Rectangle<float> area, const ChannelState* states,
                                   int numStates, bool showTargetZone)
{
    auto ledRow = area.removeFromTop (static_cast<float> (ledRowHeight));
    const auto slot = area;

    // Dark slot cut into the steel.
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (slot.expanded (1.5f), 3.0f);
    g.setColour (juce::Colour (0xff0d0e10));
    g.fillRoundedRectangle (slot, 2.0f);

    const auto bars = slot.reduced (2.0f);
    const float gap = 2.0f;
    const float barWidth = (bars.getWidth() - gap * static_cast<float> (numStates - 1)) / static_cast<float> (numStates);

    // Colour by level: green, yellow from -18 dB, orange from -6 dB, red at 0 dBFS.
    juce::ColourGradient gradient (juce::Colour (0xff2fd25a), 0.0f, bars.getBottom(), juce::Colour (0xffff3020), 0.0f,
                                   bars.getY(), false);
    gradient.addColour (dbToProportion (-18.0f), juce::Colour (0xff9be03a));
    gradient.addColour (dbToProportion (-9.0f), juce::Colour (0xffffd23a));
    gradient.addColour (dbToProportion (-3.0f), juce::Colour (0xffff8a24));

    auto yFor = [&] (float db) { return bars.getBottom() - dbToProportion (db) * bars.getHeight(); };

    for (int c = 0; c < numStates; ++c)
    {
        const auto& state = states[c];
        const auto column = juce::Rectangle<float> (bars.getX() + static_cast<float> (c) * (barWidth + gap), bars.getY(),
                                                    barWidth, bars.getHeight());

        // Unlit segments, so the scale is visible even in silence.
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRect (column);

        g.setGradientFill (gradient);
        g.setOpacity (0.35f);
        g.fillRect (column.withTop (yFor (state.peakDb)));
        g.setOpacity (1.0f);
        g.fillRect (column.withTop (yFor (state.rmsDb)));

        if (state.holdDb > floorDb)
        {
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.fillRect (column.withTop (yFor (state.holdDb) - 1.0f).withHeight (2.0f));
        }

        // Clip LED above each channel.
        const float led = std::min (8.0f, column.getWidth() + 2.0f);
        LionLookAndFeel::drawLed (g, juce::Rectangle<float> (led, led).withCentre ({ column.getCentreX(), ledRow.getCentreY() }),
                                  state.clip, palette::ledRedOn);
    }

    // Segment lines every 3 dB.
    g.setColour (juce::Colour (0xff0d0e10).withAlpha (0.8f));
    for (float db = -3.0f; db > floorDb; db -= 3.0f)
        g.fillRect (bars.withTop (yFor (db)).withHeight (1.0f));

    if (showTargetZone)
    {
        // Small green brackets on both sides of the slot: where vocal peaks should land at the input.
        const float top = yFor (targetHighDb);
        const float bottom = yFor (targetLowDb);
        g.setColour (palette::ledGreenOn.withAlpha (0.85f));
        g.fillRect (juce::Rectangle<float> (slot.getX() - 4.0f, top, 2.5f, bottom - top));
        g.fillRect (juce::Rectangle<float> (slot.getRight() + 1.5f, top, 2.5f, bottom - top));
    }
}

} // namespace lionvoice::gui
