cmake_minimum_required(VERSION 3.15)
project(StereoCompressorBuild1 VERSION 0.0.1)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.3
)
FetchContent_MakeAvailable(JUCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

juce_add_plugin(StereoCompressorBuild1
    COMPANY_NAME "John Micensky"
    BUNDLE_ID com.johnmicensky.stereocompressorbuild1
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS VST3 AU
    PRODUCT_NAME "StereoCompressorBuild1")

juce_generate_juce_header(StereoCompressorBuild1)

target_sources(StereoCompressorBuild1 PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_link_libraries(StereoCompressorBuild1 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra)


