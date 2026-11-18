// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PluginProcessor.h"
#include "PluginEditor.h"

LionVoiceAudioProcessor::LionVoiceAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void LionVoiceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Called before playback starts: later steps allocate and reset DSP state here.
    juce::ignoreUnused (sampleRate, samplesPerBlock);
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

    // Passthrough: the buffer already contains the input, so leaving it untouched outputs it unchanged.
}

juce::AudioProcessorEditor* LionVoiceAudioProcessor::createEditor()
{
    return new LionVoiceAudioProcessorEditor (*this);
}

void LionVoiceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // No parameters yet. Step 6 stores the parameter tree here.
    juce::ignoreUnused (destData);
}

void LionVoiceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

// Entry point used by JUCE's plugin wrappers (VST3, AU, Standalone) to create the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LionVoiceAudioProcessor();
}
