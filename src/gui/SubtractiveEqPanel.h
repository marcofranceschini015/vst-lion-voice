// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Knob.h"
#include "Panel.h"

#include <array>
#include <memory>

namespace lionvoice::gui
{

/**
    The live Subtractive EQ panel: high-pass filter (frequency + 18 dB/oct key) and four cut bands
    (frequency, cut, Q), stacked in rows as in the GUI sketch.
*/
class SubtractiveEqPanel final : public Panel
{
public:
    explicit SubtractiveEqPanel (juce::AudioProcessorValueTreeState& state);
    ~SubtractiveEqPanel() override;

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;

    static constexpr int numBands = 4;
    static constexpr int numRows = numBands + 1; // HPF + bands
    static constexpr int rowLabelHeight = 13;

    struct BandControls
    {
        std::unique_ptr<Knob> frequency, cut, q;
    };

    Knob hpfFrequency;
    juce::ToggleButton steepSlopeKey { "18 dB/OCT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slopeAttachment;
    std::array<BandControls, numBands> bands;

    std::array<juce::Rectangle<int>, numRows> rows;
    std::array<juce::String, numRows> rowLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SubtractiveEqPanel)
};

} // namespace lionvoice::gui
