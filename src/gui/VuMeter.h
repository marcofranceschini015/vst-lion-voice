// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lionvoice::gui
{

/**
    Analog-style VU meter: cream dial, standard VU scale (-20 ... +3), red zone above 0 VU.

    The needle has real VU ballistics: a critically damped mechanical movement that reaches 99 % of a step in
    about 300 ms, so it moves smoothly instead of jumping. The owner sets the target and calls advance() from a
    GUI timer.

    Used in gain-reduction mode on the Leveler: the needle rests on 0 and falls to the left by the number of
    dB being taken off (like the LA-2A's meter in GAIN REDUCTION mode).
*/
class VuMeter final : public juce::Component
{
public:
    static constexpr float minDb = -20.0f;
    static constexpr float maxDb = 3.0f;

    explicit VuMeter (juce::String captionText = "VU");

    /** Where the needle should go, in VU dB (clamped to the scale). */
    void setTargetDb (float db) noexcept;

    /** Moves the needle by the elapsed time; repaints when it moved. */
    void advance (double elapsedSeconds);

    void paint (juce::Graphics& g) override;

    /** Needle position 0 (far left) ... 1 (far right) for a level: proportional to amplitude, like a real VU. */
    static float dbToPosition (float db) noexcept;

    /** Draws a complete meter with the needle at a position (also used by the snapshot tool). */
    static void drawMeter (juce::Graphics& g, juce::Rectangle<float> area, float needlePosition,
                           const juce::String& caption);

private:
    juce::String caption;
    float target = dbToPosition (0.0f);
    float position = dbToPosition (0.0f);
    float velocity = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VuMeter)
};

} // namespace lionvoice::gui
