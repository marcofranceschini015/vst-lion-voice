// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PluginProcessor.h"
#include "Parameters.h"

namespace
{
/** Root tag of the saved state. */
constexpr auto stateType = "LionVoice";

/** Saved with the state so future versions can migrate old projects. */
constexpr auto stateVersionProperty = "stateVersion";
constexpr int currentStateVersion = 1;
} // namespace

LionVoiceAudioProcessor::LionVoiceAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , parameters (*this, nullptr, stateType, lionvoice::params::createLayout())
{
    inGainDb = parameters.getRawParameterValue (lionvoice::params::inGain);
    outGainDb = parameters.getRawParameterValue (lionvoice::params::outGain);

    jassert (inGainDb != nullptr && outGainDb != nullptr);
}

void LionVoiceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Set the targets first: prepare() then jumps straight to them (no fade-in at playback start).
    inputGain.setGainDb (inGainDb->load());
    outputGain.setGainDb (outGainDb->load());

    inputGain.prepare (sampleRate, samplesPerBlock);
    outputGain.prepare (sampleRate, samplesPerBlock);
}

void LionVoiceAudioProcessor::releaseResources() {}

bool LionVoiceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Mono in -> mono out, or stereo in -> stereo out.
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void LionVoiceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Silence any output channel that has no matching input (defensive; layouts are matched above).
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    float* const* channels = buffer.getArrayOfWritePointers();
    const int numChannels = getTotalNumInputChannels();
    const int numSamples = buffer.getNumSamples();

    // Read each parameter once per block.
    inputGain.setGainDb (inGainDb->load (std::memory_order_relaxed));
    outputGain.setGainDb (outGainDb->load (std::memory_order_relaxed));

    inputGain.process (channels, numChannels, numSamples);

    // Subtractive EQ (next step) -> Leveler -> Character comp -> Main EQ -> Saturation -> De-esser

    outputGain.process (channels, numChannels, numSamples);
}

juce::AudioProcessorEditor* LionVoiceAudioProcessor::createEditor()
{
    // Temporary: JUCE's automatic slider-per-parameter window, until the SSL-style GUI replaces it.
    return new juce::GenericAudioProcessorEditor (*this);
}

void LionVoiceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty (stateVersionProperty, currentStateVersion, nullptr);

    if (const auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LionVoiceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);

    // Ignore anything that isn't a Lion Voice state (corrupt data, another plugin's chunk...).
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

// Entry point used by JUCE's plugin wrappers (VST3, AU, Standalone) to create the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LionVoiceAudioProcessor();
}
