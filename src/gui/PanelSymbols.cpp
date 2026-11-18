// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "PanelSymbols.h"
#include "Palette.h"
#include "SteelBackground.h"

#include <cmath>

namespace lionvoice::gui
{
namespace
{

// All paths are designed in a 1 x 1 box and then scaled into the target area.

juce::Path dipCurve()
{
    juce::Path p;
    p.startNewSubPath (0.0f, 0.35f);
    p.lineTo (0.26f, 0.35f);
    p.cubicTo (0.4f, 0.35f, 0.4f, 0.85f, 0.5f, 0.85f);
    p.cubicTo (0.6f, 0.85f, 0.6f, 0.35f, 0.74f, 0.35f);
    p.lineTo (1.0f, 0.35f);
    return p;
}

juce::Path bellCurve()
{
    juce::Path p;
    p.startNewSubPath (0.0f, 0.78f);
    p.lineTo (0.24f, 0.78f);
    p.cubicTo (0.4f, 0.78f, 0.38f, 0.18f, 0.5f, 0.18f);
    p.cubicTo (0.62f, 0.18f, 0.6f, 0.78f, 0.76f, 0.78f);
    p.lineTo (1.0f, 0.78f);
    return p;
}

juce::Path lightBulb()
{
    juce::Path p;
    p.addCentredArc (0.5f, 0.34f, 0.3f, 0.3f, 0.0f, juce::MathConstants<float>::pi * 1.2f,
                     juce::MathConstants<float>::pi * 2.8f, true);
    p.lineTo (0.62f, 0.72f);
    p.lineTo (0.38f, 0.72f);
    p.closeSubPath();
    p.startNewSubPath (0.4f, 0.84f);
    p.lineTo (0.6f, 0.84f);
    p.startNewSubPath (0.44f, 0.96f);
    p.lineTo (0.56f, 0.96f);
    return p;
}

juce::Path lightningBolt()
{
    juce::Path p;
    p.startNewSubPath (0.62f, 0.0f);
    p.lineTo (0.18f, 0.56f);
    p.lineTo (0.46f, 0.56f);
    p.lineTo (0.36f, 1.0f);
    p.lineTo (0.82f, 0.40f);
    p.lineTo (0.54f, 0.40f);
    p.closeSubPath();
    return p;
}

juce::Path softClippedSine()
{
    juce::Path p;
    constexpr int points = 48;
    constexpr float drive = 2.6f;
    const float norm = std::tanh (drive);

    for (int i = 0; i <= points; ++i)
    {
        const float x = static_cast<float> (i) / points;
        const float y = 0.5f - 0.42f * std::tanh (drive * std::sin (x * juce::MathConstants<float>::twoPi)) / norm;
        if (i == 0)
            p.startNewSubPath (x, y);
        else
            p.lineTo (x, y);
    }
    return p;
}

juce::Path downArrow()
{
    juce::Path p;
    p.startNewSubPath (0.82f, 0.12f);
    p.lineTo (0.82f, 0.86f);
    p.startNewSubPath (0.68f, 0.68f);
    p.lineTo (0.82f, 0.88f);
    p.lineTo (0.96f, 0.68f);
    return p;
}

/** Strokes (or fills) a unit-box path as an engraving: light edge underneath, dark ink on top. */
void engrave (juce::Graphics& g, juce::Path path, juce::Rectangle<float> area, bool filled)
{
    path.applyTransform (juce::AffineTransform::scale (area.getWidth(), area.getHeight()).translated (area.getPosition()));
    const juce::PathStrokeType stroke (std::max (1.3f, area.getWidth() * 0.09f), juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded);

    auto draw = [&] (juce::Colour colour, float dy)
    {
        g.setColour (colour);
        auto shifted = path;
        shifted.applyTransform (juce::AffineTransform::translation (0.0f, dy));
        if (filled)
            g.fillPath (shifted);
        else
            g.strokePath (shifted, stroke);
    };

    draw (palette::engraveHighlight, 1.0f);
    draw (palette::engraveInk, 0.0f);
}

} // namespace

void drawPanelSymbol (juce::Graphics& g, PanelSymbol symbol, juce::Rectangle<float> area)
{
    switch (symbol)
    {
        case PanelSymbol::none:
            break;
        case PanelSymbol::subtractiveEq:
            engrave (g, dipCurve(), area, false);
            break;
        case PanelSymbol::mainEq:
            engrave (g, bellCurve(), area, false);
            break;
        case PanelSymbol::leveler:
            engrave (g, lightBulb(), area, false);
            break;
        case PanelSymbol::character:
            engrave (g, lightningBolt(), area, true);
            break;
        case PanelSymbol::saturation:
            engrave (g, softClippedSine(), area, false);
            break;
        case PanelSymbol::deEsser:
            steel::drawEngravedText (g, "S", area.withTrimmedRight (area.getWidth() * 0.35f).expanded (2.0f),
                                     juce::Justification::centred, area.getHeight() * 1.15f, true, 0.0f);
            engrave (g, downArrow(), area, false);
            break;
    }
}

} // namespace lionvoice::gui
