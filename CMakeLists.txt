cmake_minimum_required(VERSION 3.22)

# macOS: universal binary (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")

project(KikoStartGRO VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.8
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

option(KG_COPY_AFTER_BUILD "Install the plugin into the user's plugin folder after building" ON)

# Formats per platform. Add LV2 to the Linux list if you want it too.
if(APPLE)
    set(KG_FORMATS VST3 AU Standalone)
else()
    set(KG_FORMATS VST3 Standalone)
endif()

juce_add_plugin(KikoStartGRO
    COMPANY_NAME                "KikoAudio"
    PLUGIN_MANUFACTURER_CODE    Kiko
    PLUGIN_CODE                 Kgro
    FORMATS                     ${KG_FORMATS}
    PRODUCT_NAME                "KikoStart GRO"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_MusicEffect
    COPY_PLUGIN_AFTER_BUILD     ${KG_COPY_AFTER_BUILD})

target_sources(KikoStartGRO PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Curves.h)

target_compile_definitions(KikoStartGRO PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(KikoStartGRO
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
