// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

namespace lionvoice::dsp
{

/**
    Normalised coefficients of a second-order IIR filter (a0 already divided out):

        H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)

    The defaults describe a filter that passes audio unchanged.
*/
struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

/**
    One second-order filter section (a "biquad"), the building block of every EQ band in the plugin.

    Uses the Transposed Direct Form II structure, which behaves well numerically. The internal state
    is kept in double precision because low-frequency filters at high sample rates are sensitive to
    rounding errors.

    One instance processes one audio channel. Real-time safe.
*/
class Biquad
{
public:
    /** Changes the filter shape. The state is kept, so changes while playing don't click. */
    void setCoefficients (const BiquadCoefficients& newCoefficients) noexcept { coeffs = newCoefficients; }

    const BiquadCoefficients& getCoefficients() const noexcept { return coeffs; }

    /** Clears the filter memory (call when playback restarts). */
    void reset() noexcept
    {
        z1 = 0.0;
        z2 = 0.0;
    }

    float processSample (float input) noexcept
    {
        const double x = input;
        const double y = coeffs.b0 * x + z1;
        z1 = coeffs.b1 * x - coeffs.a1 * y + z2;
        z2 = coeffs.b2 * x - coeffs.a2 * y;
        return static_cast<float> (y);
    }

    /** Filters a block of samples in place. */
    void processBlock (float* samples, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            samples[i] = processSample (samples[i]);
    }

private:
    BiquadCoefficients coeffs;
    double z1 = 0.0;
    double z2 = 0.0;
};

} // namespace lionvoice::dsp
