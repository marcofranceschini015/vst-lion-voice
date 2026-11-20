// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PluginProcessor.h"
#include "Parameters.h"
#include "PluginEditor.h"

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
    namespace params = lionvoice::params;

    auto get = [this] (const char* id)
    {
        auto* value = parameters.getRawParameterValue (id);
        jassert (value != nullptr); // unknown parameter ID
        return value;
    };

    inGainDb = get (params::inGain);
    outGainDb = get (params::outGain);

    subBypass = get (params::subBypass);
    subHpfFreq = get (params::subHpfFreq);
    subHpfSlope = get (params::subHpfSlope);

    for (std::size_t b = 0; b < params::subBands.size(); ++b)
    {
        subBandFreq[b] = get (params::subBands[b].freqId);
        subBandCut[b] = get (params::subBands[b].cutId);
        subBandQ[b] = get (params::subBands[b].qId);
    }

    levBypass = get (params::levBypass);
    levPeakReduction = get (params::levPeakReduction);
    levGain = get (params::levGain);
    levLimit = get (params::levLimit);

    fetBypass = get (params::fetBypass);
    fetInput = get (params::fetInput);
    fetOutput = get (params::fetOutput);
    fetAttack = get (params::fetAttack);
    fetRelease = get (params::fetRelease);
    fetRatio = get (params::fetRatio);
    fetMix = get (params::fetMix);
    fetSidechainHpf = get (params::fetSidechainHpf);
}

void LionVoiceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numChannels = getTotalNumInputChannels();

    // Set the targets first: prepare() then jumps straight to them (no ramps when playback starts).
    inputGain.setGainDb (inGainDb->load());
    outputGain.setGainDb (outGainDb->load());
    subtractiveEq.setSettings (readSubtractiveEqSettings());
    subtractiveEqBypass.setBypassed (subBypass->load() >= 0.5f);
    leveler.setSettings (readLevelerSettings());
    levelerBypass.setBypassed (levBypass->load() >= 0.5f);
    character.setSettings (readCharacterSettings());
    characterBypass.setBypassed (fetBypass->load() >= 0.5f);

    inputGain.prepare (sampleRate, samplesPerBlock);
    subtractiveEqBypass.prepare (sampleRate, samplesPerBlock, numChannels);
    subtractiveEq.prepare (sampleRate);
    levelerBypass.prepare (sampleRate, samplesPerBlock, numChannels);
    leveler.prepare (sampleRate, samplesPerBlock);
    characterBypass.prepare (sampleRate, samplesPerBlock, numChannels);
    character.prepare (sampleRate, samplesPerBlock);
    outputGain.prepare (sampleRate, samplesPerBlock);

    inputMeter.prepare (sampleRate);
    outputMeter.prepare (sampleRate);
}

lionvoice::dsp::SubtractiveEqSettings LionVoiceAudioProcessor::readSubtractiveEqSettings() const noexcept
{
    constexpr auto relaxed = std::memory_order_relaxed;

    lionvoice::dsp::SubtractiveEqSettings settings;
    settings.hpfFrequency = subHpfFreq->load (relaxed);
    settings.hpfSlope = subHpfSlope->load (relaxed) >= 0.5f ? lionvoice::dsp::HpfSlope::db18
                                                            : lionvoice::dsp::HpfSlope::db12;

    for (std::size_t b = 0; b < settings.bands.size(); ++b)
        settings.bands[b] = { subBandFreq[b]->load (relaxed), subBandCut[b]->load (relaxed),
                              subBandQ[b]->load (relaxed) };

    return settings;
}

lionvoice::dsp::LevelerSettings LionVoiceAudioProcessor::readLevelerSettings() const noexcept
{
    constexpr auto relaxed = std::memory_order_relaxed;
    return { levPeakReduction->load (relaxed), levGain->load (relaxed), levLimit->load (relaxed) >= 0.5f };
}

lionvoice::dsp::CharacterSettings LionVoiceAudioProcessor::readCharacterSettings() const noexcept
{
    constexpr auto relaxed = std::memory_order_relaxed;
    const int ratioIndex = std::clamp (juce::roundToInt (fetRatio->load (relaxed)), 0, 4);

    lionvoice::dsp::CharacterSettings settings;
    settings.inputDb = fetInput->load (relaxed);
    settings.outputDb = fetOutput->load (relaxed);
    settings.attackMs = fetAttack->load (relaxed);
    settings.releaseMs = fetRelease->load (relaxed);
    settings.ratio = static_cast<lionvoice::dsp::FetRatio> (ratioIndex);
    settings.mix = fetMix->load (relaxed) / 100.0f;
    settings.sidechainHighPass = fetSidechainHpf->load (relaxed) >= 0.5f;
    return settings;
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
    constexpr auto relaxed = std::memory_order_relaxed;
    inputGain.setGainDb (inGainDb->load (relaxed));
    subtractiveEq.setSettings (readSubtractiveEqSettings());
    subtractiveEqBypass.setBypassed (subBypass->load (relaxed) >= 0.5f);
    leveler.setSettings (readLevelerSettings());
    levelerBypass.setBypassed (levBypass->load (relaxed) >= 0.5f);
    character.setSettings (readCharacterSettings());
    characterBypass.setBypassed (fetBypass->load (relaxed) >= 0.5f);
    outputGain.setGainDb (outGainDb->load (relaxed));

    // --- Chain ---------------------------------------------------------------
    inputGain.process (channels, numChannels, numSamples);
    inputMeter.process (channels, numChannels, numSamples);

    subtractiveEqBypass.process (
        channels, numChannels, numSamples,
        [this] (float* const* ch, int nc, int ns) { subtractiveEq.process (ch, nc, ns); },
        [this] { subtractiveEq.reset(); });

    levelerBypass.process (
        channels, numChannels, numSamples,
        [this] (float* const* ch, int nc, int ns) { leveler.process (ch, nc, ns); },
        [this] { leveler.reset(); });

    characterBypass.process (
        channels, numChannels, numSamples,
        [this] (float* const* ch, int nc, int ns) { character.process (ch, nc, ns); },
        [this] { character.reset(); });

    // Main EQ -> Saturation -> De-esser   (future modules)

    outputGain.process (channels, numChannels, numSamples);
    outputMeter.process (channels, numChannels, numSamples);
}

juce::AudioProcessorEditor* LionVoiceAudioProcessor::createEditor()
{
    return new LionVoiceAudioProcessorEditor (*this);
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
