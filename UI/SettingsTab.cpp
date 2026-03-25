#include "SettingsTab.h"
#include "PluginParameters.h"

SettingsTab::SettingsTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
{
    // --- Glide ---
    knobGlideTime.attach(apvts, ParamIDs::glideTime);
    glideAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::glideOn, glideToggle);
    legatoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::glideLegato, legatoToggle);
    addAndMakeVisible(knobGlideTime);
    addAndMakeVisible(glideToggle);
    addAndMakeVisible(legatoToggle);

    // --- Tape ---
    knobTapeDrive.attach(apvts, ParamIDs::tapeDrive);
    knobTapeSat.attach(apvts, ParamIDs::tapeSaturation);
    knobTapeBump.attach(apvts, ParamIDs::tapeBump);
    knobTapeMix.attach(apvts, ParamIDs::tapeMix);
    addAndMakeVisible(knobTapeDrive);
    addAndMakeVisible(knobTapeSat);
    addAndMakeVisible(knobTapeBump);
    addAndMakeVisible(knobTapeMix);

    // --- Compressor ---
    knobCompThresh.attach(apvts, ParamIDs::compThreshold);
    knobCompAttack.attach(apvts, ParamIDs::compAttack);
    knobCompRelease.attach(apvts, ParamIDs::compRelease);
    knobCompMix.attach(apvts, ParamIDs::compParallelMix);
    knobCompGain.attach(apvts, ParamIDs::compOutputGain);
    addAndMakeVisible(knobCompThresh);
    addAndMakeVisible(knobCompAttack);
    addAndMakeVisible(knobCompRelease);
    addAndMakeVisible(knobCompMix);
    addAndMakeVisible(knobCompGain);

    // --- Reverb ---
    knobRevMix.attach(apvts, ParamIDs::reverbMix);
    knobRevDecay.attach(apvts, ParamIDs::reverbDecay);
    knobRevTone.attach(apvts, ParamIDs::reverbTone);
    reverbAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::reverbEnabled, reverbToggle);
    addAndMakeVisible(knobRevMix);
    addAndMakeVisible(knobRevDecay);
    addAndMakeVisible(knobRevTone);
    addAndMakeVisible(reverbToggle);

    // --- Output ---
    knobStereoWidth.attach(apvts, ParamIDs::stereoWidth);
    addAndMakeVisible(knobStereoWidth);
}

void SettingsTab::paintCard(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    // Glassmorphism card
    g.setColour(BW::PurpleGlass);
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    g.setColour(BW::GreyBorder);
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);

    // Title
    g.setColour(BW::TextMuted);
    g.setFont(juce::Font(10.0f, juce::Font::bold).withExtraKerningFactor(0.15f));
    g.drawText(title, bounds.getX() + 10, bounds.getY() + 4, bounds.getWidth() - 20, 14,
               juce::Justification::centredLeft);
}

void SettingsTab::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);

    paintCard(g, cardGlide, "GLIDE / PORTAMENTO");
    paintCard(g, cardTape, "TAPE SATURATION");
    paintCard(g, cardComp, "COMPRESSOR");
    paintCard(g, cardReverb, "HARMONIC REVERB");
    paintCard(g, cardOutput, "OUTPUT EQ");
}

void SettingsTab::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);
    int cardPad = 6;
    int halfW = (bounds.getWidth() - cardPad) / 2;

    // Two columns
    auto leftCol = bounds.removeFromLeft(halfW);
    bounds.removeFromLeft(cardPad);
    auto rightCol = bounds;

    int cardH = (leftCol.getHeight() - cardPad * 2) / 3;

    // --- Left column: Glide, Tape, Output ---
    cardGlide = leftCol.removeFromTop(cardH);
    leftCol.removeFromTop(cardPad);
    cardTape = leftCol.removeFromTop(cardH);
    leftCol.removeFromTop(cardPad);
    cardOutput = leftCol;

    // --- Right column: Compressor, Reverb ---
    int rightCardH = (rightCol.getHeight() - cardPad) / 2;
    cardComp = rightCol.removeFromTop(rightCardH);
    rightCol.removeFromTop(cardPad);
    cardReverb = rightCol;

    // --- Layout children within cards ---
    auto cardContent = [](juce::Rectangle<int> card) {
        return card.reduced(8).withTrimmedTop(18);
    };

    // Glide
    {
        auto area = cardContent(cardGlide);
        auto toggleRow = area.removeFromTop(24);
        glideToggle.setBounds(toggleRow.removeFromLeft(toggleRow.getWidth() / 2));
        legatoToggle.setBounds(toggleRow);
        knobGlideTime.setBounds(area);
    }

    // Tape
    {
        auto area = cardContent(cardTape);
        int kw = area.getWidth() / 4;
        knobTapeDrive.setBounds(area.removeFromLeft(kw));
        knobTapeSat.setBounds(area.removeFromLeft(kw));
        knobTapeBump.setBounds(area.removeFromLeft(kw));
        knobTapeMix.setBounds(area);
    }

    // Output
    {
        auto area = cardContent(cardOutput);
        knobStereoWidth.setBounds(area);
    }

    // Compressor
    {
        auto area = cardContent(cardComp);
        int kw = area.getWidth() / 5;
        knobCompThresh.setBounds(area.removeFromLeft(kw));
        knobCompAttack.setBounds(area.removeFromLeft(kw));
        knobCompRelease.setBounds(area.removeFromLeft(kw));
        knobCompMix.setBounds(area.removeFromLeft(kw));
        knobCompGain.setBounds(area);
    }

    // Reverb
    {
        auto area = cardContent(cardReverb);
        auto toggleRow = area.removeFromTop(24);
        reverbToggle.setBounds(toggleRow.removeFromLeft(100));

        int kw = area.getWidth() / 3;
        knobRevMix.setBounds(area.removeFromLeft(kw));
        knobRevDecay.setBounds(area.removeFromLeft(kw));
        knobRevTone.setBounds(area);
    }
}
