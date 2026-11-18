// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "SubtractiveEqPanel.h"
#include "Palette.h"
#include "Parameters.h"
#include "SteelBackground.h"

#include <cmath>

namespace lionvoice::gui
{
namespace
{

/** Band cap colours, low to high frequency (SSL convention: brown, blue, green, red). */
const std::array<juce::Colour, 4> bandColours { palette::capBrown, palette::capBlue, palette::capGreen,
                                                palette::capRed };

/** "250" or "2.5" (in kHz when >= 1000 Hz), without unit. */
juce::String number (float hz)
{
    if (hz < 1000.0f)
        return juce::String (juce::roundToInt (hz));

    const float khz = hz / 1000.0f;
    const bool whole = juce::approximatelyEqual (khz, std::round (khz));
    return juce::String (khz, whole ? 0 : 1);
}

/** "200-300 HZ", "2-4 KHZ", "800 HZ-1 KHZ". */
juce::String formatRange (float minHz, float maxHz)
{
    auto unit = [] (float hz) { return hz < 1000.0f ? juce::String (" HZ") : juce::String (" KHZ"); };

    if ((minHz < 1000.0f) == (maxHz < 1000.0f))
        return number (minHz) + "-" + number (maxHz) + unit (maxHz);

    return number (minHz) + unit (minHz) + "-" + number (maxHz) + unit (maxHz);
}

} // namespace

SubtractiveEqPanel::SubtractiveEqPanel (juce::AudioProcessorValueTreeState& state)
    : Panel ("SUBTRACTIVE EQ", PanelSymbol::subtractiveEq, true)
    , hpfFrequency ("FREQ", palette::capWhite)
{
    namespace params = lionvoice::params;

    attachBypass (state, params::subBypass);

    hpfFrequency.attach (state, params::subHpfFreq);
    addAndMakeVisible (hpfFrequency);

    steepSlopeKey.setColour (juce::ToggleButton::tickColourId, palette::ledAmberOn);
    slopeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        state, params::subHpfSlope, steepSlopeKey);
    addAndMakeVisible (steepSlopeKey);

    rowLabels[0] = "HPF  " + formatRange (params::subHpfMinHz, params::subHpfMaxHz);

    for (std::size_t b = 0; b < bands.size(); ++b)
    {
        const auto& spec = params::subBands[b];
        auto& band = bands[b];

        band.frequency = std::make_unique<Knob> ("FREQ", bandColours[b]);
        band.cut = std::make_unique<Knob> ("CUT", bandColours[b]);
        band.q = std::make_unique<Knob> ("Q", palette::capGrey);

        band.frequency->attach (state, spec.freqId);
        band.cut->attach (state, spec.cutId);
        band.q->attach (state, spec.qId);

        addAndMakeVisible (*band.frequency);
        addAndMakeVisible (*band.cut);
        addAndMakeVisible (*band.q);

        rowLabels[b + 1] = juce::String (spec.name).toUpperCase() + "  " + formatRange (spec.minHz, spec.maxHz);
    }
}

SubtractiveEqPanel::~SubtractiveEqPanel() = default;

void SubtractiveEqPanel::layoutContent (juce::Rectangle<int> contentArea)
{
    const int rowHeight = contentArea.getHeight() / numRows;

    for (auto& row : rows)
        row = contentArea.removeFromTop (rowHeight);

    auto knobColumns = [] (juce::Rectangle<int> row)
    {
        row.removeFromTop (rowLabelHeight);
        const int width = row.getWidth() / 3;
        std::array<juce::Rectangle<int>, 3> columns;
        for (auto& column : columns)
            column = row.removeFromLeft (width).reduced (4, 1);
        return columns;
    };

    // HPF row: frequency knob in the first column, slope key across the other two.
    const auto hpfColumns = knobColumns (rows[0]);
    hpfFrequency.setBounds (hpfColumns[0]);
    const auto keyArea = hpfColumns[1].getUnion (hpfColumns[2]);
    steepSlopeKey.setBounds (
        keyArea.withSizeKeepingCentre (keyArea.getWidth(), 26).translated (0, -Knob::captionHeight / 2));

    for (std::size_t b = 0; b < bands.size(); ++b)
    {
        const auto columns = knobColumns (rows[b + 1]);
        bands[b].frequency->setBounds (columns[0]);
        bands[b].cut->setBounds (columns[1]);
        bands[b].q->setBounds (columns[2]);
    }
}

void SubtractiveEqPanel::paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea)
{
    juce::ignoreUnused (contentArea);

    for (std::size_t r = 0; r < rows.size(); ++r)
    {
        const auto row = rows[r];
        drawSectionLabel (g, rowLabels[r], row.withHeight (rowLabelHeight).withTrimmedLeft (2));

        if (r > 0) // groove between rows
            steel::drawEngravedLine (g, static_cast<float> (row.getY()) - 2.0f, static_cast<float> (row.getX()),
                                     static_cast<float> (row.getRight()));
    }
}

} // namespace lionvoice::gui
