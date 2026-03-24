#include "AIAssistTab.h"

AIAssistTab::AIAssistTab(juce::AudioProcessorValueTreeState& vts) : apvts(vts)
{
    // --- Chat display ---
    chatDisplay.setMultiLine(true);
    chatDisplay.setReadOnly(true);
    chatDisplay.setScrollbarsShown(true);
    chatDisplay.setColour(juce::TextEditor::backgroundColourId, BW::Deep);
    chatDisplay.setColour(juce::TextEditor::textColourId, BW::White);
    chatDisplay.setColour(juce::TextEditor::outlineColourId, BW::GreyBorder);
    chatDisplay.setFont(juce::Font(13.0f));
    addAndMakeVisible(chatDisplay);

    // --- Input field ---
    inputField.setMultiLine(false);
    inputField.setReturnKeyStartsNewLine(false);
    inputField.setColour(juce::TextEditor::backgroundColourId, BW::Deep);
    inputField.setColour(juce::TextEditor::textColourId, BW::White);
    inputField.setColour(juce::TextEditor::outlineColourId, BW::Grey);
    inputField.setColour(juce::TextEditor::focusedOutlineColourId, BW::Purple);
    inputField.setTextToShowWhenEmpty("Ask Claude about your bass sound...", BW::TextMuted);
    inputField.onReturnKey = [this]() { sendCurrentMessage(); };
    addAndMakeVisible(inputField);

    // --- Send button ---
    sendBtn.setColour(juce::TextButton::buttonColourId, BW::Pink);
    sendBtn.setColour(juce::TextButton::textColourOffId, BW::White);
    sendBtn.onClick = [this]() { sendCurrentMessage(); };
    addAndMakeVisible(sendBtn);

    // --- Connection status ---
    statusLabel.setFont(juce::Font(11.0f));
    statusLabel.setColour(juce::Label::textColourId, BW::Grey);
    statusLabel.setText("MCE: Offline", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    // --- Context panel ---
    contextTitle.setText("CURRENT PRESET CONTEXT", juce::dontSendNotification);
    contextTitle.setFont(juce::Font(10.0f, juce::Font::bold).withExtraKerningFactor(0.15f));
    contextTitle.setColour(juce::Label::textColourId, BW::TextMuted);
    addAndMakeVisible(contextTitle);

    contextDisplay.setMultiLine(true);
    contextDisplay.setReadOnly(true);
    contextDisplay.setColour(juce::TextEditor::backgroundColourId, BW::PurpleDark.withAlpha(0.5f));
    contextDisplay.setColour(juce::TextEditor::textColourId, BW::PinkSoft);
    contextDisplay.setColour(juce::TextEditor::outlineColourId, BW::GreyBorder);
    contextDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f,
                                      juce::Font::plain));
    addAndMakeVisible(contextDisplay);

    // --- Quick prompts ---
    for (int i = 0; i < 5; ++i)
    {
        quickPrompts[i].setButtonText(promptTexts[i]);
        quickPrompts[i].setColour(juce::TextButton::buttonColourId, BW::PurpleDark);
        quickPrompts[i].setColour(juce::TextButton::textColourOffId, BW::PinkSoft);
        quickPrompts[i].onClick = [this, i]()
        {
            inputField.setText(promptTexts[i]);
            sendCurrentMessage();
        };
        addAndMakeVisible(quickPrompts[i]);
    }

    // --- Apply suggestion ---
    applyBtn.setColour(juce::TextButton::buttonColourId, BW::Purple);
    applyBtn.setColour(juce::TextButton::textColourOffId, BW::White);
    applyBtn.setEnabled(false);
    applyBtn.onClick = [this]() { if (onApplySuggestion) onApplySuggestion(); };
    addAndMakeVisible(applyBtn);

    // --- MCE URL ---
    urlLabel.setText("MCE Server:", juce::dontSendNotification);
    urlLabel.setFont(juce::Font(10.0f));
    urlLabel.setColour(juce::Label::textColourId, BW::TextMuted);
    addAndMakeVisible(urlLabel);

    urlField.setText("ws://localhost:9150");
    urlField.setColour(juce::TextEditor::backgroundColourId, BW::Deep);
    urlField.setColour(juce::TextEditor::textColourId, BW::White);
    urlField.setColour(juce::TextEditor::outlineColourId, BW::Grey);
    urlField.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 10.0f,
                                 juce::Font::plain));
    addAndMakeVisible(urlField);

    reconnectBtn.setColour(juce::TextButton::buttonColourId, BW::PurpleDark);
    reconnectBtn.setColour(juce::TextButton::textColourOffId, BW::PurpleGlow);
    addAndMakeVisible(reconnectBtn);

    // Initial context
    addMessage("System", "Claude AI assistant for BW BASS.\nConnect to an MCE server to begin.");
}

void AIAssistTab::sendCurrentMessage()
{
    auto text = inputField.getText().trim();
    if (text.isEmpty()) return;

    addMessage("You", text);
    inputField.clear();

    if (onSendMessage)
        onSendMessage(text);
    else
        addMessage("System", "MCE server not connected. Messages are local only.");
}

void AIAssistTab::addMessage(const juce::String& sender, const juce::String& text)
{
    auto existingText = chatDisplay.getText();
    if (existingText.isNotEmpty())
        existingText += "\n\n";

    existingText += "[" + sender + "]\n" + text;
    chatDisplay.setText(existingText);
    chatDisplay.moveCaretToEnd();
}

void AIAssistTab::setConnectionStatus(bool connected, int latMs)
{
    isConnected = connected;
    latency = latMs;

    juce::String statusText = connected
        ? "MCE: Connected (" + juce::String(latMs) + "ms)"
        : "MCE: Offline";

    statusLabel.setText(statusText, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, connected ? BW::Green : BW::Grey);
}

void AIAssistTab::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);

    // Divider between chat and context panel
    int divX = getWidth() * 6 / 10;
    g.setColour(BW::Grey.withAlpha(0.3f));
    g.fillRect(divX, 4, 1, getHeight() - 8);
}

void AIAssistTab::resized()
{
    auto bounds = getLocalBounds().reduced(8, 4);
    int dividerX = bounds.getWidth() * 6 / 10;

    // --- Left: Chat ---
    auto chatArea = bounds.removeFromLeft(dividerX - 4);
    bounds.removeFromLeft(8); // divider gap

    // Status bar at top
    statusLabel.setBounds(chatArea.removeFromTop(18));

    // Input row at bottom
    auto inputRow = chatArea.removeFromBottom(30);
    sendBtn.setBounds(inputRow.removeFromRight(60));
    inputRow.removeFromRight(4);
    inputField.setBounds(inputRow);

    chatArea.removeFromBottom(4);
    chatDisplay.setBounds(chatArea);

    // --- Right: Context panel ---
    auto rightPanel = bounds;

    contextTitle.setBounds(rightPanel.removeFromTop(16));
    contextDisplay.setBounds(rightPanel.removeFromTop(100).reduced(0, 2));
    rightPanel.removeFromTop(8);

    // Quick prompts
    for (int i = 0; i < 5; ++i)
        quickPrompts[i].setBounds(rightPanel.removeFromTop(28).reduced(0, 2));

    rightPanel.removeFromTop(8);
    applyBtn.setBounds(rightPanel.removeFromTop(30).reduced(0, 2));

    // Bottom: MCE URL
    auto urlRow = rightPanel.removeFromBottom(24);
    urlLabel.setBounds(urlRow.removeFromLeft(70));
    reconnectBtn.setBounds(urlRow.removeFromRight(80));
    urlRow.removeFromRight(4);
    urlField.setBounds(urlRow);
}
