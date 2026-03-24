cmake_minimum_required(VERSION 3.22)
project(GrooveEngineRnB VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(GrooveEngineRnB
    COMPANY_NAME "GrooveEngine"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Grvn
    PLUGIN_CODE GRNB
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Groove Engine RnB"
)

target_sources(GrooveEngineRnB
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        DSP/BLOscillator.cpp
        DSP/OscillatorBank.cpp
        DSP/NoiseGenerator.cpp
        DSP/MoogLadderFilter.cpp
        DSP/EnvelopeGenerator.cpp
        DSP/PitchEnvelope.cpp
        DSP/PortamentoEngine.cpp
        DSP/LFOEngine.cpp
        DSP/ModMatrix.cpp
        DSP/MixerSection.cpp
        DSP/OutputStage.cpp
        DSP/MoogTapeSaturator.cpp
        DSP/RnBCompressor.cpp
        DSP/HarmonicReverb.cpp
        DSP/NoteManager.cpp
        DSP/SynthEngine.cpp
        UI/BWLookAndFeel.cpp
        UI/BWKnob.cpp
        UI/HeaderBar.cpp
        UI/ADSRDisplay.cpp
        UI/MainTab.cpp
        UI/FilterTab.cpp
        UI/SettingsTab.cpp
        UI/AIAssistTab.cpp
)

target_include_directories(GrooveEngineRnB
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/DSP
        ${CMAKE_SOURCE_DIR}/UI
)

target_compile_definitions(GrooveEngineRnB
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(GrooveEngineRnB
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Remove /MP from ALL targets to prevent heap exhaustion on low-memory systems.
# JUCE's juce_recommended_warning_flags interface library propagates /MP to every
# target that links it (including _VST3, _Standalone sub-targets).
get_target_property(_juce_warn_opts juce_recommended_warning_flags INTERFACE_COMPILE_OPTIONS)
if(_juce_warn_opts)
    list(FILTER _juce_warn_opts EXCLUDE REGEX "/MP")
    set_target_properties(juce_recommended_warning_flags PROPERTIES INTERFACE_COMPILE_OPTIONS "${_juce_warn_opts}")
endif()

