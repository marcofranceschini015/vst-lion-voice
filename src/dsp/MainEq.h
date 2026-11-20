// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Biquad.h"
#include "Smoother.h"

#include <array>

namespace lionvoice::dsp
{

/** All user settings of the main (additive) EQ. Defaults match the plugin's parameter defaults (all flat). */
struct MainEqSettings
{
    float bodyFrequency = 80.0f;       // 50 ... 120 Hz
    float bodyBoostDb = 0.0f;          // 0 ... +10 dB
    float bodyAttenDb = 0.0f;          // 0 ... 10 dB (amount of cut)
    float presenceFrequency = 4500.0f; // 3 ... 6 kHz
    float presenceQ = 1.0f;            // 0.5 ... 3
    float presenceBoostDb = 0.0f;      // 0 ... +10 dB
    float airFrequency = 12000.0f;     // 8 ... 16 kHz (corner of the shelf)
    float airBoostDb = 0.0f;           // 0 ... +8 dB
};

/**
    Post-compression "sweetening" EQ:

    BODY (Pultec style)  BOOST: low shelf at FREQ (Q 0.707) - fuller low end below FREQ.
                         ATTEN: broad bell cut (Q 0.8) at 2.5 x FREQ - tightens the boom/mud just above.
                         Used together they give the classic "Pultec trick" curve: deep lows up, a dip above.
                         This reproduces the audible result, not the passive circuit itself.
    PRESENCE             bell boost, 3 ... 6 kHz, adjustable Q: clarity and intelligibility.
    AIR (Baxandall style) gentle first-order high shelf, de-cramped near Nyquist: openness without harshness.

    Click-free like the subtractive EQ: 20 ms ramps (frequencies on a log scale), coefficients recomputed every
    32 samples only while something moves. Up to 2 channels, no allocation: real-time safe.
*/
class MainEq
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int coefficientUpdateInterval = 32;
    static constexpr double smoothingSeconds = 0.020;
    static constexpr float attenFrequencyRatio = 2.5f;

    MainEq() noexcept;

    void prepare (double newSampleRate) noexcept;
    void setSettings (const MainEqSettings& settings) noexcept;
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    /** Clears the filter memory and jumps to the target settings. */
    void reset() noexcept;

    /** Total response at a frequency, in dB (no audio needed). */
    static double magnitudeDb (const MainEqSettings& settings, double sampleRate, double frequency) noexcept;

private:
    enum Filter
    {
        bodyBoost,
        bodyAtten,
        presence,
        air,
        numFilters
    };

    /** The four filters' coefficients for a set of (smoothed or target) values. */
    static std::array<BiquadCoefficients, numFilters> design (double sampleRate, const MainEqSettings& s) noexcept;

    bool isSmoothing() const noexcept;
    void advanceSmoothers (int numSamples) noexcept;
    MainEqSettings currentValues() const noexcept;
    void updateCoefficients() noexcept;

    double sampleRate = 48000.0;

    // Smoothed values (frequencies as log2 so sweeps sound even).
    LinearSmoother bodyLog2Frequency, bodyBoostDb, bodyAttenDb;
    LinearSmoother presenceLog2Frequency, presenceQ, presenceBoostDb;
    LinearSmoother airLog2Frequency, airBoostDb;

    std::array<std::array<Biquad, numFilters>, maxChannels> filters;
};

} // namespace lionvoice::dsp
