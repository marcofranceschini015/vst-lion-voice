// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include "PanelSymbols.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace lionvoice::gui
{

/**
    One module plate of the console: engraved title, symbol in the top-right corner and (optionally)
    a BYPASS key at the bottom.

    Subclasses add their controls, place them in layoutContent() and draw labels in paintContent().
    A panel that is not available yet (setAvailable (false)) is shown dimmed and cannot be touched.
*/
class Panel : public juce::Component
{
public:
    Panel (juce::String titleText, PanelSymbol symbolToShow, bool hasBypassKey);
    ~Panel() override;

    /** Connects the BYPASS key to a plugin parameter. */
    void attachBypass (juce::AudioProcessorValueTreeState& state, const juce::String& parameterId);

    /** false = module not implemented yet: dimmed, controls disabled, "COMING SOON" tag. */
    void setAvailable (bool shouldBeAvailable);
    bool isAvailable() const noexcept { return available; }

    void paint (juce::Graphics& g) override;
    void resized() final;

    static constexpr int titleHeight = 30;
    static constexpr int bypassHeight = 36;
    static constexpr int padding = 10;

protected:
    /** Area between the title and the bypass row. */
    juce::Rectangle<int> getContentArea() const;

    virtual void layoutContent (juce::Rectangle<int> contentArea) { juce::ignoreUnused (contentArea); }
    virtual void paintContent (juce::Graphics& g, juce::Rectangle<int> contentArea) { juce::ignoreUnused (g, contentArea); }

    /** Small engraved label, e.g. a section name inside the panel. */
    static void drawSectionLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                                  juce::Justification justification = juce::Justification::centredLeft);

private:
    juce::String title;
    PanelSymbol symbol;
    bool available = true;

    std::unique_ptr<juce::ToggleButton> bypassKey;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Panel)
};

} // namespace lionvoice::gui
