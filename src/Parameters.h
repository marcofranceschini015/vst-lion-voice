// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

/**
    Every automatable parameter of Lion Voice.

    The IDs are what DAWs store in projects and automation. Once released they must NEVER be renamed
    or removed; new parameters get new IDs.
*/
namespace lionvoice::params
{

// IN / OUT gain -------------------------------------------------------------
inline constexpr auto inGain = "in_gain";
inline constexpr auto outGain = "out_gain";

inline constexpr float gainMinDb = -24.0f;
inline constexpr float gainMaxDb = 24.0f;
inline constexpr float gainStepDb = 0.1f;
inline constexpr float gainDefaultDb = 0.0f;

// Subtractive EQ ------------------------------------------------------------
inline constexpr auto subBypass = "sub_bypass";
inline constexpr auto subHpfFreq = "sub_hpf_freq";
inline constexpr auto subHpfSlope = "sub_hpf_slope";

inline constexpr float subHpfMinHz = 20.0f;
inline constexpr float subHpfMaxHz = 300.0f;
inline constexpr float subHpfDefaultHz = 80.0f;

inline constexpr float subCutMinDb = -15.0f;
inline constexpr float subCutMaxDb = 0.0f;
inline constexpr float subQMin = 0.5f;
inline constexpr float subQMax = 6.0f;
inline constexpr float subQDefault = 1.4f;

/** One cut band: its parameter IDs, display name and frequency range. */
struct SubBandSpec
{
    const char* freqId;
    const char* cutId;
    const char* qId;
    const char* name;
    float minHz;
    float maxHz;
    float defaultHz;
};

inline constexpr std::array<SubBandSpec, 4> subBands { {
    { "sub_b1_freq", "sub_b1_cut", "sub_b1_q", "Mud", 200.0f, 300.0f, 250.0f },
    { "sub_b2_freq", "sub_b2_cut", "sub_b2_q", "Boxy", 400.0f, 600.0f, 500.0f },
    { "sub_b3_freq", "sub_b3_cut", "sub_b3_q", "Nasal", 800.0f, 1000.0f, 900.0f },
    { "sub_b4_freq", "sub_b4_cut", "sub_b4_q", "Harsh", 2000.0f, 4000.0f, 3000.0f },
} };

/** Builds the full parameter list, used to construct the AudioProcessorValueTreeState. */
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

} // namespace lionvoice::params
