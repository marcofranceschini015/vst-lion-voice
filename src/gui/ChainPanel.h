// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "Panel.h"

#include <array>
#include <atomic>

namespace lionvoice::gui
{

/**
    Shows the signal chain as a top-to-bottom cascade, IN -> modules -> OUT.
    Each stage has an LED that is lit while the stage is processing audio: IN and OUT always, a module
    when it is implemented and not bypassed.
*/
class ChainPanel final : public Panel, private juce::Timer
{
public:
    explicit ChainPanel (juce::AudioProcessorValueTreeState& state);
    ~ChainPanel() override;

private:
    struct Stage
    {
        const char* name;
        std::atomic<float>* bypassParameter; // nullptr: no bypass (IN/OUT) or not implemented yet
        bool implemented;
        bool lit;
    };

    void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) override;
    void timerCallback() override;
    bool updateLeds();

    std::array<Stage, 8> stages;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChainPanel)
};

} // namespace lionvoice::gui
