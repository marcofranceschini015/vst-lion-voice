// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#include "InactivePanels.h"
#include "SteelBackground.h"

namespace lionvoice::gui
{
namespace
{

/** Classic VU meter face (cream dial, scale, red zone, needle at rest). */
void drawVuMeter (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Bezel cut into the steel.
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (area.expanded (2.0f), 6.0f);

    const auto face = area.reduced (2.0f);

    // Everything below stays inside the dial (the needle pivot sits below its lower edge).
    juce::Graphics::ScopedSaveState clip (g);
    juce::Path facePath;
    facePath.addRoundedRectangle (face, 4.0f);
    g.reduceClipRegion (facePath);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3e7c4), face.getCentreX(), face.getY(),
                                             juce::Colour (0xffd9c690), face.getCentreX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 4.0f);

    const auto pivot = juce::Point<float> (face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f);
    const float radius = face.getHeight() * 0.95f;
    const float start = -0.8f;
    const float end = 0.8f;
    const float redStart = 0.35f;

    juce::Path scale;
    scale.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, start, redStart, true);
    g.setColour (juce::Colour (0xff2a2418));
    g.strokePath (scale, juce::PathStrokeType (1.5f));

    juce::Path red;
    red.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, redStart, end, true);
    g.setColour (juce::Colour (0xffb8261c));
    g.strokePath (red, juce::PathStrokeType (3.0f));

    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * static_cast<float> (i) / 10.0f;
        g.setColour (a > redStart ? juce::Colour (0xffb8261c) : juce::Colour (0xff2a2418));
        g.drawLine ({ pivot.getPointOnCircumference (radius, a), pivot.getPointOnCircumference (radius + 7.0f, a) },
                    1.4f);
    }

    g.setColour (juce::Colour (0xff2a2418));
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.5f), juce::Justification::centred, false);

    // Needle resting on the left.
    g.setColour (juce::Colour (0xff1a1a1a));
    g.drawLine ({ pivot.getPointOnCircumference (radius * 0.35f, start - 0.05f),
                  pivot.getPointOnCircumference (radius + 4.0f, start - 0.05f) },
                1.6f);

    // Glass reflection.
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.35f).reduced (3.0f, 2.0f), 3.0f);
}

/** Splits an area into equal columns. */
template <std::size_t N>
std::array<juce::Rectangle<int>, N> columns (juce::Rectangle<int> area)
{
    std::array<juce::Rectangle<int>, N> result;
    const int width = area.getWidth() / static_cast<int> (N);
    for (auto& column : result)
        column = area.removeFromLeft (width);
    return result;
}

constexpr int sectionLabelHeight = 14;

} // namespace

// ---------------------------------------------------------------------------------------------
LevelerPanel::LevelerPanel()
    : Panel ("LEVELER", PanelSymbol::leveler, true)
{
    gain.setPlaceholderPosition (0.5);
    peakReduction.setPlaceholderPosition (0.35);
    addAndMakeVisible (gain);
    addAndMakeVisible (peakReduction);
    addAndMakeVisible (limitKey);
    setAvailable (false);
}

void LevelerPanel::layoutContent (juce::Rectangle<int> area)
{
    meterArea = area.removeFromLeft (230).withSizeKeepingCentre (220, 130);
    area.removeFromLeft (20);

    auto keyColumn = area.removeFromRight (90);
    limitKey.setBounds (keyColumn.withSizeKeepingCentre (80, 26));

    const auto knobs = columns<2> (area);
    gain.setBounds (knobs[0].withSizeKeepingCentre (100, 115));
    peakReduction.setBounds (knobs[1].withSizeKeepingCentre (100, 115));
}

void LevelerPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    drawVuMeter (g, meterArea.toFloat());
}

// ---------------------------------------------------------------------------------------------
CharacterPanel::CharacterPanel()
    : Panel ("CHARACTER", PanelSymbol::character, true)
{
    const char* ratios[] = { "4", "8", "12", "20" };

    for (auto* knob : { &input, &output, &attack, &release, &mix })
        addAndMakeVisible (*knob);

    input.setPlaceholderPosition (0.4);
    output.setPlaceholderPosition (0.5);
    attack.setPlaceholderPosition (0.8);
    release.setPlaceholderPosition (0.6);
    mix.setPlaceholderPosition (1.0);

    for (std::size_t i = 0; i < ratioKeys.size(); ++i)
    {
        ratioKeys[i] = std::make_unique<juce::ToggleButton> (ratios[i]);
        ratioKeys[i]->setColour (juce::ToggleButton::tickColourId, palette::ledAmberOn);
        ratioKeys[i]->setToggleState (i == 0, juce::dontSendNotification);
        addAndMakeVisible (*ratioKeys[i]);
    }

    setAvailable (false);
}

void CharacterPanel::layoutContent (juce::Rectangle<int> area)
{
    auto top = area.removeFromTop (area.getHeight() * 3 / 5);
    const auto knobs = columns<4> (top);
    input.setBounds (knobs[0].withSizeKeepingCentre (92, 105));
    output.setBounds (knobs[1].withSizeKeepingCentre (92, 105));
    attack.setBounds (knobs[2].withSizeKeepingCentre (76, 90));
    release.setBounds (knobs[3].withSizeKeepingCentre (76, 90));

    auto bottom = area;
    mix.setBounds (bottom.removeFromRight (bottom.getWidth() / 4).withSizeKeepingCentre (64, 74));

    ratioLabelArea = bottom.removeFromLeft (60);
    const auto keys = columns<4> (bottom);
    for (std::size_t i = 0; i < ratioKeys.size(); ++i)
        ratioKeys[i]->setBounds (keys[i].withSizeKeepingCentre (keys[i].getWidth() - 8, 26));
}

void CharacterPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    drawSectionLabel (g, "RATIO", ratioLabelArea);
}

// ---------------------------------------------------------------------------------------------
MainEqPanel::MainEqPanel()
    : Panel ("MAIN EQ", PanelSymbol::mainEq, true)
{
    for (auto* knob : { &bodyFreq, &bodyBoost, &bodyAtten, &presenceFreq, &presenceQ, &presenceBoost, &airBoost })
    {
        knob->setPlaceholderPosition (0.3);
        addAndMakeVisible (*knob);
    }

    setAvailable (false);
}

void MainEqPanel::layoutContent (juce::Rectangle<int> area)
{
    const int height = area.getHeight() / 3;
    for (auto& section : sections)
        section = area.removeFromTop (height);

    auto knobRow = [] (juce::Rectangle<int> section) { return section.withTrimmedTop (sectionLabelHeight).reduced (0, 4); };

    const auto body = columns<3> (knobRow (sections[0]));
    bodyFreq.setBounds (body[0].reduced (6, 0));
    bodyBoost.setBounds (body[1].reduced (6, 0));
    bodyAtten.setBounds (body[2].reduced (6, 0));

    const auto presence = columns<3> (knobRow (sections[1]));
    presenceFreq.setBounds (presence[0].reduced (6, 0));
    presenceQ.setBounds (presence[1].reduced (6, 0));
    presenceBoost.setBounds (presence[2].reduced (6, 0));

    const auto air = columns<3> (knobRow (sections[2]));
    airBoost.setBounds (air[1].reduced (6, 0));
}

void MainEqPanel::paintContent (juce::Graphics& g, juce::Rectangle<int>)
{
    const char* labels[] = { "BODY  60-100 HZ", "PRESENCE  3-6 KHZ", "AIR" };

    for (std::size_t i = 0; i < sections.size(); ++i)
    {
        drawSectionLabel (g, labels[i], sections[i].withHeight (sectionLabelHeight).withTrimmedLeft (2));
        if (i > 0)
            steel::drawEngravedLine (g, static_cast<float> (sections[i].getY()) - 2.0f,
                                     static_cast<float> (sections[i].getX()), static_cast<float> (sections[i].getRight()));
    }
}

// ---------------------------------------------------------------------------------------------
DeEsserPanel::DeEsserPanel()
    : Panel ("DE-ESSER", PanelSymbol::deEsser, true)
{
    frequency.setPlaceholderPosition (0.45);
    threshold.setPlaceholderPosition (0.6);
    range.setPlaceholderPosition (0.4);
    listenKey.setColour (juce::ToggleButton::tickColourId, palette::ledGreenOn);

    for (auto* component : std::initializer_list<juce::Component*> { &frequency, &threshold, &range, &listenKey })
        addAndMakeVisible (*component);

    setAvailable (false);
}

void DeEsserPanel::layoutContent (juce::Rectangle<int> area)
{
    frequency.setBounds (area.removeFromTop (area.getHeight() * 2 / 5).withSizeKeepingCentre (110, 125));

    auto middle = area.removeFromTop (area.getHeight() * 2 / 3);
    const auto knobs = columns<2> (middle);
    threshold.setBounds (knobs[0].withSizeKeepingCentre (80, 92));
    range.setBounds (knobs[1].withSizeKeepingCentre (80, 92));

    listenKey.setBounds (area.withSizeKeepingCentre (110, 26));
}

// ---------------------------------------------------------------------------------------------
SaturationPanel::SaturationPanel()
    : Panel ("SATURATION", PanelSymbol::saturation, true)
{
    setAvailable (false);
}

} // namespace lionvoice::gui
