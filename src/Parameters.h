// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
    Every automatable parameter of Lion Voice.

    The IDs are what DAWs store in projects and automation. Once released they must NEVER be renamed
    or removed; new parameters get new IDs.
*/
namespace lionvoice::params
{

// IDs -----------------------------------------------------------------------
inline constexpr auto inGain = "in_gain";
inline constexpr auto outGain = "out_gain";

// Ranges --------------------------------------------------------------------
inline constexpr float gainMinDb = -24.0f;
inline constexpr float gainMaxDb = 24.0f;
inline constexpr float gainStepDb = 0.1f;
inline constexpr float gainDefaultDb = 0.0f;

/** Builds the full parameter list, used to construct the AudioProcessorValueTreeState. */
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

} // namespace lionvoice::params
