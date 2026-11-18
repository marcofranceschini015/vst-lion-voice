// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_graphics/juce_graphics.h>

/** Drawing helpers for the brushed-steel faceplate. Message thread only. */
namespace lionvoice::gui::steel
{

/** Fills the area with brushed steel. The texture is generated once (procedurally) and then cached. */
void drawBrushedSteel (juce::Graphics& g, juce::Rectangle<float> area);

/** A plate recessed into the faceplate: slightly darker, with an engraved (cut-in) edge. */
void drawEngravedPlate (juce::Graphics& g, juce::Rectangle<float> area, float cornerRadius = 5.0f);

/** A thin horizontal groove cut into the steel (used to separate sections inside a plate). */
void drawEngravedLine (juce::Graphics& g, float y, float x1, float x2);

/** A steel cross-head screw. */
void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius, float angleRadians);

/** Text that looks cut into the metal: dark ink with a thin light edge underneath. */
void drawEngravedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                       juce::Justification justification, float fontHeight, bool bold = true,
                       float letterSpacing = 0.08f);

} // namespace lionvoice::gui::steel
