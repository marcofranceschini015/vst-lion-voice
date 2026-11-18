// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_graphics/juce_graphics.h>

/** Every colour of the Lion Voice interface, in one place. */
namespace lionvoice::gui::palette
{

// Brushed steel faceplate ------------------------------------------------------
inline const juce::Colour steelBase { 0xffb4b9be };
inline const juce::Colour steelPlateTint { 0x14000000 }; // darkens the module plates slightly

// Engraving (text and lines cut into the steel) ----------------------------------
inline const juce::Colour engraveInk { 0xff26292c };
inline const juce::Colour engraveInkSoft { 0xb026292c };
inline const juce::Colour engraveHighlight { 0x8cffffff };
inline const juce::Colour engraveShadow { 0x66000000 };

// SSL-style knob caps -------------------------------------------------------------
inline const juce::Colour capWhite { 0xffe9e8e2 };
inline const juce::Colour capBrown { 0xff7b4a2b };
inline const juce::Colour capBlue { 0xff2d5ea8 };
inline const juce::Colour capGreen { 0xff2f8a4f };
inline const juce::Colour capRed { 0xffc5302a };
inline const juce::Colour capGrey { 0xff8c9196 };
inline const juce::Colour capBlack { 0xff1e2022 };
inline const juce::Colour capOrange { 0xffd8772a };

// Knob body ----------------------------------------------------------------------
inline const juce::Colour knobSkirtTop { 0xff45494d };
inline const juce::Colour knobSkirtBottom { 0xff141516 };

// Switches and LEDs ---------------------------------------------------------------
inline const juce::Colour keyTop { 0xff3a3d41 };
inline const juce::Colour keyBottom { 0xff1b1d1f };
inline const juce::Colour ledRedOn { 0xffff3b30 };
inline const juce::Colour ledGreenOn { 0xff3dff6e };
inline const juce::Colour ledAmberOn { 0xffffb020 };
inline const juce::Colour ledOff { 0xff3b2220 };

// Screws ----------------------------------------------------------------------------
inline const juce::Colour screwLight { 0xffd9dcdf };
inline const juce::Colour screwDark { 0xff6f7378 };

} // namespace lionvoice::gui::palette
