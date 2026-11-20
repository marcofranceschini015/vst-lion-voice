// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Biquad.h"
#include "GainStage.h"
#include "Smoother.h"
#include "dynamics/Ballistics.h"
#include "dynamics/GainComputer.h"

#include <array>
#include <atomic>
#include <vector>

namespace lionvoice::dsp
{

enum class FetRatio
{
    r4 = 0,
    r8,
    r12,
    r20,
    allButtons
};

struct CharacterSettings
{
    float inputDb = 0.0f;     // drive into the fixed threshold
    float outputDb = 0.0f;    // make-up
    float attackMs = 0.3f;    // 0.02 ... 0.8 ms
    float releaseMs = 150.0f; // 50 ... 1100 ms
    FetRatio ratio = FetRatio::r4;
    float mix = 1.0f;          // 0 = dry only, 1 = compressed only (parallel compression in between)
    bool sidechainHighPass = true;
};

/**
    1176 style FET compressor: fast peak control with attitude.

    No threshold knob: INPUT drives the signal into a fixed threshold (-16 dBFS), OUTPUT sets the level.
    Per sample:  INPUT -> sidechain (optional 100 Hz HPF) -> peak of the loudest channel -> dB
                 -> GainComputer (ratio/knee per button) -> Ballistics (attack/release) -> gain
    then OUTPUT, then MIX with the dry input (parallel "New York" compression).

    "ALL" emulates the famous all-buttons-in mode: ratio 20, hard knee, up to 2 dB of extra reduction just
    above the threshold (an over-compression that can make louder peaks come out slightly quieter) and 30 %
    faster timing. It is an approximation of the real circuit's behaviour, not a model of it.

    Stereo-linked, no latency, real-time safe (prepare() allocates).
*/
class CharacterCompressor
{
public:
    static constexpr int maxChannels = 2;
    static constexpr float thresholdDb = -16.0f;
    static constexpr double sidechainHighPassHz = 100.0;
    static constexpr float allButtonsExtraDb = 2.0f;
    static constexpr double allButtonsTimeScale = 0.7;

    CharacterCompressor() noexcept;

    void prepare (double sampleRate, int maxBlockSize);
    void setSettings (const CharacterSettings& settings) noexcept;
    void process (float* const* channels, int numChannels, int numSamples) noexcept;
    void reset() noexcept;

    /** Largest gain reduction of the last block, in dB (0 or negative). Safe to read from the GUI thread. */
    float getGainReductionDb() const noexcept { return gainReduction.load (std::memory_order_relaxed); }

    /** Static gain reduction (no timing) for a detector level, including the ALL-buttons behaviour. */
    static float staticGainReductionDb (FetRatio ratio, float levelDb) noexcept;

    static GainComputer curveFor (FetRatio ratio) noexcept;

private:
    void processChunk (float* const* channels, int numChannels, int offset, int numSamples) noexcept;

    double sampleRate = 48000.0;
    CharacterSettings current;

    GainStage input, output;
    std::array<Biquad, maxChannels> sidechainFilter;
    Ballistics ballistics;
    LinearSmoother mix;

    std::vector<float> dry; // dry copy for the MIX control, maxChannels x maxBlockSize
    int blockSize = 0;

    std::atomic<float> gainReduction { 0.0f };
};

} // namespace lionvoice::dsp
