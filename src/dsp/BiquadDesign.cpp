// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "BiquadDesign.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace lionvoice::dsp::BiquadDesign
{
namespace
{

/** Pre-computed terms shared by all cookbook formulas. */
struct Prewarp
{
    double cosW0;
    double sinW0;
};

double clampFrequency (double sampleRate, double frequency) noexcept
{
    return std::clamp (frequency, 1.0, 0.49 * sampleRate);
}

Prewarp prewarp (double sampleRate, double frequency) noexcept
{
    const double w0 = 2.0 * std::numbers::pi * clampFrequency (sampleRate, frequency) / sampleRate;
    return { std::cos (w0), std::sin (w0) };
}

double alphaFor (const Prewarp& p, double q) noexcept
{
    return p.sinW0 / (2.0 * std::max (q, 0.01));
}

/** Divides every coefficient by a0, so the filter can use the normalised form. */
BiquadCoefficients normalise (double b0, double b1, double b2, double a0, double a1, double a2) noexcept
{
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

} // namespace

BiquadCoefficients peaking (double sampleRate, double frequency, double q, double gainDb) noexcept
{
    const auto p = prewarp (sampleRate, frequency);
    const double alpha = alphaFor (p, q);
    const double a = std::pow (10.0, gainDb / 40.0);

    return normalise (1.0 + alpha * a, -2.0 * p.cosW0, 1.0 - alpha * a,
                      1.0 + alpha / a, -2.0 * p.cosW0, 1.0 - alpha / a);
}

BiquadCoefficients lowShelf (double sampleRate, double frequency, double q, double gainDb) noexcept
{
    const auto p = prewarp (sampleRate, frequency);
    const double alpha = alphaFor (p, q);
    const double a = std::pow (10.0, gainDb / 40.0);
    const double twoSqrtAAlpha = 2.0 * std::sqrt (a) * alpha;

    return normalise (a * ((a + 1.0) - (a - 1.0) * p.cosW0 + twoSqrtAAlpha),
                      2.0 * a * ((a - 1.0) - (a + 1.0) * p.cosW0),
                      a * ((a + 1.0) - (a - 1.0) * p.cosW0 - twoSqrtAAlpha),
                      (a + 1.0) + (a - 1.0) * p.cosW0 + twoSqrtAAlpha,
                      -2.0 * ((a - 1.0) + (a + 1.0) * p.cosW0),
                      (a + 1.0) + (a - 1.0) * p.cosW0 - twoSqrtAAlpha);
}

BiquadCoefficients highShelf (double sampleRate, double frequency, double q, double gainDb) noexcept
{
    const auto p = prewarp (sampleRate, frequency);
    const double alpha = alphaFor (p, q);
    const double a = std::pow (10.0, gainDb / 40.0);
    const double twoSqrtAAlpha = 2.0 * std::sqrt (a) * alpha;

    return normalise (a * ((a + 1.0) + (a - 1.0) * p.cosW0 + twoSqrtAAlpha),
                      -2.0 * a * ((a - 1.0) + (a + 1.0) * p.cosW0),
                      a * ((a + 1.0) + (a - 1.0) * p.cosW0 - twoSqrtAAlpha),
                      (a + 1.0) - (a - 1.0) * p.cosW0 + twoSqrtAAlpha,
                      2.0 * ((a - 1.0) - (a + 1.0) * p.cosW0),
                      (a + 1.0) - (a - 1.0) * p.cosW0 - twoSqrtAAlpha);
}

BiquadCoefficients highPass (double sampleRate, double frequency, double q) noexcept
{
    const auto p = prewarp (sampleRate, frequency);
    const double alpha = alphaFor (p, q);

    return normalise ((1.0 + p.cosW0) / 2.0, -(1.0 + p.cosW0), (1.0 + p.cosW0) / 2.0,
                      1.0 + alpha, -2.0 * p.cosW0, 1.0 - alpha);
}

BiquadCoefficients lowPass (double sampleRate, double frequency, double q) noexcept
{
    const auto p = prewarp (sampleRate, frequency);
    const double alpha = alphaFor (p, q);

    return normalise ((1.0 - p.cosW0) / 2.0, 1.0 - p.cosW0, (1.0 - p.cosW0) / 2.0,
                      1.0 + alpha, -2.0 * p.cosW0, 1.0 - alpha);
}

BiquadCoefficients highPass1stOrder (double sampleRate, double frequency) noexcept
{
    // Bilinear transform of H(s) = s / (s + w), pre-warped: k = tan(w0 / 2).
    const double k = std::tan (std::numbers::pi * clampFrequency (sampleRate, frequency) / sampleRate);

    return normalise (1.0, -1.0, 0.0, 1.0 + k, k - 1.0, 0.0);
}

namespace
{
/** |H(j 2 pi f)|^2 of H(s) = (G s + sqrt(G) w) / (s + sqrt(G) w): 1 at DC, G at high frequencies, sqrt(G) at w. */
double analogShelfPowerGain (double frequency, double cornerFrequency, double linearGain) noexcept
{
    const double ratio2 = (frequency / cornerFrequency) * (frequency / cornerFrequency);
    return (linearGain * linearGain * ratio2 + linearGain) / (ratio2 + linearGain);
}
} // namespace

double analogHighShelf1stOrderDb (double frequency, double cornerFrequency, double gainDb) noexcept
{
    const double g = std::pow (10.0, gainDb / 20.0);
    return 10.0 * std::log10 (analogShelfPowerGain (frequency, cornerFrequency, g));
}

BiquadCoefficients highShelf1stOrderMatched (double sampleRate, double frequency, double gainDb) noexcept
{
    const double fc = clampFrequency (sampleRate, frequency);
    const double g = std::pow (10.0, gainDb / 20.0);

    // Target power gains of the analog shelf at DC, at the corner and at Nyquist.
    const double atDc = 1.0;
    const double atCorner = analogShelfPowerGain (fc, fc, g);
    const double atNyquist = analogShelfPowerGain (sampleRate * 0.5, fc, g);
    const double c = std::cos (2.0 * std::numbers::pi * fc / sampleRate);

    // For H(z) = (b0 + b1 z^-1) / (1 + a1 z^-1), matching |H|^2 at DC and Nyquist fixes the numerator; matching
    // it at the corner as well gives  K a1^2 + 2 M a1 + K = 0  for the pole. Its two roots multiply to 1:
    // the one inside the unit circle is the stable filter.
    const double u = atDc * (1.0 + c) * 0.5;
    const double v = atNyquist * (1.0 - c) * 0.5;
    const double k = u + v - atCorner;
    const double m = u - v - atCorner * c;

    double a1 = 0.0;
    if (std::abs (k) > 1.0e-12)
    {
        const double root = std::sqrt (std::max (0.0, m * m - k * k));
        const double first = (-m + root) / k;
        const double second = (-m - root) / k;
        a1 = std::abs (first) < 1.0 ? first : second;
    }

    // Numerator from the DC and Nyquist gains (minimum phase): b0 + b1 = sqrt(P_dc)(1 + a1), b0 - b1 = sqrt(P_ny)(1 - a1).
    const double sum = std::sqrt (atDc) * (1.0 + a1);
    const double difference = std::sqrt (atNyquist) * (1.0 - a1);

    return { 0.5 * (sum + difference), 0.5 * (sum - difference), 0.0, a1, 0.0 };
}

double magnitudeDb (const BiquadCoefficients& c, double sampleRate, double frequency) noexcept
{
    // Evaluate H(z) on the unit circle: z^-1 = e^(-j w).
    const double w = 2.0 * std::numbers::pi * frequency / sampleRate;
    const std::complex<double> z1 = std::polar (1.0, -w);
    const std::complex<double> z2 = z1 * z1;

    const auto numerator = c.b0 + c.b1 * z1 + c.b2 * z2;
    const auto denominator = 1.0 + c.a1 * z1 + c.a2 * z2;
    const double magnitude = std::abs (numerator / denominator);

    return magnitude > 1.0e-10 ? 20.0 * std::log10 (magnitude) : -200.0;
}

} // namespace lionvoice::dsp::BiquadDesign
