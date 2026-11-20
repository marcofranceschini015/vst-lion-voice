// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "CharacterPanel.h"
#include "Parameters.h"

namespace lionvoice::gui
{
namespace
{
constexpr int keyRowHeight = 34;
constexpr int barRowHeight = 30;
} // namespace

CharacterPanel::CharacterPanel (juce::AudioProcessorValueTreeState& state,
                                const dsp::CharacterCompressor& compressorToShow)
    : Panel ("CHARACTER", PanelSymbol::character, true)
    , compressor (compressorToShow)
{
    attachBypass (state, params::fetBypass);

    input.attach (state, params::fetInput);
    output.attach (state, params::fetOutput);
    attack.attach (state, params::fetAttack);
    release.attach (state, params::fetRelease);
    mix.attach (state, params::fetMix);

    for (auto* knob : { &input, &output, &attack, &release, &mix })
        addAndMakeVisible (*knob);

    std::vector<juce::Button*> keys;
    for (std::size_t i = 0; i < ratioKeys.size(); ++i)
    {
        // "4:1" -> "4" on the key; "ALL" stays.
        ratioKeys[i] = std::make_unique<juce::ToggleButton> (params::fetRatioNames[static_cast<int> (i)].upToFirstOccurrenceOf (":", false, false));
        ratioKeys[i]->setColour (juce::ToggleButton::tickColourId, palette::ledAmberOn);
        addAndMakeVisible (*ratioKeys[i]);
        keys.push_back (ratioKeys[i].get());
    }

    if (auto* ratio = state.getParameter (params::fetRatio))
        ratioAttachment = std::make_unique<RadioKeysAttachment> (*ratio, keys);

    sidechainKey.setColour (juce::ToggleButton::tickColourId, palette::ledGreenOn);
    sidechainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        state, params::fetSidechainHpf, sidechainKey);
    addAndMakeVisible (sidechainKey);

    addAndMakeVisible (reductionBar);

    lastUpdateMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

CharacterPanel::~CharacterPanel()
{
    stopTimer();
    ratioAttachment.reset(); // detach before the keys are destroyed
}

void CharacterPanel::layoutContent (juce::Rectangle<int> area)
{
    auto barRow = area.removeFromBottom (barRowHeight);
    area.removeFromBottom (4);
    auto keyRow = area.removeFromBottom (keyRowHeight);
    area.removeFromBottom (4);

    // Knobs: INPUT, OUTPUT, ATTACK, RELEASE, MIX.
    const int column = area.getWidth() / 5;
    Knob* knobs[] = { &input, &output, &attack, &release, &mix };
    for (auto* knob : knobs)
    {
        const bool big = knob == &input || knob == &output;
        knob->setBounds (area.removeFromLeft (column).withSizeKeepingCentre (big ? 84 : 72, big ? 100 : 88));
    }

    // RATIO keys across the whole row.
    ratioLabelArea = keyRow.removeFromLeft (52);
    const int keyWidth = keyRow.getWidth() / static_cast<int> (ratioKeys.size());
    for (auto& key : ratioKeys)
    {
        // Just wide enough for the label plus the key, so each number clearly belongs to its own key.
        const int labelWidth = 11 * key->getButtonText().length();
        key->setBounds (keyRow.removeFromLeft (keyWidth).withSizeKeepingCentre (labelWidth + 6 + 26, 26));
    }

    // Gain-reduction bar, with the sidechain key on its right.
    reductionLabelArea = barRow.removeFromLeft (52);
    sidechainKey.setBounds (barRow.removeFromRight (104).withSizeKeepingCentre (100, 26).translated (0, -5));
    barRow.removeFromRight (12);
    reductionBar.setBounds (barRow);
}

void CharacterPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    drawSectionLabel (g, "RATIO", ratioLabelArea);
    drawSectionLabel (g, "GR", reductionLabelArea.withTrimmedBottom (10));
}

void CharacterPanel::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double elapsed = (now - lastUpdateMs) / 1000.0;
    lastUpdateMs = now;

    reductionBar.update (compressor.getGainReductionDb(), std::min (elapsed, 0.1));
}

} // namespace lionvoice::gui
