#include "HeaderBar.h"
#include "PluginParameters.h"

HeaderBar::HeaderBar(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
{
    // --- Title ---
    titleLabel.setText("BW BASS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold).withExtraKerningFactor(0.2f));
    titleLabel.setColour(juce::Label::textColourId, BW::Pink);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // --- Preset nav ---
    auto setupSmallBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, BW::Deep);
        btn.setColour(juce::TextButton::textColourOffId, BW::PurpleGlow);
    };
    setupSmallBtn(prevBtn);
    setupSmallBtn(nextBtn);
    prevBtn.onClick = [this]() { if (onPresetNav) onPresetNav(-1); };
    nextBtn.onClick = [this]() { if (onPresetNav) onPresetNav(+1); };
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);

    presetNameBtn.setColour(juce::TextButton::buttonColourId, BW::Deep);
    presetNameBtn.setColour(juce::TextButton::textColourOffId, BW::White);
    presetNameBtn.onClick = [this]() { if (onPresetBrowserOpen) onPresetBrowserOpen(); };
    addAndMakeVisible(presetNameBtn);

    // Save / Init
    saveBtn.setColour(juce::TextButton::buttonColourId, BW::PurpleDark);
    saveBtn.setColour(juce::TextButton::textColourOffId, BW::PurpleGlow);
    saveBtn.onClick = [this]() { if (onSave) onSave(); };
    addAndMakeVisible(saveBtn);

    initBtn.setColour(juce::TextButton::buttonColourId, BW::PurpleDark);
    initBtn.setColour(juce::TextButton::textColourOffId, BW::Grey);
    initBtn.onClick = [this]() { if (onInit) onInit(); };
    addAndMakeVisible(initBtn);

    // --- Tab buttons ---
    const juce::String tabNames[] = { "MAIN", "FILTER", "EFFECTS", "AI ASSIST" };
    for (int i = 0; i < 4; ++i)
    {
        tabButtons[i].setButtonText(tabNames[i]);
        tabButtons[i].setClickingTogglesState(true);
        tabButtons[i].setRadioGroupId(1001);
        tabButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        tabButtons[i].setColour(juce::TextButton::textColourOffId, BW::TextMuted);
        tabButtons[i].setColour(juce::TextButton::textColourOnId, BW::Pink);
        tabButtons[i].onClick = [this, i]()
        {
            activeTab = i;
            if (onTabChanged) onTabChanged(i);
            repaint();
        };
        addAndMakeVisible(tabButtons[i]);
    }
    tabButtons[0].setToggleState(true, juce::dontSendNotification);

    // --- Driver (bass-mode) selector ---
    driverLabel.setText("DRIVER", juce::dontSendNotification);
    driverLabel.setFont(juce::Font(9.0f, juce::Font::bold).withExtraKerningFactor(0.18f));
    driverLabel.setColour(juce::Label::textColourId, BW::Pink);
    driverLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driverLabel);

    driverBox.addItem("PLUCK", 1);
    driverBox.addItem("808",   2);
    driverBox.addItem("REESE", 3);
    driverBox.setColour(juce::ComboBox::backgroundColourId, BW::Deep);
    driverBox.setColour(juce::ComboBox::textColourId, BW::White);
    driverBox.setColour(juce::ComboBox::outlineColourId, BW::Pink);
    driverBox.setJustificationType(juce::Justification::centred);
    driverAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::bassMode, driverBox);
    addAndMakeVisible(driverBox);

    // --- Level knob (compact) ---
    levelKnob.attach(apvts, ParamIDs::masterVolume);
    addAndMakeVisible(levelKnob);
}

void HeaderBar::setPresetName(const juce::String& name)
{
    presetNameBtn.setButtonText(name);
}

void HeaderBar::setActiveTab(int tabIndex)
{
    if (tabIndex >= 0 && tabIndex < 4)
    {
        activeTab = tabIndex;
        tabButtons[tabIndex].setToggleState(true, juce::dontSendNotification);
        repaint();
    }
}

void HeaderBar::setMeterLevels(float leftDB, float rightDB)
{
    meterL = leftDB;
    meterR = rightDB;
    repaint();
}

void HeaderBar::paint(juce::Graphics& g)
{
    // Background
    g.setColour(BW::Deep);
    g.fillRect(getLocalBounds());

    // Bottom border
    g.setColour(BW::Grey.withAlpha(0.4f));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);

    // Active tab underline
    for (int i = 0; i < 4; ++i)
    {
        if (tabButtons[i].getToggleState())
        {
            auto tabBounds = tabButtons[i].getBounds();
            g.setColour(BW::Pink);
            g.fillRect(tabBounds.getX(), tabBounds.getBottom() - 2,
                       tabBounds.getWidth(), 2);
        }
    }

    // Bolder box around the DRIVER selector to designate it as a driver
    if (!driverBoxBounds.isEmpty())
    {
        auto framed = driverBoxBounds.expanded(4);
        g.setColour(BW::Pink);
        g.drawRoundedRectangle(framed.toFloat(), 4.0f, 2.0f);
    }

    // --- Output meter (right side) ---
    auto meterArea = getLocalBounds().removeFromRight(30).reduced(4, 8);
    int meterW = 4;
    auto meterL_area = meterArea.removeFromLeft(meterW);
    meterArea.removeFromLeft(3);
    auto meterR_area = meterArea.removeFromLeft(meterW);

    auto drawMeter = [&](juce::Rectangle<int> area, float dB)
    {
        g.setColour(BW::Grey.withAlpha(0.3f));
        g.fillRoundedRectangle(area.toFloat(), 1.5f);

        float level = juce::jlimit(0.0f, 1.0f, (dB + 60.0f) / 60.0f);
        int filledH = static_cast<int>(area.getHeight() * level);
        auto filledArea = area.withTop(area.getBottom() - filledH);

        g.setColour(level > 0.85f ? BW::Pink : BW::PurpleGlow);
        g.fillRoundedRectangle(filledArea.toFloat(), 1.5f);
    };

    drawMeter(meterL_area, meterL);
    drawMeter(meterR_area, meterR);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds();

    // Reserve meter strip on the far right
    auto meterStrip = bounds.removeFromRight(30);
    juce::ignoreUnused(meterStrip);

    // Reserve right column for LEVEL knob — top-right, under SAVE
    auto levelColumn = bounds.removeFromRight(64);

    // Top row: title | preset nav | save/init
    auto topRow = bounds.removeFromTop(28).reduced(8, 2);
    // Tab row + DRIVER selector
    auto tabRow  = bounds.reduced(8, 0);

    titleLabel.setBounds(topRow.removeFromLeft(80));
    topRow.removeFromLeft(8);

    // Save/Init on far right of top row
    initBtn.setBounds(topRow.removeFromRight(40));
    topRow.removeFromRight(4);
    saveBtn.setBounds(topRow.removeFromRight(46));
    topRow.removeFromRight(12);

    // Preset nav in remaining centre
    prevBtn.setBounds(topRow.removeFromLeft(24));
    topRow.removeFromLeft(2);
    nextBtn.setBounds(topRow.removeFromRight(24));
    topRow.removeFromRight(2);
    presetNameBtn.setBounds(topRow);

    // LEVEL knob occupies the full right-side column (top-right under SAVE,
    // spanning the rest of the header height). BWKnob renders its own
    // label/knob/value internally.
    levelKnob.setBounds(levelColumn.reduced(4, 4));

    // Tab row: 4 tab buttons on the left, DRIVER selector on the right
    auto driverArea = tabRow.removeFromRight(110).reduced(8, 2);
    driverLabel.setBounds(driverArea.removeFromTop(10));
    driverBox.setBounds(driverArea);
    driverBoxBounds = driverBox.getBounds();

    int tabW = juce::jmin(90, tabRow.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
        tabButtons[i].setBounds(tabRow.removeFromLeft(tabW));
}
