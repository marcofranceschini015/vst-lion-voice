# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini
#
# Tests. Catch2 is downloaded at configure time (pinned tag).
#   LionVoiceDspTests     unit tests of the JUCE-free DSP code (fast to build)
#   LionVoicePluginTests  the whole plugin as a host sees it (compiles its own copy of the plugin sources)

include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(LionVoiceDspTests
    BiquadTests.cpp
    CharacterTests.cpp
    DynamicsTests.cpp
    GainStageTests.cpp
    LevelMeterTests.cpp
    LevelerTests.cpp
    MainEqTests.cpp
    ModuleBypassTests.cpp
    SmootherTests.cpp
    SubtractiveEqTests.cpp)

target_link_libraries(LionVoiceDspTests
    PRIVATE
        LionVoiceDsp
        Catch2::Catch2WithMain)

# Registers every TEST_CASE with CTest (and the VS Code test explorer).
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(LionVoiceDspTests)

juce_add_console_app(LionVoicePluginTests
    PRODUCT_NAME "LionVoicePluginTests")

target_sources(LionVoicePluginTests
    PRIVATE
        PluginTests.cpp
        ${LIONVOICE_PLUGIN_SOURCES})

target_include_directories(LionVoicePluginTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src)

target_compile_definitions(LionVoicePluginTests
    PRIVATE
        JucePlugin_Name="Lion Voice"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(LionVoicePluginTests
    PRIVATE
        LionVoiceDsp
        Catch2::Catch2WithMain
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)

catch_discover_tests(LionVoicePluginTests)
