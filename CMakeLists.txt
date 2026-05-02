cmake_minimum_required(VERSION 3.22)

project(SimpleSamplerSuite VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

# JUCE is expected as a sibling directory: ../JUCE
add_subdirectory(../JUCE JUCE)

# FxmeJuceTools custom module — registered once, linked by all kit sub-projects
juce_add_module(../JUCE/usermodules/FxmeJuceTools)

# ── Kit plugins ────────────────────────────────────────────────────────────
add_subdirectory(Kits/BlackWidow)
add_subdirectory(Kits/Century)
