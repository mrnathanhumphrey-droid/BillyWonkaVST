#include "FilterTab.h"
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

FilterTab::FilterTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
{
    // --- Main filter knobs ---
    knobCutoff.setLargeMode(true);
    knobCutoff.attach(apvts, ParamIDs::filterCutoff);
    knobResonance.attach(apvts, ParamIDs::filterResonance);
    knobEnvAmount.attach(apvts, ParamIDs::filterEnvAmount);
    knobKeyTrack.attach(apvts, ParamIDs::filterKeyTrack);
    knobFilterSat.attach(apvts, ParamIDs::filterSaturation);
    addAndMakeVisible(knobCutoff);
    addAndMakeVisible(knobResonance);
    addAndMakeVisible(knobEnvAmount);
    addAndMakeVisible(knobKeyTrack);
    addAndMakeVisible(knobFilterSat);

    // Slope selector
    slopeBox.addItem("LP 12dB", 1);
    slopeBox.addItem("LP 24dB", 2);
    styleComboBox(slopeBox);
    slopeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::filterSlope, slopeBox);
    addAndMakeVisible(slopeBox);

    // --- Filter envelope ---
    filterAdsrDisplay.attachToAPVTS(apvts,
        ParamIDs::envFAttack, ParamIDs::envFDecay,
        ParamIDs::envFSustain, ParamIDs::envFRelease);
    addAndMakeVisible(filterAdsrDisplay);

    knobFAttack.attach(apvts, ParamIDs::envFAttack);
    knobFDecay.attach(apvts, ParamIDs::envFDecay);
    knobFSustain.attach(apvts, ParamIDs::envFSustain);
    knobFRelease.attach(apvts, ParamIDs::envFRelease);
    addAndMakeVisible(knobFAttack);
    addAndMakeVisible(knobFDecay);
    addAndMakeVisible(knobFSustain);
    addAndMakeVisible(knobFRelease);

    // --- LFO ---
    knobLFORate.attach(apvts, ParamIDs::lfoRate);
    knobLFODepth.attach(apvts, ParamIDs::lfoDepth);
    addAndMakeVisible(knobLFORate);
    addAndMakeVisible(knobLFODepth);

    lfoShapeBox.addItem("Triangle", 1);
    lfoShapeBox.addItem("Square", 2);
    lfoShapeBox.addItem("Sine", 3);
    styleComboBox(lfoShapeBox);
    lfoShapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::lfoWaveform, lfoShapeBox);
    addAndMakeVisible(lfoShapeBox);

    lfoTargetBox.addItem("Filter Cutoff", 1);
    lfoTargetBox.addItem("Pitch", 2);
    lfoTargetBox.addItem("Amplitude", 3);
    styleComboBox(lfoTargetBox);
    lfoTargetAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::lfoTarget, lfoTargetBox);
    addAndMakeVisible(lfoTargetBox);

    lfoSyncToggle.setColour(juce::ToggleButton::textColourId, BW::PinkSoft);
    lfoSyncAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::lfoSync, lfoSyncToggle);
    addAndMakeVisible(lfoSyncToggle);

    // Labels
    styleSectionLabel(filterLabel, "MOOG LADDER FILTER");
    styleSectionLabel(filterEnvLabel, "FILTER ENVELOPE");
    styleSectionLabel(lfoLabel, "LFO");
    addAndMakeVisible(filterLabel);
    addAndMakeVisible(filterEnvLabel);
    addAndMakeVisible(lfoLabel);
}

void FilterTab::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);

    // Section separators
    g.setColour(BW::Grey.withAlpha(0.3f));
    int s1 = static_cast<int>(getHeight() * 0.42f);
    int s2 = static_cast<int>(getHeight() * 0.72f);
    g.fillRect(8, s1, getWidth() - 16, 1);
    g.fillRect(8, s2, getWidth() - 16, 1);
}

void FilterTab::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);

    // --- Section 1: Filter controls (top ~42%) ---
    auto filterSection = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.42f));
    filterLabel.setBounds(filterSection.removeFromTop(16));

    // Slope selector top-right
    slopeBox.setBounds(filterSection.removeFromTop(24).removeFromRight(120).reduced(4, 0));

    // Cutoff is larger, centre
    auto filterKnobRow = filterSection;
    int knobW = filterKnobRow.getWidth() / 5;
    knobCutoff.setBounds(filterKnobRow.removeFromLeft(knobW * 2).reduced(8, 0)); // Double wide
    knobResonance.setBounds(filterKnobRow.removeFromLeft(knobW));
    knobEnvAmount.setBounds(filterKnobRow.removeFromLeft(knobW));

    // Stack keytrack and drive vertically in remaining space
    auto rightCol = filterKnobRow;
    knobKeyTrack.setBounds(rightCol.removeFromTop(rightCol.getHeight() / 2));
    knobFilterSat.setBounds(rightCol);

    // --- Section 2: Filter envelope (~30%) ---
    auto fEnvSection = bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.52f));
    filterEnvLabel.setBounds(fEnvSection.removeFromTop(16));

    auto fEnvViz = fEnvSection.removeFromTop(static_cast<int>(fEnvSection.getHeight() * 0.55f));
    filterAdsrDisplay.setBounds(fEnvViz.reduced(0, 2));

    auto fEnvKnobs = fEnvSection;
    int fKnobW = fEnvKnobs.getWidth() / 4;
    knobFAttack.setBounds(fEnvKnobs.removeFromLeft(fKnobW));
    knobFDecay.setBounds(fEnvKnobs.removeFromLeft(fKnobW));
    knobFSustain.setBounds(fEnvKnobs.removeFromLeft(fKnobW));
    knobFRelease.setBounds(fEnvKnobs);

    // --- Section 3: LFO (remaining) ---
    lfoLabel.setBounds(bounds.removeFromTop(16));

    auto lfoRow1 = bounds.removeFromTop(24);
    int halfW = lfoRow1.getWidth() / 2;
    lfoShapeBox.setBounds(lfoRow1.removeFromLeft(halfW).reduced(4, 0));
    lfoTargetBox.setBounds(lfoRow1.reduced(4, 0));

    auto lfoRow2 = bounds;
    int lfoKnobW = lfoRow2.getWidth() / 3;
    knobLFORate.setBounds(lfoRow2.removeFromLeft(lfoKnobW));
    knobLFODepth.setBounds(lfoRow2.removeFromLeft(lfoKnobW));
    lfoSyncToggle.setBounds(lfoRow2.reduced(8, 8));
}
