// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_graphics/juce_graphics.h>

namespace lionvoice::gui
{

/** The small engraved icon in the top-right corner of each module panel. */
enum class PanelSymbol
{
    none,
    subtractiveEq, // a dip in a flat line: cutting
    leveler,       // light bulb: optical (LA-2A style) compressor
    character,     // lightning bolt: fast FET compressor
    mainEq,        // a bell rising from a flat line: boosting
    saturation,    // soft-clipped sine wave
    deEsser        // an "S" with a downward arrow
};

/** Draws the symbol engraved into the steel, fitted inside the area. */
void drawPanelSymbol (juce::Graphics& g, PanelSymbol symbol, juce::Rectangle<float> area);

} // namespace lionvoice::gui
