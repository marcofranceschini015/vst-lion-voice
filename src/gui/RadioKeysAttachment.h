// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace lionvoice::gui
{

/**
    Connects a row of keys to one choice parameter, like the ratio buttons of a 1176: key N selects choice N,
    exactly one key is lit, and automation / preset changes update the keys. Message thread only.
*/
class RadioKeysAttachment
{
public:
    /** keysInOrder[i] selects choice i of the parameter. The keys must outlive the attachment. */
    RadioKeysAttachment (juce::RangedAudioParameter& parameter, std::vector<juce::Button*> keysInOrder)
        : keys (std::move (keysInOrder))
        , attachment (parameter, [this] (float value) { showChoice (juce::roundToInt (value)); }, nullptr)
    {
        for (std::size_t i = 0; i < keys.size(); ++i)
        {
            keys[i]->setClickingTogglesState (false); // the parameter decides which key is lit
            keys[i]->onClick = [this, i] { attachment.setValueAsCompleteGesture (static_cast<float> (i)); };
        }

        attachment.sendInitialUpdate();
    }

    ~RadioKeysAttachment()
    {
        for (auto* key : keys)
            key->onClick = nullptr;
    }

private:
    void showChoice (int index)
    {
        for (std::size_t i = 0; i < keys.size(); ++i)
            keys[i]->setToggleState (static_cast<int> (i) == index, juce::dontSendNotification);
    }

    std::vector<juce::Button*> keys;
    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE (RadioKeysAttachment)
};

} // namespace lionvoice::gui
