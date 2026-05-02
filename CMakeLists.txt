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
# Build all kits by default. To build a single kit:
#   cmake -B build -DKIT=BlackWidow
#   cmake -B build -DKIT=Century
set(KIT "" CACHE STRING "Build only this kit (leave empty to build all)")

set(_all_kits BlackWidow Century)

if(KIT STREQUAL "")
    foreach(_kit IN LISTS _all_kits)
        add_subdirectory(Kits/${_kit})
    endforeach()
else()
    if(NOT "${KIT}" IN_LIST _all_kits)
        message(FATAL_ERROR "Unknown KIT '${KIT}'. Valid values: ${_all_kits}")
    endif()
    add_subdirectory(Kits/${KIT})
endif()
