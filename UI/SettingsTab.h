#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWKnob.h"
#include "BWColours.h"

/**
 * SettingsTab — Tab 3: MIDI, Ableton integration, performance, display.
 * Two-column layout with glassmorphism cards.
 */
class SettingsTab : public juce::Component
{
public:
    SettingsTab(juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    void paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);

    juce::AudioProcessorValueTreeState& apvts;

    // Glide / Portamento controls
    BWKnob knobGlideTime   { "GLIDE TIME", "ms" };
    juce::ToggleButton glideToggle   { "Glide" };
    juce::ToggleButton legatoToggle  { "Legato Only" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> glideAttach, legatoAttach;

    // Tape saturation
    BWKnob knobTapeDrive   { "TAPE DRIVE", "" };
    BWKnob knobTapeSat     { "TAPE SAT",   "" };
    BWKnob knobTapeBump    { "TAPE BUMP",  "" };
    BWKnob knobTapeMix     { "TAPE MIX",   "" };

    // Compressor
    BWKnob knobCompThresh  { "THRESH", "dB" };
    BWKnob knobCompAttack  { "ATTACK", "ms" };
    BWKnob knobCompRelease { "RELEASE","ms" };
    BWKnob knobCompMix     { "MIX",    ""   };
    BWKnob knobCompGain    { "GAIN",   "dB" };

    // Reverb
    BWKnob knobRevMix      { "MIX",   "%"  };
    BWKnob knobRevDecay    { "DECAY", ""   };
    BWKnob knobRevTone     { "TONE",  ""   };
    juce::ToggleButton reverbToggle { "Reverb" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbAttach;

    // Output
    BWKnob knobStereoWidth { "WIDTH",  ""   };

    // Card bounds (for painting)
    juce::Rectangle<int> cardGlide, cardTape, cardComp, cardReverb, cardOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsTab)
};
