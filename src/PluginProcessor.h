// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "dsp/GainStage.h"
#include "dsp/LevelMeter.h"
#include "dsp/ModuleBypass.h"
#include "dsp/SubtractiveEq.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

/**
    The audio side of Lion Voice.

    The host (DAW) calls processBlock() on its real-time audio thread with small blocks of samples.
    Code in that path must never allocate memory, take locks or wait on anything.

    Signal chain (current stage: gain stages and subtractive EQ are active):
        IN gain -> Subtractive EQ -> Leveler -> Character comp -> Main EQ -> Saturation -> De-esser -> OUT gain
*/
class LionVoiceAudioProcessor final : public juce::AudioProcessor
{
public:
    LionVoiceAudioProcessor();
    ~LionVoiceAudioProcessor() override = default;

    // Lifecycle -------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    // Audio -----------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    using AudioProcessor::processBlock;

    // Editor ----------------------------------------------------------------
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Plugin description ----------------------------------------------------
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Programs (unused; hosts expect at least one) --------------------------
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // State (saved with the DAW project) ------------------------------------
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** All parameters. The GUI attaches its controls to this. */
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }

    /** Level meters for the GUI: IN is measured after the input gain, OUT after the output gain. */
    lionvoice::dsp::LevelMeter& getInputMeter() noexcept { return inputMeter; }
    lionvoice::dsp::LevelMeter& getOutputMeter() noexcept { return outputMeter; }

private:
    /** Reads the subtractive EQ parameters into one settings struct (audio thread, lock-free). */
    lionvoice::dsp::SubtractiveEqSettings readSubtractiveEqSettings() const noexcept;

    juce::AudioProcessorValueTreeState parameters;

    // Live parameter values, written by the host/GUI and read lock-free on the audio thread.
    std::atomic<float>* inGainDb = nullptr;
    std::atomic<float>* outGainDb = nullptr;

    std::atomic<float>* subBypass = nullptr;
    std::atomic<float>* subHpfFreq = nullptr;
    std::atomic<float>* subHpfSlope = nullptr;
    std::array<std::atomic<float>*, lionvoice::dsp::SubtractiveEqSettings::numBands> subBandFreq {};
    std::array<std::atomic<float>*, lionvoice::dsp::SubtractiveEqSettings::numBands> subBandCut {};
    std::array<std::atomic<float>*, lionvoice::dsp::SubtractiveEqSettings::numBands> subBandQ {};

    // Signal chain, in processing order.
    lionvoice::dsp::GainStage inputGain;
    lionvoice::dsp::ModuleBypass subtractiveEqBypass;
    lionvoice::dsp::SubtractiveEq subtractiveEq;
    lionvoice::dsp::GainStage outputGain;

    lionvoice::dsp::LevelMeter inputMeter;
    lionvoice::dsp::LevelMeter outputMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LionVoiceAudioProcessor)
};
