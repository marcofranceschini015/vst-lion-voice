// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "VuMeter.h"

#include <cmath>

namespace lionvoice::gui
{
namespace
{
// Critically damped needle: 99 % of a step in ~300 ms  ->  (1 + wt) e^-wt = 0.01 at t = 0.3 s  ->  w ~ 22 rad/s.
constexpr float naturalFrequency = 22.1f;
constexpr double maxStepSeconds = 0.002; // integration step (keeps the simulation stable at any timer rate)

const juce::Colour faceTop { 0xfff3e7c4 };
const juce::Colour faceBottom { 0xffd9c690 };
const juce::Colour ink { 0xff2a2418 };
const juce::Colour red { 0xffb8261c };

constexpr float arcStart = -0.8f; // radians from 12 o'clock
constexpr float arcEnd = 0.8f;
} // namespace

VuMeter::VuMeter (juce::String captionText)
    : caption (std::move (captionText))
{
}

float VuMeter::dbToPosition (float db) noexcept
{
    // A VU needle deflects in proportion to amplitude; +3 VU is full scale.
    const float clamped = juce::jlimit (minDb, maxDb, db);
    return std::pow (10.0f, (clamped - maxDb) / 20.0f);
}

void VuMeter::setTargetDb (float db) noexcept
{
    target = dbToPosition (db);
}

void VuMeter::advance (double elapsedSeconds)
{
    const float before = position;
    const int steps = std::max (1, static_cast<int> (std::ceil (elapsedSeconds / maxStepSeconds)));
    const auto dt = static_cast<float> (elapsedSeconds / steps);

    for (int i = 0; i < steps; ++i)
    {
        // x'' = w^2 (target - x) - 2 w x'   (critically damped spring)
        const float acceleration =
            naturalFrequency * naturalFrequency * (target - position) - 2.0f * naturalFrequency * velocity;
        velocity += acceleration * dt;
        position += velocity * dt;
    }

    position = juce::jlimit (0.0f, 1.05f, position); // the pin stops the needle just past full scale

    if (std::abs (position - before) > 1.0e-4f)
        repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    drawMeter (g, getLocalBounds().toFloat(), position, caption);
}

void VuMeter::drawMeter (juce::Graphics& g, juce::Rectangle<float> area, float needlePosition,
                         const juce::String& captionText)
{
    // Bezel cut into the steel.
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (area.expanded (2.0f), 6.0f);

    const auto face = area.reduced (2.0f);

    juce::Graphics::ScopedSaveState clip (g);
    juce::Path facePath;
    facePath.addRoundedRectangle (face, 4.0f);
    g.reduceClipRegion (facePath);

    g.setGradientFill (juce::ColourGradient (faceTop, face.getCentreX(), face.getY(), faceBottom, face.getCentreX(),
                                             face.getBottom(), false));
    g.fillRoundedRectangle (face, 4.0f);

    // The needle pivots below the dial, as in real meters.
    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f);
    const float radius = face.getHeight() * 0.95f;
    auto angleFor = [] (float needle) { return arcStart + needle * (arcEnd - arcStart); };

    const float zeroAngle = angleFor (dbToPosition (0.0f));

    juce::Path scale;
    scale.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, arcStart, zeroAngle, true);
    g.setColour (ink);
    g.strokePath (scale, juce::PathStrokeType (1.5f));

    juce::Path redZone;
    redZone.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, zeroAngle, arcEnd, true);
    g.setColour (red);
    g.strokePath (redZone, juce::PathStrokeType (3.0f));

    // Ticks and numbers of the standard VU scale.
    struct Mark
    {
        float db;
        const char* label;
    };
    const Mark marks[] = { { -20.0f, "20" }, { -10.0f, "10" }, { -7.0f, "7" }, { -5.0f, "5" }, { -3.0f, "3" },
                           { -2.0f, "" },    { -1.0f, "1" },   { 0.0f, "0" },  { 1.0f, "1" },  { 2.0f, "2" },
                           { 3.0f, "3" } };

    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    for (const auto& mark : marks)
    {
        const float a = angleFor (dbToPosition (mark.db));
        const auto colour = mark.db > 0.0f ? red : ink;
        g.setColour (colour);
        g.drawLine ({ pivot.getPointOnCircumference (radius, a), pivot.getPointOnCircumference (radius + 7.0f, a) },
                    juce::exactlyEqual (mark.db, 0.0f) ? 2.0f : 1.4f);

        if (*mark.label != '\0')
            g.drawText (mark.label,
                        juce::Rectangle<float> (16.0f, 10.0f).withCentre (pivot.getPointOnCircumference (radius + 15.0f, a)),
                        juce::Justification::centred, false);
    }

    g.setColour (ink);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (captionText, face.withTrimmedTop (face.getHeight() * 0.48f).withTrimmedBottom (face.getHeight() * 0.22f),
                juce::Justification::centred, false);
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText ("GAIN REDUCTION", face.withTrimmedTop (face.getHeight() * 0.72f).withTrimmedBottom (face.getHeight() * 0.08f),
                juce::Justification::centred, false);

    // Needle.
    const float needleAngle = angleFor (juce::jlimit (0.0f, 1.05f, needlePosition));
    g.setColour (juce::Colour (0xff1a1a1a));
    g.drawLine ({ pivot.getPointOnCircumference (radius * 0.35f, needleAngle),
                  pivot.getPointOnCircumference (radius + 6.0f, needleAngle) },
                1.6f);

    // Glass reflection.
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.35f).reduced (3.0f, 2.0f), 3.0f);
}

} // namespace lionvoice::gui
