// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    Horizontal LED bar showing gain reduction: 1, 2, 3, 4, 6, 8, 10, 14, 20 dB from left to right.
    Lights instantly when the reduction grows, falls back at 30 dB/s so short grabs stay readable.
    The owner calls update() from a GUI timer.
*/
class GainReductionBar final : public juce::Component
{
public:
    /** reductionDb: 0 or negative. */
    void update (float reductionDb, double elapsedSeconds);

    void paint (juce::Graphics& g) override;

    /** Draws a bar for a given reduction (also used by the snapshot tool). */
    static void drawBar (juce::Graphics& g, juce::Rectangle<float> area, float reductionDb);

private:
    float displayedDb = 0.0f;
};

} // namespace lionvoice::gui
