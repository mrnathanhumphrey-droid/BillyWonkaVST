#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWColours.h"

/**
 * BWKnob — Branded rotary knob with label and value readout.
 *
 * Usage:
 *   BWKnob knob("CUTOFF", "Hz");
 *   knob.attach(apvts, "filter_cutoff");
 *   addAndMakeVisible(knob);
 *
 * Layout (top to bottom):
 *   [LABEL]      — uppercase, tracked, BW::TextMuted
 *   [KNOB]       — rotary, 270deg arc
 *   [VALUE UNIT] — monospace, BW::PinkSoft
 */
class BWKnob : public juce::Component
{
public:
    BWKnob(const juce::String& labelText, const juce::String& unitSuffix = "",
            bool isCentred = false);

    void attach(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);

    void resized() override;

    juce::Slider& getSlider() { return slider; }
    void setLabelText(const juce::String& text) { label.setText(text, juce::dontSendNotification); }

    /** Make the knob larger (for primary controls like CUTOFF). */
    void setLargeMode(bool large) { largeMode = large; }

private:
    juce::Slider slider;
    juce::Label label;
    juce::Label valueLabel;
    juce::String unit;
    bool largeMode = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateValueDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BWKnob)
};
