#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWKnob.h"
#include "ADSRDisplay.h"
#include "BWColours.h"

/**
 * MainTab — Tab 1: Amp envelope, oscillators, pitch envelope, glide.
 *
 * Layout (top to bottom):
 *   - AMPLITUDE ENVELOPE: ADSR viz + 4 knobs (Attack, Decay, Sustain, Release)
 *   - OSCILLATORS: OSC1/OSC2 wave selectors + 6 knobs (OSC1, OSC2, Detune, Sub, Noise, Fdbk)
 *   - DYNAMICS / GLIDE: P.ENV, P.TIME, Glide toggle, Mono toggle, small Glide-Time knob
 *
 * (Driver selector + Level knob live in the HeaderBar.
 *  Drive knob lives in the Effects tab.)
 */
class MainTab : public juce::Component
{
public:
    MainTab(juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    // ADSR display
    ADSRDisplay adsrDisplay;

    // Amp envelope knobs
    BWKnob knobAttack  { "ATTACK",  "s" };
    BWKnob knobDecay   { "DECAY",   "s" };
    BWKnob knobSustain { "SUSTAIN", "%" };
    BWKnob knobRelease { "RELEASE", "s" };

    // Pitch envelope + glide-time (compact bottom row)
    BWKnob knobPitchAmt  { "P.ENV",      "st" };
    BWKnob knobPitchTm   { "P.TIME",     "ms" };
    BWKnob knobGlideTime { "GLIDE TIME", "ms" };
    juce::ToggleButton glideToggle { "GLIDE" };
    juce::ToggleButton monoToggle  { "MONO" };

    // Oscillator row
    BWKnob knobOsc1Level  { "OSC 1",  "" };
    BWKnob knobOsc2Level  { "OSC 2",  "" };
    BWKnob knobOsc2Detune { "DETUNE", "ct" };
    BWKnob knobOsc3Level  { "SUB",    "" };
    BWKnob knobNoiseLevel { "NOISE",  "" };
    BWKnob knobFeedback   { "FDBK",   "" };

    // Wave selectors
    juce::ComboBox osc1WaveBox, osc2WaveBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttach, osc2WaveAttach;

    // Toggle attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> glideAttach;

    // Section labels
    juce::Label ampLabel, oscLabel, dynLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainTab)
};
