// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Knob.h"
#include "Palette.h"
#include "Panel.h"

#include <array>
#include <memory>

/**
    Panels of the modules that are not implemented yet. They show the planned controls (as placeholders,
    not connected to any parameter), dimmed and disabled. Each one becomes a live panel when its DSP lands.
*/
namespace lionvoice::gui
{

/** Dynamic-EQ de-esser: FREQ, THRESHOLD, RANGE, LISTEN. */
class DeEsserPanel final : public Panel
{
public:
    DeEsserPanel();

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;

    Knob frequency { "FREQ", palette::capBlue };
    Knob threshold { "THRESHOLD", palette::capBlue };
    Knob range { "RANGE", palette::capGrey };
    juce::ToggleButton listenKey { "LISTEN" };
};

/** Saturation: still to be designed, empty for now. */
class SaturationPanel final : public Panel
{
public:
    SaturationPanel();
};

} // namespace lionvoice::gui
