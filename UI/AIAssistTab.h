#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWColours.h"

/**
 * AIAssistTab — Tab 4: Claude AI integration via MCE server.
 *
 * Split view: left = chat, right = context panel + quick prompts + play config.
 */
class AIAssistTab : public juce::Component
{
public:
    AIAssistTab(juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint(juce::Graphics&) override;

    /** Add a message to the chat display. */
    void addMessage(const juce::String& sender, const juce::String& text);

    /** Set MCE connection status. */
    void setConnectionStatus(bool connected, int latencyMs = 0);

    /** Callback when user sends a chat message. */
    std::function<void(const juce::String&)> onSendMessage;

    /** Callback when "Apply Suggestion" is clicked. */
    std::function<void()> onApplySuggestion;

    /** Callback when play is requested (via button or chat trigger). */
    std::function<void(const juce::String&)> onPlayRequest;

    /** Callback when stop is pressed (MIDI panic — kill all notes). */
    std::function<void()> onStopRequest;

private:
    void sendCurrentMessage();
    juce::String buildPlayPrompt() const;
    bool isPlayTrigger(const juce::String& text) const;

    juce::AudioProcessorValueTreeState& apvts;

    // Chat area
    juce::TextEditor chatDisplay;
    juce::TextEditor inputField;
    juce::TextButton sendBtn { "Send" };

    // Connection status
    juce::Label statusLabel;
    bool isConnected = false;
    int latency = 0;

    // Context panel
    juce::Label contextTitle;
    juce::TextEditor contextDisplay;

    // Quick prompts
    juce::TextButton quickPrompts[5];
    static constexpr const char* promptTexts[5] = {
        "Suggest a preset for boom-bap bass",
        "How do I get a rubbery sub feel?",
        "Optimize this preset for R&B slow jams",
        "Explain what my current filter settings do",
        "Generate a new preset from scratch"
    };

    // --- Play configuration ---
    juce::Label playConfigLabel;
    juce::ComboBox keyBox;
    juce::ComboBox styleBox;
    juce::ComboBox timeSigBox;
    juce::ComboBox barsBox;
    juce::ComboBox bpmBox;
    juce::TextButton playBtn { "PLAY" };
    juce::TextButton stopBtn { "STOP" };

    // Apply suggestion
    juce::TextButton applyBtn { "Apply Suggestion" };

    // MCE server URL
    juce::Label urlLabel;
    juce::TextEditor urlField;
    juce::TextButton reconnectBtn { "Reconnect" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIAssistTab)
};
