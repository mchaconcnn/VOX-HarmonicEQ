cmake_minimum_required(VERSION 3.22)

project(HarmonicEQ VERSION 0.3.1 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(HarmonicEQ
    COMPANY_NAME "rco.cr"
    BUNDLE_ID "cr.rco.vox.harmoniceq"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE RCOc
    PLUGIN_CODE Heq1
    AU_MAIN_TYPE kAudioUnitType_Effect
    FORMATS AU Standalone
    PRODUCT_NAME "VOX HarmonicEQ"
)

juce_generate_juce_header(HarmonicEQ)

target_sources(HarmonicEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PitchDetector.cpp
    Source/PitchDetector.h
    Source/HarmonicFilterBank.cpp
    Source/HarmonicFilterBank.h
    Source/WaveformMonitor.h
)

target_compile_features(HarmonicEQ PRIVATE cxx_std_20)
target_compile_definitions(HarmonicEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(HarmonicEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_add_console_app(HarmonicEQTests PRODUCT_NAME "VOX HarmonicEQ Tests")
target_sources(HarmonicEQTests PRIVATE
    Tests/DspTests.cpp
    Source/PitchDetector.cpp
    Source/HarmonicFilterBank.cpp
    Source/WaveformMonitor.h
)
target_include_directories(HarmonicEQTests PRIVATE Source)
target_compile_features(HarmonicEQTests PRIVATE cxx_std_20)
target_compile_definitions(HarmonicEQTests PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(HarmonicEQTests PRIVATE
    juce::juce_audio_basics
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

enable_testing()
add_test(NAME HarmonicEQDSP COMMAND HarmonicEQTests)
