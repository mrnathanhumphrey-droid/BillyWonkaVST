#include "BWKnob.h"

BWKnob::BWKnob(const juce::String& labelText, const juce::String& unitSuffix, bool isCentred)
    : unit(unitSuffix)
{
    // --- Slider setup ---
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slider.setPopupDisplayEnabled(false, false, this);
    slider.setDoubleClickReturnValue(true, slider.getValue());
    slider.onValueChange = [this]() { updateValueDisplay(); };
    addAndMakeVisible(slider);

    // --- Label (above knob) ---
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, BW::TextMuted);
    label.setFont(juce::Font(10.0f).withExtraKerningFactor(0.15f));
    addAndMakeVisible(label);

    // --- Value readout (below knob) ---
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setColour(juce::Label::textColourId, BW::PinkSoft);
    valueLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f,
                                  juce::Font::plain));
    addAndMakeVisible(valueLabel);
}

void BWKnob::attach(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, slider);
    updateValueDisplay();
}

void BWKnob::resized()
{
    auto bounds = getLocalBounds();
    int labelH = 14;
    int valueH = 14;

    label.setBounds(bounds.removeFromTop(labelH));
    valueLabel.setBounds(bounds.removeFromBottom(valueH));
    slider.setBounds(bounds);
}

void BWKnob::updateValueDisplay()
{
    float val = static_cast<float>(slider.getValue());
    juce::String text;

    if (std::abs(val) >= 1000.0f)
        text = juce::String(val / 1000.0f, 1) + "k";
    else if (std::abs(val) >= 100.0f)
        text = juce::String(static_cast<int>(val));
    else if (std::abs(val) >= 10.0f)
        text = juce::String(val, 1);
    else
        text = juce::String(val, 2);

    if (unit.isNotEmpty())
        text += " " + unit;

    valueLabel.setText(text, juce::dontSendNotification);
}
