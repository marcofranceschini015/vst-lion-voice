// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "dsp/LevelMeter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

namespace lionvoice::gui
{

/**
    Vertical stereo meter, -60 ... 0 dBFS:
      - bright bar  = RMS (perceived loudness),
      - faint bar   = peak, falling back at 24 dB/s,
      - white line  = peak hold (1.5 s),
      - red LED     = clip (sticky; click the meter to reset),
      - green marks = optional target zone for vocal peaks (-12 ... -10 dBFS).

    The owner calls update() from a GUI timer.
*/
class LevelMeterDisplay final : public juce::Component
{
public:
    static constexpr float floorDb = -60.0f;
    static constexpr float targetLowDb = -12.0f;
    static constexpr float targetHighDb = -10.0f;
    static constexpr int ledRowHeight = 14;

    struct ChannelState
    {
        float peakDb = floorDb;
        float holdDb = floorDb;
        float rmsDb = floorDb;
        double holdSeconds = 0.0;
        bool clip = false;
    };

    LevelMeterDisplay (dsp::LevelMeter& meterToShow, bool showTargetZone);

    /** Reads the meter and applies the ballistics; call ~30 times per second. */
    void update (double elapsedSeconds);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& event) override;

    /** Maps a level in dB to 0 (bottom, -60 dB) ... 1 (top, 0 dBFS). */
    static float dbToProportion (float db) noexcept;

    /** Draws a meter in any area (also used by the snapshot tool). */
    static void drawMeter (juce::Graphics& g, juce::Rectangle<float> area, const ChannelState* channels, int numChannels,
                           bool showTargetZone);

private:
    dsp::LevelMeter& meter;
    bool targetZone;
    int numChannels = 2;
    std::array<ChannelState, dsp::LevelMeter::maxChannels> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterDisplay)
};

} // namespace lionvoice::gui
