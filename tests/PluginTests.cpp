// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini
//
// Tests of the whole plugin (processor + parameters + state), as a host would use it.

#include "Parameters.h"
#include "PluginProcessor.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <juce_audio_processors/juce_audio_processors.h>

#include <chrono>
#include <iostream>
#include <memory>

using Catch::Matchers::WithinAbs;

namespace
{

constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;

/** Creates and prepares the plugin like a host would. */
std::unique_ptr<LionVoiceAudioProcessor> makePlugin (int numChannels = 2)
{
    auto plugin = std::make_unique<LionVoiceAudioProcessor>();

    if (numChannels == 1)
    {
        juce::AudioProcessor::BusesLayout mono;
        mono.inputBuses.add (juce::AudioChannelSet::mono());
        mono.outputBuses.add (juce::AudioChannelSet::mono());
        REQUIRE (plugin->setBusesLayout (mono));
    }

    plugin->setRateAndBufferSizeDetails (sampleRate, blockSize);
    plugin->prepareToPlay (sampleRate, blockSize);
    return plugin;
}

void setParameter (juce::AudioProcessor& plugin, const char* id, float realValue)
{
    auto* parameter = dynamic_cast<juce::RangedAudioParameter*> (
        static_cast<LionVoiceAudioProcessor&> (plugin).getParameters().getParameter (id));
    REQUIRE (parameter != nullptr);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (realValue));
}

float getParameter (LionVoiceAudioProcessor& plugin, const char* id)
{
    return plugin.getParameters().getRawParameterValue (id)->load();
}

/** Deterministic noise, so failures are reproducible. */
juce::AudioBuffer<float> makeNoise (int numChannels, int numSamples)
{
    juce::AudioBuffer<float> buffer (numChannels, numSamples);
    juce::Random random (42);
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            buffer.setSample (ch, i, (random.nextFloat() * 2.0f - 1.0f) * 0.5f);
    return buffer;
}

/** Runs a whole buffer through the plugin in host-sized blocks. */
void processInBlocks (juce::AudioProcessor& plugin, juce::AudioBuffer<float>& buffer)
{
    juce::MidiBuffer midi;
    for (int start = 0; start < buffer.getNumSamples(); start += blockSize)
    {
        const int n = std::min (blockSize, buffer.getNumSamples() - start);
        juce::AudioBuffer<float> block (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, n);
        plugin.processBlock (block, midi);
    }
}

bool identical (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
            if (! juce::exactlyEqual (a.getSample (ch, i), b.getSample (ch, i)))
                return false;
    return true;
}

} // namespace

TEST_CASE ("Parameter IDs never change", "[plugin]")
{
    // These IDs are stored in users' projects and automation. If this test fails because an ID was
    // renamed or removed, revert the change: add new parameters instead.
    const juce::StringArray expected {
        "in_gain",     "sub_bypass", "sub_hpf_freq", "sub_hpf_slope", "sub_b1_freq", "sub_b1_cut",
        "sub_b1_q",    "sub_b2_freq", "sub_b2_cut",   "sub_b2_q",      "sub_b3_freq", "sub_b3_cut",
        "sub_b3_q",    "sub_b4_freq", "sub_b4_cut",   "sub_b4_q",      "out_gain",
        // v0.2
        "lev_bypass",  "lev_peak_red", "lev_gain",    "lev_limit",
        "fet_bypass",  "fet_input",    "fet_output",  "fet_attack",    "fet_release", "fet_ratio",
        "fet_mix",     "fet_sc_hpf",
    };

    juce::ScopedJuceInitialiser_GUI juce;
    LionVoiceAudioProcessor plugin;

    juce::StringArray actual;
    for (auto* parameter : plugin.getParameters().processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter))
            actual.add (withId->paramID);

    for (const auto& id : expected)
        CHECK (actual.contains (id));
}

TEST_CASE ("Null test: with every module bypassed and 0 dB gains the plugin is bit-transparent", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto plugin = makePlugin();

    // Bypass every module, including the ones added in future versions (all bypass IDs end in "_bypass").
    int bypassed = 0;
    for (auto* parameter : plugin->getParameters().processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*> (parameter))
            if (withId->paramID.endsWith ("_bypass"))
            {
                parameter->setValueNotifyingHost (1.0f);
                ++bypassed;
            }
    CHECK (bypassed >= 2);

    plugin->prepareToPlay (sampleRate, blockSize); // start fully bypassed (no crossfade in progress)

    auto audio = makeNoise (2, 48000);
    const auto original = audio;
    processInBlocks (*plugin, audio);

    REQUIRE (identical (audio, original));
}

TEST_CASE ("With default settings the HPF is active", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto plugin = makePlugin();

    auto audio = makeNoise (2, 48000);
    const auto original = audio;
    processInBlocks (*plugin, audio);

    CHECK_FALSE (identical (audio, original));
}

TEST_CASE ("Mono tracks are supported", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto plugin = makePlugin (1);
    setParameter (*plugin, lionvoice::params::inGain, -6.0f);
    plugin->prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> audio (1, 4096);
    audio.clear();
    for (int i = 0; i < audio.getNumSamples(); ++i)
        audio.setSample (0, i, 0.5f);

    processInBlocks (*plugin, audio);

    for (int i = 0; i < audio.getNumSamples(); ++i)
        REQUIRE (std::isfinite (audio.getSample (0, i)));
    CHECK (plugin->getInputMeter().getNumChannels() == 1);
}

TEST_CASE ("State survives save and reload", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    namespace params = lionvoice::params;

    auto original = makePlugin();
    setParameter (*original, params::inGain, -3.5f);
    setParameter (*original, params::outGain, 2.0f);
    setParameter (*original, params::subBypass, 1.0f);
    setParameter (*original, params::subHpfFreq, 120.0f);
    setParameter (*original, params::subHpfSlope, 1.0f);
    setParameter (*original, params::subBands[0].cutId, -7.5f);
    setParameter (*original, params::subBands[3].freqId, 3200.0f);
    original->getParameters().state.setProperty ("uiScale", 1.25, nullptr);

    juce::MemoryBlock saved;
    original->getStateInformation (saved);

    auto restored = makePlugin();
    restored->setStateInformation (saved.getData(), static_cast<int> (saved.getSize()));

    CHECK_THAT (getParameter (*restored, params::inGain), WithinAbs (-3.5, 1.0e-4));
    CHECK_THAT (getParameter (*restored, params::outGain), WithinAbs (2.0, 1.0e-4));
    CHECK (getParameter (*restored, params::subBypass) > 0.5f);
    CHECK_THAT (getParameter (*restored, params::subHpfFreq), WithinAbs (120.0, 0.5));
    CHECK (getParameter (*restored, params::subHpfSlope) > 0.5f);
    CHECK_THAT (getParameter (*restored, params::subBands[0].cutId), WithinAbs (-7.5, 1.0e-4));
    CHECK_THAT (getParameter (*restored, params::subBands[3].freqId), WithinAbs (3200.0, 0.5));
    CHECK_THAT (static_cast<double> (restored->getParameters().state.getProperty ("uiScale")), WithinAbs (1.25, 1.0e-6));
}

TEST_CASE ("Garbage state data is ignored", "[plugin]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto plugin = makePlugin();
    setParameter (*plugin, lionvoice::params::inGain, 4.0f);

    const char garbage[] = "this is not a Lion Voice preset";
    plugin->setStateInformation (garbage, static_cast<int> (sizeof (garbage)));

    CHECK_THAT (getParameter (*plugin, lionvoice::params::inGain), WithinAbs (4.0, 1.0e-4));
}

TEST_CASE ("CPU benchmark (run manually)", "[.][benchmark]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    auto plugin = makePlugin();
    for (const auto& band : lionvoice::params::subBands)
        setParameter (*plugin, band.cutId, -6.0f);

    auto audio = makeNoise (2, static_cast<int> (sampleRate) * 60); // one minute of stereo audio

    const auto start = std::chrono::steady_clock::now();
    processInBlocks (*plugin, audio);
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "Processed 60 s of stereo audio in " << elapsed.count() * 1000.0 << " ms ("
              << 60.0 / elapsed.count() << "x real time, " << elapsed.count() / 60.0 * 100.0
              << " % of one core)\n";
}
