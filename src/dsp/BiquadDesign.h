// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Biquad.h"

/**
    Coefficient recipes for the filters used in Lion Voice, based on Robert Bristow-Johnson's
    "Audio EQ Cookbook" (bilinear transform with frequency pre-warping, so the centre/cutoff
    frequency lands exactly where requested).

    All functions are pure (no state), real-time safe, and clamp their inputs to a usable range:
    frequency to [1 Hz, 0.49 * sampleRate], Q to at least 0.01.
*/
namespace lionvoice::dsp::BiquadDesign
{

/** Q of a 12 dB/oct Butterworth high-pass (maximally flat, -3 dB at the cutoff). */
inline constexpr double butterworthQ12 = 0.70710678118654752;

/**
    Q of the second-order section of an 18 dB/oct (third-order) Butterworth high-pass.
    Combined with highPass1stOrder() at the same frequency it gives -3 dB at the cutoff and no bump.
*/
inline constexpr double butterworthQ18 = 1.0;

/** Bell boost or cut around a centre frequency. gainDb = 0 leaves the signal unchanged. */
BiquadCoefficients peaking (double sampleRate, double frequency, double q, double gainDb) noexcept;

/** Boosts or cuts everything below the corner frequency. */
BiquadCoefficients lowShelf (double sampleRate, double frequency, double q, double gainDb) noexcept;

/** Boosts or cuts everything above the corner frequency. */
BiquadCoefficients highShelf (double sampleRate, double frequency, double q, double gainDb) noexcept;

/** Second-order (12 dB/oct) high-pass. */
BiquadCoefficients highPass (double sampleRate, double frequency, double q) noexcept;

/** Second-order (12 dB/oct) low-pass. */
BiquadCoefficients lowPass (double sampleRate, double frequency, double q) noexcept;

/** First-order (6 dB/oct) high-pass. Only b0, b1 and a1 are used. */
BiquadCoefficients highPass1stOrder (double sampleRate, double frequency) noexcept;

/**
    Exact gain of the filter at a frequency, in dB, computed from the coefficients (no audio needed).
    Used by the tests and, later, to draw EQ curves.
*/
double magnitudeDb (const BiquadCoefficients& coefficients, double sampleRate, double frequency) noexcept;

} // namespace lionvoice::dsp::BiquadDesign
