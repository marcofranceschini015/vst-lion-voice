// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Marco Franceschini
//
// Renders the plugin interface to PNG files without a DAW, for design reviews and the README.
//
// Usage: LionVoiceGuiSnapshot [output-directory]     (default: build/snapshots)
//
// Writes:
//   console.png    the full plugin window at 100 % size (2x pixel density)
//   controls.png   every knob colour and the key/LED states, enlarged

#include "gui/LionLookAndFeel.h"
#include "gui/Palette.h"
#include "gui/SteelBackground.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <memory>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{

bool writePng (const juce::Image& image, const juce::File& file)
{
    file.deleteFile();
    juce::FileOutputStream stream (file);
    juce::PNGImageFormat png;
    const bool ok = stream.openedOk() && png.writeImageToStream (image, stream);
    std::cout << (ok ? "Wrote " : "FAILED to write ") << file.getFullPathName() << "\n";
    return ok;
}

/** All knob caps at three positions, plus keys and LEDs, on the steel faceplate. */
juce::Image renderControlSheet()
{
    namespace palette = lionvoice::gui::palette;
    using lionvoice::gui::LionLookAndFeel;

    const juce::Colour caps[] = { palette::capWhite, palette::capBrown, palette::capBlue, palette::capGreen,
                                  palette::capRed,   palette::capGrey,  palette::capBlack, palette::capOrange };
    const char* names[] = { "WHITE", "BROWN", "BLUE", "GREEN", "RED", "GREY", "BLACK", "ORANGE" };
    const float positions[] = { 0.0f, 0.5f, 1.0f };

    constexpr int cell = 90;
    constexpr float scale = 2.0f;
    const int width = cell * 8;
    const int height = cell * 3 + 140;

    juce::Image image (juce::Image::ARGB, juce::roundToInt (width * scale), juce::roundToInt (height * scale), true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (scale));

    lionvoice::gui::steel::drawBrushedSteel (g, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) });

    const float start = juce::MathConstants<float>::pi * 1.25f;
    const float end = juce::MathConstants<float>::pi * 2.75f;

    for (int c = 0; c < 8; ++c)
    {
        for (int p = 0; p < 3; ++p)
            LionLookAndFeel::drawKnob (g, juce::Rectangle<float> (static_cast<float> (c * cell), static_cast<float> (p * cell), cell, cell).reduced (12.0f),
                                       positions[p], start, end, caps[c]);

        lionvoice::gui::steel::drawEngravedText (g, names[c],
                                                 { static_cast<float> (c * cell), static_cast<float> (3 * cell), cell, 20.0f },
                                                 juce::Justification::centred, 11.0f);
    }

    const float y = static_cast<float> (3 * cell + 40);
    LionLookAndFeel::drawKey (g, { 30.0f, y, 26.0f, 26.0f }, false, false, palette::ledRedOn);
    LionLookAndFeel::drawKey (g, { 80.0f, y, 26.0f, 26.0f }, true, false, palette::ledRedOn);
    LionLookAndFeel::drawKey (g, { 130.0f, y, 26.0f, 26.0f }, true, true, palette::ledGreenOn);
    LionLookAndFeel::drawLed (g, { 200.0f, y + 8.0f, 10.0f, 10.0f }, false, palette::ledGreenOn);
    LionLookAndFeel::drawLed (g, { 240.0f, y + 8.0f, 10.0f, 10.0f }, true, palette::ledGreenOn);
    LionLookAndFeel::drawLed (g, { 280.0f, y + 8.0f, 10.0f, 10.0f }, true, palette::ledAmberOn);
    lionvoice::gui::steel::drawScrew (g, { 340.0f, y + 13.0f }, 6.0f, 0.5f);
    lionvoice::gui::steel::drawEngravedText (g, "KEYS / LEDS / SCREW", { 380.0f, y, 300.0f, 26.0f },
                                             juce::Justification::centredLeft, 12.0f);

    return image;
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const auto outputDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "build/snapshots");
    outputDir.createDirectory();

    std::unique_ptr<juce::AudioProcessor> processor (createPluginFilter());
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor->createEditorAndMakeActive());

    bool ok = writePng (editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f),
                        outputDir.getChildFile ("console.png"));
    ok = writePng (renderControlSheet(), outputDir.getChildFile ("controls.png")) && ok;

    // Same teardown order a host uses: tell the processor first, then delete the editor.
    processor->editorBeingDeleted (editor.get());
    editor.reset();
    processor.reset();
    return ok ? 0 : 1;
}
