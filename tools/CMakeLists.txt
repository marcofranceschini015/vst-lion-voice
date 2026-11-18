# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini
#
# Developer tools.

# Renders the plugin interface to PNG files (no DAW needed).
# It compiles its own copy of the plugin sources as a plain app, so the plugin build stays untouched.
juce_add_console_app(LionVoiceGuiSnapshot
    PRODUCT_NAME "LionVoiceGuiSnapshot")

target_sources(LionVoiceGuiSnapshot
    PRIVATE
        GuiSnapshot.cpp
        ${LIONVOICE_PLUGIN_SOURCES})

target_include_directories(LionVoiceGuiSnapshot
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src)

target_compile_definitions(LionVoiceGuiSnapshot
    PRIVATE
        JucePlugin_Name="Lion Voice"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(LionVoiceGuiSnapshot
    PRIVATE
        LionVoiceDsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
