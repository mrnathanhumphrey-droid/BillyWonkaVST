#include "MainTab.h"
#include "PluginParameters.h"

static void styleSectionLabel(juce::Label& l, const juce::String& text)
{
    l.setText(text, juce::dontSendNotification);
    l.setFont(juce::Font(10.0f, juce::Font::bold).withExtraKerningFactor(0.15f));
    l.setColour(juce::Label::textColourId, BW::TextMuted);
    l.setJustificationType(juce::Justification::centredLeft);
}

static void styleComboBox(juce::ComboBox& box)
{
    box.setColour(juce::ComboBox::backgroundColourId, BW::Deep);
    box.setColour(juce::ComboBox::textColourId, BW::White);
    box.setColour(juce::ComboBox::outlineColourId, BW::Grey);
}

MainTab::MainTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
{
    // --- ADSR Display ---
    adsrDisplay.attachToAPVTS(apvts,
        ParamIDs::envAAttack, ParamIDs::envADecay,
        ParamIDs::envASustain, ParamIDs::envARelease);
    addAndMakeVisible(adsrDisplay);

    // --- Amp ADSR knobs ---
    knobAttack.attach(apvts, ParamIDs::envAAttack);
    knobDecay.attach(apvts, ParamIDs::envADecay);
    knobSustain.attach(apvts, ParamIDs::envASustain);
    knobRelease.attach(apvts, ParamIDs::envARelease);
    addAndMakeVisible(knobAttack);
    addAndMakeVisible(knobDecay);
    addAndMakeVisible(knobSustain);
    addAndMakeVisible(knobRelease);

    // --- Pitch envelope + glide-time (bottom row) ---
    knobPitchAmt.attach(apvts, ParamIDs::pitchEnvAmount);
    knobPitchTm.attach(apvts, ParamIDs::pitchEnvTime);
    knobGlideTime.attach(apvts, ParamIDs::glideTime);
    addAndMakeVisible(knobPitchAmt);
    addAndMakeVisible(knobPitchTm);
    addAndMakeVisible(knobGlideTime);

    glideToggle.setColour(juce::ToggleButton::textColourId, BW::PinkSoft);
    glideToggle.setColour(juce::ToggleButton::tickColourId, BW::Pink);
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::glideOn, glideToggle);
    addAndMakeVisible(glideToggle);

    addAndMakeVisible(monoToggle);
    monoToggle.setToggleState(true, juce::dontSendNotification); // Always mono for this synth
    monoToggle.setEnabled(false);

    // --- Oscillator row ---
    knobOsc1Level.attach(apvts, ParamIDs::osc1Level);
    knobOsc2Level.attach(apvts, ParamIDs::osc2Level);
    knobOsc2Detune.attach(apvts, ParamIDs::osc2Detune);
    knobOsc3Level.attach(apvts, ParamIDs::osc3Level);
    knobNoiseLevel.attach(apvts, ParamIDs::noiseLevel);
    knobFeedback.attach(apvts, ParamIDs::feedbackAmount);
    addAndMakeVisible(knobOsc1Level);
    addAndMakeVisible(knobOsc2Level);
    addAndMakeVisible(knobOsc2Detune);
    addAndMakeVisible(knobOsc3Level);
    addAndMakeVisible(knobNoiseLevel);
    addAndMakeVisible(knobFeedback);

    // Wave selectors
    juce::StringArray waveNames { "Sine", "Triangle", "Saw", "Rev Saw", "Square", "Pulse" };
    for (int i = 0; i < waveNames.size(); ++i)
    {
        osc1WaveBox.addItem(waveNames[i], i + 1);
        osc2WaveBox.addItem(waveNames[i], i + 1);
    }
    styleComboBox(osc1WaveBox);
    styleComboBox(osc2WaveBox);
    osc1WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::osc1Waveform, osc1WaveBox);
    osc2WaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::osc2Waveform, osc2WaveBox);
    addAndMakeVisible(osc1WaveBox);
    addAndMakeVisible(osc2WaveBox);

    // Section labels
    styleSectionLabel(ampLabel, "AMPLITUDE ENVELOPE");
    styleSectionLabel(oscLabel, "OSCILLATORS");
    styleSectionLabel(dynLabel, "PITCH / GLIDE");
    addAndMakeVisible(ampLabel);
    addAndMakeVisible(oscLabel);
    addAndMakeVisible(dynLabel);
}

void MainTab::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);

    auto drawSep = [&](int y)
    {
        g.setColour(BW::Grey.withAlpha(0.3f));
        g.fillRect(8, y, getWidth() - 16, 1);
    };

    int adsrBottom = static_cast<int>(getHeight() * 0.45f);
    int oscBottom  = static_cast<int>(getHeight() * 0.82f);
    drawSep(adsrBottom);
    drawSep(oscBottom);
}

void MainTab::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);

    // --- Section 1: ADSR (top ~45%) ---
    auto adsrSection = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.45f));
    ampLabel.setBounds(adsrSection.removeFromTop(16));
    auto adsrViz = adsrSection.removeFromTop(static_cast<int>(adsrSection.getHeight() * 0.6f));
    adsrDisplay.setBounds(adsrViz.reduced(0, 2));

    auto adsrKnobRow = adsrSection;
    int knobW = adsrKnobRow.getWidth() / 4;
    knobAttack.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobDecay.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobSustain.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobRelease.setBounds(adsrKnobRow);

    // --- Section 3: Bottom row — Pitch env + Glide (reserve from bottom first) ---
    auto bottomRow = bounds.removeFromBottom(static_cast<int>(bounds.getHeight() * 0.30f));
    dynLabel.setBounds(bottomRow.removeFromTop(14));

    // 5 cells: P.ENV | P.TIME | GLIDE toggle | MONO toggle | GLIDE TIME knob
    int bw = bottomRow.getWidth() / 5;
    knobPitchAmt.setBounds(bottomRow.removeFromLeft(bw));
    knobPitchTm .setBounds(bottomRow.removeFromLeft(bw));
    auto togglesCol = bottomRow.removeFromLeft(bw).reduced(4, 4);
    glideToggle.setBounds(togglesCol.removeFromTop(togglesCol.getHeight() / 2));
    monoToggle .setBounds(togglesCol);
    knobGlideTime.setBounds(bottomRow.removeFromLeft(bw));
    // (last cell is intentional spacer; keeps layout symmetric if width grows)

    // --- Section 2: Oscillators (middle — whatever's left) ---
    oscLabel.setBounds(bounds.removeFromTop(16));

    // Wave selectors row
    auto waveRow = bounds.removeFromTop(24);
    osc1WaveBox.setBounds(waveRow.removeFromLeft(waveRow.getWidth() / 2).reduced(4, 0));
    osc2WaveBox.setBounds(waveRow.reduced(4, 0));

    int oscKnobW = bounds.getWidth() / 6;
    knobOsc1Level .setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc2Level .setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc2Detune.setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc3Level .setBounds(bounds.removeFromLeft(oscKnobW));
    knobNoiseLevel.setBounds(bounds.removeFromLeft(oscKnobW));
    knobFeedback  .setBounds(bounds);
}
