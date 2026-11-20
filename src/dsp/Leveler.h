// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Biquad.h"
#include "GainStage.h"
#include "Smoother.h"
#include "dynamics/GainComputer.h"
#include "dynamics/OptoCell.h"

#include <array>
#include <atomic>

namespace lionvoice::dsp
{

struct LevelerSettings
{
    float peakReduction = 25.0f; // 0 ... 100
    float gainDb = 0.0f;         // make-up, 0 ... +24 dB
    bool limit = false;          // false = COMP (3:1, very soft), true = LIMIT (10:1)
};

/**
    LA-2A style leveling amplifier: smooth, program-dependent optical compression.

    Per sample:
        sidechain (HPF 80 Hz + gentle HF emphasis) -> power of the loudest channel, averaged over ~10 ms
        -> dB -> GainComputer (threshold set by PEAK REDUCTION) -> OptoCell timing -> gain on all channels
    then the GAIN stage (make-up). Stereo-linked, no latency, real-time safe (prepare() allocates).
*/
class Leveler
{
public:
    static constexpr int maxChannels = 2;
    static constexpr double detectorSeconds = 0.010;
    static constexpr double thresholdSmoothingSeconds = 0.020;

    Leveler() noexcept;

    void prepare (double sampleRate, int maxBlockSize);
    void setSettings (const LevelerSettings& settings) noexcept;
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    /** Clears filters, detector and opto cell (no compression in progress). */
    void reset() noexcept;

    /** Largest gain reduction of the last block, in dB (0 or negative). Safe to read from the GUI thread. */
    float getGainReductionDb() const noexcept { return gainReduction.load (std::memory_order_relaxed); }

    /** Threshold (dB, RMS) used for a PEAK REDUCTION setting: 0 -> +10 dB (never compresses), 100 -> -40 dB. */
    static float thresholdForPeakReduction (float peakReduction) noexcept;

    /** Static curve for the COMP / LIMIT modes. */
    static GainComputer curveFor (bool limit, float thresholdDb) noexcept;

private:
    double sampleRate = 48000.0;

    std::array<Biquad, maxChannels> sidechainHighPass;
    std::array<Biquad, maxChannels> sidechainEmphasis;
    float detectorCoefficient = 0.0f;
    float detectorPower = 0.0f;

    LinearSmoother threshold;
    bool limitMode = false;
    OptoCell cell;
    GainStage makeup;

    std::atomic<float> gainReduction { 0.0f };
};

} // namespace lionvoice::dsp
