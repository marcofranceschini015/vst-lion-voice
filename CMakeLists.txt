# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini

cmake_minimum_required(VERSION 3.25)

# Must be set before project() so it applies to every target.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(LionVoice VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIONVOICE_BUILD_TESTS "Build the DSP unit tests" ON)
option(LIONVOICE_BUILD_TOOLS "Build developer tools (GUI snapshot renderer)" ON)

add_subdirectory(libs/JUCE)

# --- DSP library (pure C++, no JUCE) -------------------------------------------
# Shared by the plugin and the unit tests. JUCE is only used here for its recommended compiler flags.
add_library(LionVoiceDsp STATIC
    src/dsp/BiquadDesign.cpp
    src/dsp/SubtractiveEq.cpp)

target_include_directories(LionVoiceDsp
    PUBLIC
        src)

target_compile_features(LionVoiceDsp
    PUBLIC
        cxx_std_20)

target_link_libraries(LionVoiceDsp
    PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# --- Plugin ---------------------------------------------------------------------

# Plugin sources (also compiled by the developer tools).
set(LIONVOICE_PLUGIN_SOURCES
    ${PROJECT_SOURCE_DIR}/src/Parameters.cpp
    ${PROJECT_SOURCE_DIR}/src/PluginProcessor.cpp
    ${PROJECT_SOURCE_DIR}/src/PluginEditor.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/ChainPanel.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/ConsoleView.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/InOutPanel.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/InactivePanels.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Knob.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/LevelMeterDisplay.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/LionLogo.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/LionLookAndFeel.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/Panel.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/PanelSymbols.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/SteelBackground.cpp
    ${PROJECT_SOURCE_DIR}/src/gui/SubtractiveEqPanel.cpp)

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
        ${LIONVOICE_PLUGIN_SOURCES})

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
        LionVoiceDsp
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# --- Tests ----------------------------------------------------------------------
if(LIONVOICE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Developer tools --------------------------------------------------------------
if(LIONVOICE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()
