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

    // --- Dynamics row ---
    knobDrive.attach(apvts, ParamIDs::driveAmount);
    knobLevel.attach(apvts, ParamIDs::masterVolume);
    knobPitchAmt.attach(apvts, ParamIDs::pitchEnvAmount);
    knobPitchTm.attach(apvts, ParamIDs::pitchEnvTime);
    addAndMakeVisible(knobDrive);
    addAndMakeVisible(knobLevel);
    addAndMakeVisible(knobPitchAmt);
    addAndMakeVisible(knobPitchTm);

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

    // Bass mode
    bassModeBox.addItem("PLUCK", 1);
    bassModeBox.addItem("808", 2);
    bassModeBox.addItem("REESE", 3);
    styleComboBox(bassModeBox);
    bassModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::bassMode, bassModeBox);
    addAndMakeVisible(bassModeBox);

    // Section labels
    styleSectionLabel(ampLabel, "AMPLITUDE ENVELOPE");
    styleSectionLabel(dynLabel, "DYNAMICS / PITCH");
    styleSectionLabel(oscLabel, "OSCILLATORS");
    addAndMakeVisible(ampLabel);
    addAndMakeVisible(dynLabel);
    addAndMakeVisible(oscLabel);
}

void MainTab::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);

    // Section separator lines
    auto drawSep = [&](int y)
    {
        g.setColour(BW::Grey.withAlpha(0.3f));
        g.fillRect(8, y, getWidth() - 16, 1);
    };

    // Approximate separator positions
    int adsrBottom = static_cast<int>(getHeight() * 0.42f);
    int dynBottom = static_cast<int>(getHeight() * 0.68f);
    drawSep(adsrBottom);
    drawSep(dynBottom);
}

void MainTab::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);

    // --- Section 1: ADSR (top ~42%) ---
    auto adsrSection = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.42f));
    ampLabel.setBounds(adsrSection.removeFromTop(16));
    auto adsrViz = adsrSection.removeFromTop(static_cast<int>(adsrSection.getHeight() * 0.6f));
    adsrDisplay.setBounds(adsrViz.reduced(0, 2));

    // ADSR knobs row
    auto adsrKnobRow = adsrSection;
    int knobW = adsrKnobRow.getWidth() / 4;
    knobAttack.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobDecay.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobSustain.setBounds(adsrKnobRow.removeFromLeft(knobW));
    knobRelease.setBounds(adsrKnobRow);

    // --- Section 2: Dynamics (~26%) ---
    auto dynSection = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.45f));
    dynLabel.setBounds(dynSection.removeFromTop(16));

    // Bass mode selector at the right
    auto modeArea = dynSection.removeFromRight(90);
    bassModeBox.setBounds(modeArea.removeFromTop(24).reduced(4, 0));
    glideToggle.setBounds(modeArea.removeFromTop(24).reduced(4, 0));
    monoToggle.setBounds(modeArea.removeFromTop(24).reduced(4, 0));

    int dynKnobW = dynSection.getWidth() / 4;
    knobDrive.setBounds(dynSection.removeFromLeft(dynKnobW));
    knobLevel.setBounds(dynSection.removeFromLeft(dynKnobW));
    knobPitchAmt.setBounds(dynSection.removeFromLeft(dynKnobW));
    knobPitchTm.setBounds(dynSection);

    // --- Section 3: Oscillators (remaining) ---
    oscLabel.setBounds(bounds.removeFromTop(16));

    // Wave selectors row
    auto waveRow = bounds.removeFromTop(24);
    osc1WaveBox.setBounds(waveRow.removeFromLeft(waveRow.getWidth() / 2).reduced(4, 0));
    osc2WaveBox.setBounds(waveRow.reduced(4, 0));

    // Osc knobs
    int oscKnobW = bounds.getWidth() / 6;
    knobOsc1Level.setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc2Level.setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc2Detune.setBounds(bounds.removeFromLeft(oscKnobW));
    knobOsc3Level.setBounds(bounds.removeFromLeft(oscKnobW));
    knobNoiseLevel.setBounds(bounds.removeFromLeft(oscKnobW));
    knobFeedback.setBounds(bounds);
}
