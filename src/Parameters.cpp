// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "Parameters.h"

#include <cmath>

namespace lionvoice::params
{
namespace
{

/** Version hint for juce::ParameterID. Bump only for parameters added in a later release. */
constexpr int firstVersion = 1;

using Attributes = juce::AudioParameterFloatAttributes;

// Value <-> text -------------------------------------------------------------

/** Decibels with one decimal, avoiding "-0.0". The unit comes from the parameter label. */
juce::String formatDb (float value, int /*maximumLength*/)
{
    return std::abs (value) < 0.05f ? juce::String ("0.0") : juce::String (value, 1);
}

/** "80 Hz", "950 Hz", "2.50 kHz". */
juce::String formatHz (float value, int /*maximumLength*/)
{
    return value < 1000.0f ? juce::String (juce::roundToInt (value)) + " Hz" : juce::String (value / 1000.0f, 2) + " kHz";
}

/** Accepts "250", "250 Hz", "2.5k", "2.5 kHz". */
float parseHz (const juce::String& text)
{
    const auto value = text.getFloatValue();
    return text.containsIgnoreCase ("k") ? value * 1000.0f : value;
}

juce::String formatQ (float value, int /*maximumLength*/)
{
    return juce::String (value, 2);
}

// Ranges ---------------------------------------------------------------------

/** Range whose middle position sits at the geometric centre, so the knob feels logarithmic. */
juce::NormalisableRange<float> logRange (float min, float max, float step)
{
    juce::NormalisableRange<float> range { min, max, step };
    range.setSkewForCentre (std::sqrt (min * max));
    return range;
}

// Factories ------------------------------------------------------------------

std::unique_ptr<juce::AudioParameterFloat> makeDb (const char* id, const juce::String& name, float min, float max,
                                                   float defaultValue)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, firstVersion }, name, juce::NormalisableRange<float> { min, max, gainStepDb },
        defaultValue, Attributes().withLabel ("dB").withStringFromValueFunction (formatDb));
}

std::unique_ptr<juce::AudioParameterFloat> makeHz (const char* id, const juce::String& name, float min, float max,
                                                   float defaultValue)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, firstVersion }, name, logRange (min, max, 1.0f), defaultValue,
        Attributes().withStringFromValueFunction (formatHz).withValueFromStringFunction (parseHz));
}

std::unique_ptr<juce::AudioParameterFloat> makeQ (const char* id, const juce::String& name)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, firstVersion }, name,
                                                        logRange (subQMin, subQMax, 0.01f), subQDefault,
                                                        Attributes().withStringFromValueFunction (formatQ));
}

std::unique_ptr<juce::AudioProcessorParameterGroup> makeSubtractiveEqGroup()
{
    auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("sub_eq", "Subtractive EQ", " | ");

    group->addChild (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { subBypass, firstVersion },
                                                                 "Sub EQ Bypass", false));
    group->addChild (makeHz (subHpfFreq, "Sub HPF Freq", subHpfMinHz, subHpfMaxHz, subHpfDefaultHz));
    group->addChild (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { subHpfSlope, firstVersion },
                                                                   "Sub HPF Slope",
                                                                   juce::StringArray { "12 dB/oct", "18 dB/oct" }, 0));

    for (const auto& band : subBands)
    {
        const juce::String prefix = juce::String ("Sub ") + band.name;
        group->addChild (makeHz (band.freqId, prefix + " Freq", band.minHz, band.maxHz, band.defaultHz));
        group->addChild (makeDb (band.cutId, prefix + " Cut", subCutMinDb, subCutMaxDb, 0.0f));
        group->addChild (makeQ (band.qId, prefix + " Q"));
    }

    return group;
}

} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeDb (inGain, "Input Gain", gainMinDb, gainMaxDb, gainDefaultDb));
    layout.add (makeSubtractiveEqGroup());
    layout.add (makeDb (outGain, "Output Gain", gainMinDb, gainMaxDb, gainDefaultDb));

    return layout;
}

} // namespace lionvoice::params
