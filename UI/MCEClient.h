#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MidiInjector.h"

/**
 * MCEClient — HTTP-based client for the MCE (Model Context Extension) server.
 *
 * Communicates with a local Node.js MCE server via REST endpoints:
 *   POST /chat     — send a user message + param context, get Claude response
 *   POST /apply    — tell server to generate param changes, returns JSON params
 *   GET  /health   — check connection + latency
 *
 * Runs all network I/O on a background thread (juce::Thread).
 * Delivers results to the message thread via juce::MessageManager::callAsync.
 *
 * CRITICAL: No DSP code, no audio thread interaction. Reads APVTS for context only.
 */
class MCEClient : public juce::Thread,
                  public juce::Timer
{
public:
    MCEClient(juce::AudioProcessorValueTreeState& apvts, MidiInjector& injector);
    ~MCEClient() override;

    /** Set the MCE server base URL (e.g. "http://localhost:9150"). */
    void setServerURL(const juce::String& url);

    /** Connect to the server (starts health check polling). */
    void connect();

    /** Disconnect from the server. */
    void disconnect();

    /** Send a chat message to Claude via the MCE server. */
    void sendChatMessage(const juce::String& message);

    /** Request Claude to generate parameter suggestions for the current state. */
    void requestApplySuggestion();

    /** Request Claude to generate and play a note sequence. */
    void requestPlaySequence(const juce::String& description);

    /** Check if connected. Thread-safe. */
    bool isConnected() const { return connected.load(); }

    /** Get last measured latency in ms. */
    int getLatencyMs() const { return latencyMs.load(); }

    // --- Callbacks (called on message thread) ---
    std::function<void(const juce::String& sender, const juce::String& text)> onMessageReceived;
    std::function<void(bool connected, int latencyMs)> onConnectionStatusChanged;
    std::function<void(const juce::StringPairArray& params)> onParamSuggestion;

private:
    void run() override;           // Background thread: processes request queue
    void timerCallback() override;  // Health check polling (every 5s)

    /** Build a JSON string of current parameter values for context. */
    juce::String buildParamContext();

    /** Perform an HTTP POST and return the response body. */
    juce::String httpPost(const juce::String& endpoint, const juce::String& jsonBody);

    /** Perform an HTTP GET and return the response body. */
    juce::String httpGet(const juce::String& endpoint);

    /** Parse a JSON response and dispatch callbacks. */
    void handleChatResponse(const juce::String& responseBody);
    void handleApplyResponse(const juce::String& responseBody);
    void handlePlayResponse(const juce::String& responseBody);
    void handleHealthResponse(const juce::String& responseBody, int64_t elapsedMs);

    juce::AudioProcessorValueTreeState& apvts;
    MidiInjector& midiInjector;

    juce::String serverURL { "http://localhost:9150" };
    std::atomic<bool> connected { false };
    std::atomic<int> latencyMs { 0 };

    // Thread-safe request queue
    struct Request
    {
        enum Type { Chat, Apply, Play, Health };
        Type type;
        juce::String payload;
    };

    juce::CriticalSection queueLock;
    std::vector<Request> requestQueue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MCEClient)
};
