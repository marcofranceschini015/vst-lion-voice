# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini
#
# Unit tests for the JUCE-free DSP code. Catch2 is downloaded at configure time (pinned tag).

include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.16.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(LionVoiceDspTests
    BiquadTests.cpp
    SmootherTests.cpp)

target_link_libraries(LionVoiceDspTests
    PRIVATE
        LionVoiceDsp
        Catch2::Catch2WithMain)

# Registers every TEST_CASE with CTest (and the VS Code test explorer).
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(LionVoiceDspTests)
