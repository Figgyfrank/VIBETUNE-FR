cmake_minimum_required(VERSION 3.22)

project(VibeTune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VibeTune
    COMPANY_NAME "VibeTune"
    PLUGIN_MANUFACTURER_CODE Vbtn
    PLUGIN_CODE Vtun
    FORMATS VST3 Standalone
    PRODUCT_NAME "VibeTune"
    VST3_CATEGORIES Fx "Pitch Shift"
    ICON_BIG   "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_512.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_64.png"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(VibeTune)

target_sources(VibeTune PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Presets.h
    Source/DSP/AutoTuneEngine.h)

target_compile_definitions(VibeTune PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VibeTune
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
