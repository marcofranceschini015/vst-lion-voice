// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Biquad.h"
#include "Smoother.h"

#include <array>
#include <cstddef>

namespace lionvoice::dsp
{

enum class HpfSlope
{
    db12 = 0,
    db18 = 1
};

struct SubtractiveEqBand
{
    float frequency; // Hz
    float cutDb;     // 0 = no change, negative = cut
    float q;         // bandwidth: low = wide, high = narrow
};

/** All user settings of the subtractive EQ. Defaults match the plugin's parameter defaults. */
struct SubtractiveEqSettings
{
    static constexpr std::size_t numBands = 4;

    float hpfFrequency = 80.0f;
    HpfSlope hpfSlope = HpfSlope::db12;
    std::array<SubtractiveEqBand, numBands> bands { {
        { 250.0f, 0.0f, 1.4f },  // mud
        { 500.0f, 0.0f, 1.4f },  // boxiness
        { 900.0f, 0.0f, 1.4f },  // nasality
        { 3000.0f, 0.0f, 1.4f }, // harshness
    } };
};

/**
    Pre-compression clean-up EQ: a high-pass filter (12 or 18 dB/oct) followed by four bell cuts.

    Per channel: HPF 2nd-order section -> HPF 1st-order section (blended in for 18 dB/oct) -> 4 bells.

    Click-free parameter changes:
      - every value ramps over 20 ms (frequencies on a log scale, so sweeps sound even);
      - filter coefficients are recomputed every 32 samples, only while something is moving;
      - the 1st-order section always runs and is cross-blended in/out, so switching 12 <-> 18 dB/oct
        doesn't click.

    Supports up to 2 channels (mono / stereo). No memory allocation anywhere: fully real-time safe.
*/
class SubtractiveEq
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int coefficientUpdateInterval = 32;
    static constexpr double smoothingSeconds = 0.020;

    SubtractiveEq() noexcept;

    /** Sets the sample rate and settles everything on the latest settings. */
    void prepare (double newSampleRate) noexcept;

    /** New target settings; call once per block. Changes are ramped. */
    void setSettings (const SubtractiveEqSettings& settings) noexcept;

    /** Filters the channels in place. Channels beyond maxChannels are left untouched. */
    void process (float* const* channels, int numChannels, int numSamples) noexcept;

    /** Clears the filter memory and jumps to the target settings (no ramp). */
    void reset() noexcept;

    /** Total response of the EQ at a frequency, in dB, for the given settings (no audio needed). */
    static double magnitudeDb (const SubtractiveEqSettings& settings, double sampleRate, double frequency) noexcept;

private:
    struct ChannelFilters
    {
        Biquad hpfSecondOrder;
        Biquad hpfFirstOrder;
        std::array<Biquad, SubtractiveEqSettings::numBands> bands;
    };

    bool isSmoothing() const noexcept;
    void advanceSmoothers (int numSamples) noexcept;
    void updateCoefficients() noexcept;
    void processSubBlock (float* const* channels, int numChannels, int offset, int numSamples, float mixStart,
                          float mixEnd) noexcept;

    double sampleRate = 48000.0;

    LinearSmoother hpfLog2Frequency;
    LinearSmoother hpfSecondOrderQ;
    LinearSmoother firstOrderMix; // 0 = 12 dB/oct, 1 = 18 dB/oct
    std::array<LinearSmoother, SubtractiveEqSettings::numBands> bandLog2Frequency;
    std::array<LinearSmoother, SubtractiveEqSettings::numBands> bandCutDb;
    std::array<LinearSmoother, SubtractiveEqSettings::numBands> bandQ;

    std::array<ChannelFilters, maxChannels> filters;
};

} // namespace lionvoice::dsp
