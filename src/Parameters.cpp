// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "Parameters.h"

namespace lionvoice::params
{
namespace
{

/** Version hint for juce::ParameterID. Bump only for parameters added in a later release. */
constexpr int firstVersion = 1;

/** Shows decibel values with one decimal, avoiding "-0.0". */
juce::String formatDb (float value, int /*maximumLength*/)
{
    return std::abs (value) < 0.05f ? juce::String ("0.0") : juce::String (value, 1);
}

std::unique_ptr<juce::AudioParameterFloat> makeGainParameter (const char* id, const char* name)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, firstVersion },
        name,
        juce::NormalisableRange<float> { gainMinDb, gainMaxDb, gainStepDb },
        gainDefaultDb,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (formatDb));
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeGainParameter (inGain, "Input Gain"));
    layout.add (makeGainParameter (outGain, "Output Gain"));

    return layout;
}

} // namespace lionvoice::params
