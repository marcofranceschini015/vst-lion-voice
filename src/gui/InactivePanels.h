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

/** 1176 style FET compressor: INPUT, OUTPUT, ATTACK, RELEASE, RATIO keys, MIX. */
class CharacterPanel final : public Panel
{
public:
    CharacterPanel();

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;

    Knob input { "INPUT", palette::capBlack };
    Knob output { "OUTPUT", palette::capBlack };
    Knob attack { "ATTACK", palette::capBlack };
    Knob release { "RELEASE", palette::capBlack };
    Knob mix { "MIX", palette::capGrey };
    std::array<std::unique_ptr<juce::ToggleButton>, 4> ratioKeys;
    juce::Rectangle<int> ratioLabelArea;
};

/** Main EQ: BODY (Pultec-style low shelf), PRESENCE (bell), AIR (soft high shelf). */
class MainEqPanel final : public Panel
{
public:
    MainEqPanel();

private:
    void layoutContent (juce::Rectangle<int> contentArea) override;
    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;

    Knob bodyFreq { "FREQ", palette::capBrown };
    Knob bodyBoost { "BOOST", palette::capBrown };
    Knob bodyAtten { "ATTEN", palette::capBrown };
    Knob presenceFreq { "FREQ", palette::capGreen };
    Knob presenceQ { "Q", palette::capGrey };
    Knob presenceBoost { "BOOST", palette::capGreen };
    Knob airBoost { "BOOST", palette::capRed };
    std::array<juce::Rectangle<int>, 3> sections;
};

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
