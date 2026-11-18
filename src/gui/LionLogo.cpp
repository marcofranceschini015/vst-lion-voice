// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "LionLogo.h"
#include "Palette.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace lionvoice::gui::logo
{
namespace
{

using Point = juce::Point<float>;

// --- Shape helpers (all coordinates in a 100 x 120 box, the lioness faces left) ------------------

juce::Path ellipse (float cx, float cy, float rx, float ry, float angleDegrees = 0.0f)
{
    juce::Path p;
    p.addEllipse (cx - rx, cy - ry, rx * 2.0f, ry * 2.0f);
    p.applyTransform (juce::AffineTransform::rotation (juce::degreesToRadians (angleDegrees), cx, cy));
    return p;
}

/** A smooth limb (or tail) through the given points, stroked with rounded ends. */
juce::Path limb (std::initializer_list<Point> points, float width)
{
    const std::vector<Point> pts (points);
    juce::Path centre;
    centre.startNewSubPath (pts.front());

    for (std::size_t i = 1; i + 1 < pts.size(); ++i)
    {
        const auto mid = (pts[i] + pts[i + 1]) * 0.5f;
        centre.quadraticTo (pts[i], mid);
    }
    centre.lineTo (pts.back());

    juce::Path stroked;
    juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
        .createStrokedPath (stroked, centre);
    return stroked;
}

/** A flame-like lock (mane, tail tuft): wide at the base, curving to a sharp tip. */
juce::Path flame (Point base, float length, float width, float angleDegrees, float bend)
{
    const float a = juce::degreesToRadians (angleDegrees);
    const Point dir (std::cos (a), std::sin (a));
    const Point normal (-dir.y, dir.x);
    const Point tip = base + dir * length + normal * (bend * length);
    const Point curve = normal * (bend * length * 0.6f);

    juce::Path p;
    p.startNewSubPath (base - normal * (width * 0.5f));
    p.cubicTo (base - normal * width + dir * (length * 0.45f) + curve, tip - normal * (width * 0.15f) + curve * 0.3f, tip);
    p.cubicTo (tip + normal * (width * 0.15f) + curve * 0.3f, base + normal * width + dir * (length * 0.45f) + curve,
               base + normal * (width * 0.5f));
    p.closeSubPath();
    return p;
}

/** Three hooked claws fanning out from a paw, pointing along angleDegrees. */
juce::Path claws (Point paw, float angleDegrees, float size)
{
    juce::Path p;
    for (int k = -1; k <= 1; ++k)
    {
        const float a = juce::degreesToRadians (angleDegrees + static_cast<float> (k) * 32.0f);
        const Point dir (std::cos (a), std::sin (a));
        const Point normal (-dir.y, dir.x);
        const Point root = paw + dir * (size * 0.9f);
        const Point tip = root + dir * size + normal * (size * 0.35f);

        p.startNewSubPath (root - normal * (size * 0.32f));
        p.quadraticTo (root + dir * (size * 0.7f) - normal * (size * 0.1f), tip);
        p.lineTo (root + normal * (size * 0.32f));
        p.closeSubPath();
    }
    return p;
}

/**
    Heraldic rampant lioness (in the style of Brescia's coat of arms, drawn from scratch):
    standing on the hind legs, fore paws raised, mouth open with the tongue out, flame-like mane,
    S-curved tail ending in a tuft. Built from overlapping parts that are filled one by one.
*/
std::vector<juce::Path> lionessParts()
{
    std::vector<juce::Path> parts;
    auto add = [&parts] (juce::Path p) { parts.push_back (std::move (p)); };

    // Body leaning forward: chest out, torso on the diagonal, haunch low and back.
    add (ellipse (45.0f, 50.0f, 13.0f, 14.0f));
    add (ellipse (53.0f, 68.0f, 12.0f, 24.0f, -32.0f));
    add (ellipse (63.0f, 86.0f, 14.0f, 12.5f));

    // Neck and head (facing left): skull, long muzzle, open lower jaw, small round ear.
    add (ellipse (52.0f, 37.0f, 12.0f, 12.0f));
    add (ellipse (45.0f, 24.0f, 12.0f, 10.0f, -12.0f));
    add (ellipse (33.0f, 26.5f, 8.5f, 5.5f, 6.0f));
    add (ellipse (35.5f, 34.5f, 7.5f, 2.8f, 14.0f));
    add (ellipse (51.5f, 14.5f, 3.4f, 4.2f, 20.0f));

    // Mane: long flame-like locks flowing back and down from the head along the neck.
    add (flame ({ 52.0f, 15.0f }, 16.0f, 8.0f, -35.0f, 0.3f));
    add (flame ({ 57.0f, 21.0f }, 18.0f, 9.0f, -10.0f, 0.3f));
    add (flame ({ 60.0f, 29.0f }, 18.0f, 9.0f, 12.0f, 0.32f));
    add (flame ({ 61.0f, 37.0f }, 17.0f, 9.0f, 32.0f, 0.32f));
    add (flame ({ 59.0f, 45.0f }, 15.0f, 8.0f, 55.0f, 0.3f));
    add (flame ({ 53.0f, 50.0f }, 12.0f, 7.0f, 80.0f, 0.28f));
    add (flame ({ 41.0f, 37.0f }, 9.0f, 5.0f, 115.0f, -0.3f)); // tuft under the chin

    // Fore legs: one raised high, one reaching forward.
    add (limb ({ { 43.0f, 46.0f }, { 34.0f, 42.0f }, { 26.0f, 34.0f }, { 20.0f, 27.0f } }, 7.0f));
    add (ellipse (18.0f, 25.5f, 4.5f, 3.8f, -40.0f));
    add (claws ({ 18.0f, 25.5f }, -145.0f, 3.4f));

    add (limb ({ { 42.0f, 58.0f }, { 33.0f, 58.0f }, { 26.0f, 62.0f }, { 20.0f, 63.0f } }, 6.5f));
    add (ellipse (17.5f, 63.5f, 4.2f, 3.5f, 10.0f));
    add (claws ({ 17.5f, 63.5f }, 175.0f, 3.2f));

    // Hind legs, strongly bent: one stepping forward, one pushing back.
    add (limb ({ { 57.0f, 88.0f }, { 44.0f, 95.0f }, { 50.0f, 106.0f }, { 42.0f, 113.0f } }, 8.5f));
    add (ellipse (38.5f, 113.5f, 5.5f, 3.4f));
    add (claws ({ 36.0f, 113.5f }, 172.0f, 3.2f));

    add (limb ({ { 69.0f, 90.0f }, { 80.0f, 96.0f }, { 74.0f, 106.0f }, { 80.0f, 113.0f } }, 8.0f));
    add (ellipse (83.0f, 113.5f, 5.2f, 3.3f));
    add (claws ({ 85.0f, 113.5f }, 8.0f, 3.2f));

    // Tail: rises from the haunch in an S-curve and ends in a flame tuft.
    add (limb ({ { 72.0f, 82.0f }, { 86.0f, 78.0f }, { 81.0f, 62.0f }, { 89.0f, 52.0f }, { 83.0f, 40.0f }, { 88.0f, 32.0f } },
               3.4f));
    add (flame ({ 88.0f, 34.0f }, 11.0f, 6.5f, -72.0f, -0.35f));
    add (flame ({ 88.0f, 34.0f }, 9.5f, 5.0f, -30.0f, 0.3f));
    add (flame ({ 87.0f, 35.0f }, 8.5f, 5.0f, -115.0f, 0.3f));

    return parts;
}

} // namespace

juce::Path lionessPath()
{
    juce::Path combined;
    for (const auto& part : lionessParts())
        combined.addPath (part);
    return combined;
}

void drawEngravedLioness (juce::Graphics& g, juce::Rectangle<float> area)
{
    auto parts = lionessParts();
    const auto fit = lionessPath().getTransformToScaleToFit (area, true, juce::Justification::centred);
    for (auto& part : parts)
        part.applyTransform (fit);

    // Cut into the steel: a light edge below first, then the dark recess on top.
    g.setColour (palette::engraveHighlight);
    for (const auto& part : parts)
        g.fillPath (part, juce::AffineTransform::translation (0.0f, 1.0f));

    g.setColour (palette::engraveInk);
    for (const auto& part : parts)
        g.fillPath (part);

    // Details cut back to bare steel: the eye and the open mouth.
    juce::Path eye;
    eye.addEllipse (40.0f, 21.0f, 2.8f, 1.9f);
    juce::Path mouth;
    mouth.addTriangle (24.5f, 30.5f, 38.0f, 31.0f, 25.5f, 33.0f);
    g.setColour (palette::steelBase.withAlpha (0.9f));
    g.fillPath (eye, fit);
    g.fillPath (mouth, fit);

    // The tongue, sticking out of the open mouth.
    g.setColour (palette::engraveInk);
    g.fillPath (limb ({ { 34.0f, 31.8f }, { 28.0f, 32.5f }, { 23.0f, 31.0f }, { 22.5f, 27.5f } }, 2.2f), fit);
}

} // namespace lionvoice::gui::logo
