// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace lionvoice::gui
{

/**
    The Lion Voice brand mark: a heraldic rampant lioness in the style of the lion in Brescia's coat of arms
    (the "Leonessa d'Italia"), drawn from scratch as a vector silhouette and engraved into the steel.
*/
namespace logo
{

/** The lioness outline, in a 100 x 120 box (facing left). */
juce::Path lionessPath();

/** Draws the lioness engraved into the faceplate, fitted (keeping proportions) into the area. */
void drawEngravedLioness (juce::Graphics& g, juce::Rectangle<float> area);

} // namespace logo

} // namespace lionvoice::gui
