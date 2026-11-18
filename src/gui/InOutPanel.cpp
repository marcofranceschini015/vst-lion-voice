// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "InOutPanel.h"
#include "Parameters.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{
namespace
{
constexpr int labelHeight = 18;
constexpr int faderWidth = 28;
constexpr int meterWidth = 20;
constexpr int faderMeterGap = 4;
} // namespace

InOutPanel::InOutPanel (juce::AudioProcessorValueTreeState& state, dsp::LevelMeter& inputMeter,
                        dsp::LevelMeter& outputMeter)
    : Panel ("IN / OUT", PanelSymbol::none, false)
    , inMeter (inputMeter, true)
    , outMeter (outputMeter, false)
{
    setUpFader (inFader);
    setUpFader (outFader);

    inAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, params::inGain, inFader);
    outAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, params::outGain, outFader);

    for (auto* fader : { &inFader, &outFader })
        fader->setDoubleClickReturnValue (true, static_cast<double> (params::gainDefaultDb));

    for (auto* component : std::initializer_list<juce::Component*> { &inFader, &outFader, &inMeter, &outMeter })
        addAndMakeVisible (*component);

    lastUpdateMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

InOutPanel::~InOutPanel()
{
    stopTimer();
}

void InOutPanel::setUpFader (juce::Slider& fader)
{
    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    fader.setSliderSnapsToMousePosition (false); // grab the cap and move it, like a real fader
}

void InOutPanel::layoutContent (juce::Rectangle<int> area)
{
    auto labels = area.removeFromBottom (labelHeight);
    area.removeFromBottom (4);

    auto inColumn = area.removeFromLeft (faderWidth + faderMeterGap + meterWidth);
    auto outColumn = area.removeFromRight (faderWidth + faderMeterGap + meterWidth);
    scaleArea = area;

    inFader.setBounds (inColumn.removeFromLeft (faderWidth).withTrimmedTop (LevelMeterDisplay::ledRowHeight));
    inColumn.removeFromLeft (faderMeterGap);
    inMeter.setBounds (inColumn);

    outFader.setBounds (outColumn.removeFromRight (faderWidth).withTrimmedTop (LevelMeterDisplay::ledRowHeight));
    outColumn.removeFromRight (faderMeterGap);
    outMeter.setBounds (outColumn);

    inLabelArea = labels.removeFromLeft (faderWidth + faderMeterGap + meterWidth);
    outLabelArea = labels.removeFromRight (faderWidth + faderMeterGap + meterWidth);

    // The popup bubbles belong to the plugin window, not to a separate desktop window.
    for (auto* fader : { &inFader, &outFader })
        fader->setPopupDisplayEnabled (true, true, getTopLevelComponent(), 1500);
}

void InOutPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    // Meter scale (dBFS), aligned with the meter slots.
    const auto meterBounds = inMeter.getBounds().toFloat();
    const float top = meterBounds.getY() + static_cast<float> (LevelMeterDisplay::ledRowHeight) + 2.0f;
    const float bottom = meterBounds.getBottom() - 2.0f;

    for (const float db : { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f })
    {
        const float y = bottom - LevelMeterDisplay::dbToProportion (db) * (bottom - top);
        steel::drawEngravedText (g, juce::String (juce::roundToInt (-db)),
                                 juce::Rectangle<float> (static_cast<float> (scaleArea.getX()), y - 6.0f,
                                                         static_cast<float> (scaleArea.getWidth()), 12.0f),
                                 juce::Justification::centred, 8.5f, true, 0.0f);
    }

    drawSectionLabel (g, "IN", inLabelArea, juce::Justification::centred);
    drawSectionLabel (g, "OUT", outLabelArea, juce::Justification::centred);
}

void InOutPanel::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double elapsed = (now - lastUpdateMs) / 1000.0;
    lastUpdateMs = now;

    inMeter.update (elapsed);
    outMeter.update (elapsed);
}

} // namespace lionvoice::gui
