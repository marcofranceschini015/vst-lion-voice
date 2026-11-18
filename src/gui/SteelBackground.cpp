// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "SteelBackground.h"
#include "Layout.h"
#include "Palette.h"

#include <cmath>
#include <vector>

namespace lionvoice::gui::steel
{
namespace
{

/**
    Generates brushed steel: every row gets its own random streak brightness, plus horizontally
    smeared fine noise (the "brush marks"), on top of a soft vertical sheen.
    Fixed random seed, so the texture is identical every time.
*/
juce::Image renderBrushedSteel (int width, int height)
{
    juce::Image image (juce::Image::RGB, width, height, false);
    juce::Image::BitmapData pixels (image, juce::Image::BitmapData::writeOnly);
    juce::Random random (0x11017e);

    constexpr int smear = 28; // length of a brush mark, in pixels
    std::vector<float> noise (static_cast<std::size_t> (width + smear), 0.0f);
    std::vector<float> streak (static_cast<std::size_t> (width), 0.0f);

    const float base = palette::steelBase.getPerceivedBrightness();

    for (int y = 0; y < height; ++y)
    {
        const float rowOffset = (random.nextFloat() - 0.5f) * 0.035f;

        for (auto& n : noise)
            n = random.nextFloat() - 0.5f;

        // Running average along the row: turns white noise into horizontal brush marks.
        float sum = 0.0f;
        for (int x = 0; x < smear; ++x)
            sum += noise[static_cast<std::size_t> (x)];
        for (int x = 0; x < width; ++x)
        {
            streak[static_cast<std::size_t> (x)] = sum / static_cast<float> (smear);
            sum += noise[static_cast<std::size_t> (x + smear)] - noise[static_cast<std::size_t> (x)];
        }

        const float fy = static_cast<float> (y) / static_cast<float> (height);
        const float sheen = 0.035f * std::cos ((fy - 0.25f) * juce::MathConstants<float>::pi);

        for (int x = 0; x < width; ++x)
        {
            const float fine = (random.nextFloat() - 0.5f) * 0.012f;
            const float v = juce::jlimit (0.0f, 1.0f,
                                          base + sheen + rowOffset + streak[static_cast<std::size_t> (x)] * 0.22f
                                              + fine);

            // Slightly cool tint, like stainless steel.
            pixels.setPixelColour (x, y, juce::Colour::fromFloatRGBA (v * 0.975f, v * 0.99f, v, 1.0f));
        }
    }

    return image;
}

/** Shared texture, rendered at 2x the design size so it stays sharp on Retina screens up to 100 %. */
const juce::Image& brushedSteelTexture()
{
    static const juce::Image texture = renderBrushedSteel (layout::baseWidth * 2, layout::baseHeight * 2);
    return texture;
}

} // namespace

void drawBrushedSteel (juce::Graphics& g, juce::Rectangle<float> area)
{
    const auto& texture = brushedSteelTexture();
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (texture, area, juce::RectanglePlacement::stretchToFit);
}

void drawEngravedPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerRadius)
{
    g.setColour (palette::steelPlateTint);
    g.fillRoundedRectangle (area, cornerRadius);

    // Light catches the lower/right lip, the upper/left lip is in shadow: reads as "cut in".
    g.setColour (palette::engraveHighlight);
    g.drawRoundedRectangle (area.translated (0.6f, 0.9f), cornerRadius, 1.2f);
    g.setColour (palette::engraveShadow);
    g.drawRoundedRectangle (area, cornerRadius, 1.2f);
}

void drawEngravedLine (juce::Graphics& g, float y, float x1, float x2)
{
    g.setColour (palette::engraveShadow);
    g.fillRect (juce::Rectangle<float> (x1, y, x2 - x1, 1.0f));
    g.setColour (palette::engraveHighlight);
    g.fillRect (juce::Rectangle<float> (x1, y + 1.0f, x2 - x1, 1.0f));
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius, float angleRadians)
{
    const auto head = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);

    // Countersink shadow.
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (head.expanded (radius * 0.18f).translated (0.0f, radius * 0.12f));

    g.setGradientFill (juce::ColourGradient (palette::screwLight, head.getTopLeft(), palette::screwDark,
                                             head.getBottomRight(), false));
    g.fillEllipse (head);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (head, radius * 0.08f);

    // Cross slot.
    const float slotLength = radius * 0.62f;
    const float slotWidth = radius * 0.26f;
    for (int i = 0; i < 2; ++i)
    {
        const float a = angleRadians + static_cast<float> (i) * juce::MathConstants<float>::halfPi;
        const juce::Line<float> slot (centre.getPointOnCircumference (slotLength, a),
                                      centre.getPointOnCircumference (slotLength, a + juce::MathConstants<float>::pi));
        // Light edge just below the slot, then the dark slot itself.
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine ({ slot.getStart().translated (0.0f, 0.6f), slot.getEnd().translated (0.0f, 0.6f) }, slotWidth);
        g.setColour (palette::engraveInk.withAlpha (0.85f));
        g.drawLine (slot, slotWidth * 0.8f);
    }
}

void drawEngravedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                       juce::Justification justification, float fontHeight, bool bold, float letterSpacing)
{
    juce::Font font (juce::FontOptions (fontHeight, bold ? juce::Font::bold : juce::Font::plain));
    font.setExtraKerningFactor (letterSpacing);
    g.setFont (font);

    g.setColour (palette::engraveHighlight);
    g.drawText (text, area.translated (0.0f, 1.0f), justification, false);
    g.setColour (palette::engraveInk);
    g.drawText (text, area, justification, false);
}

} // namespace lionvoice::gui::steel
