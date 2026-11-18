# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini

cmake_minimum_required(VERSION 3.25)

# Must be set before project() so it applies to every target.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(LionVoice VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(libs/JUCE)

juce_add_plugin(LionVoice
    PRODUCT_NAME "Lion Voice"
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Marco Franceschini"
    BUNDLE_ID com.marcofranceschini.lionvoice
    PLUGIN_MANUFACTURER_CODE MrFr       # four chars, at least one upper-case
    PLUGIN_CODE Lnvc                    # four chars, exactly one upper-case
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx EQ Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD TRUE        # installs into ~/Library/Audio/Plug-Ins after each build
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(LionVoice
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_include_directories(LionVoice
    PRIVATE
        src)

target_compile_definitions(LionVoice
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LionVoice
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
