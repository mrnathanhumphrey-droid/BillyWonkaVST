#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWKnob.h"
#include "ADSRDisplay.h"
#include "BWColours.h"

/**
 * MainTab — Tab 1: Amp envelope + dynamics + oscillator controls.
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

    // Dynamics row
    BWKnob knobDrive   { "DRIVE",   "" };
    BWKnob knobLevel   { "LEVEL",   "" };
    BWKnob knobPitchAmt{ "P.ENV",   "st" };
    BWKnob knobPitchTm { "P.TIME",  "ms" };
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

    // Bass mode
    juce::ComboBox bassModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> bassModeAttach;

    // Toggle attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> glideAttach;

    // Section labels
    juce::Label ampLabel, dynLabel, oscLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainTab)
};
