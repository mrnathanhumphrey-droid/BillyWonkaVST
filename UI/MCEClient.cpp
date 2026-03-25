#include "MCEClient.h"

MCEClient::MCEClient(juce::AudioProcessorValueTreeState& vts, MidiInjector& injector)
    : juce::Thread("MCEClient"), apvts(vts), midiInjector(injector)
{
}

MCEClient::~MCEClient()
{
    disconnect();
}

void MCEClient::setServerURL(const juce::String& url)
{
    serverURL = url.trimCharactersAtEnd("/");
}

void MCEClient::connect()
{
    if (!isThreadRunning())
        startThread(juce::Thread::Priority::low);

    // Start health check polling every 5 seconds
    startTimer(5000);

    // Queue an immediate health check
    {
        juce::ScopedLock lock(queueLock);
        requestQueue.push_back({ Request::Health, {} });
    }
    notify();
}

void MCEClient::disconnect()
{
    stopTimer();
    signalThreadShouldExit();
    notify();
    stopThread(2000);

    connected.store(false);
    if (onConnectionStatusChanged)
        juce::MessageManager::callAsync([this]() {
            if (onConnectionStatusChanged)
                onConnectionStatusChanged(false, 0);
        });
}

void MCEClient::sendChatMessage(const juce::String& message)
{
    // Build JSON payload with message + param context
    auto context = buildParamContext();

    juce::String json = "{"
        "\"message\":" + juce::JSON::toString(message) + ","
        "\"context\":" + context +
    "}";

    {
        juce::ScopedLock lock(queueLock);
        requestQueue.push_back({ Request::Chat, json });
    }
    notify();
}

void MCEClient::requestApplySuggestion()
{
    auto context = buildParamContext();

    juce::String json = "{"
        "\"action\":\"suggest_params\","
        "\"context\":" + context +
    "}";

    {
        juce::ScopedLock lock(queueLock);
        requestQueue.push_back({ Request::Apply, json });
    }
    notify();
}

void MCEClient::requestPlaySequence(const juce::String& description)
{
    auto context = buildParamContext();

    juce::String json = "{"
        "\"action\":\"play\","
        "\"description\":" + juce::JSON::toString(description) + ","
        "\"context\":" + context +
    "}";

    {
        juce::ScopedLock lock(queueLock);
        requestQueue.push_back({ Request::Play, json });
    }
    notify();
}

// =============================================================================
// Background Thread
// =============================================================================

void MCEClient::run()
{
    while (!threadShouldExit())
    {
        // Grab next request from queue
        Request req;
        bool hasRequest = false;

        {
            juce::ScopedLock lock(queueLock);
            if (!requestQueue.empty())
            {
                req = requestQueue.front();
                requestQueue.erase(requestQueue.begin());
                hasRequest = true;
            }
        }

        if (hasRequest)
        {
            switch (req.type)
            {
                case Request::Chat:
                {
                    auto response = httpPost("/chat", req.payload);
                    handleChatResponse(response);
                    break;
                }
                case Request::Apply:
                {
                    auto response = httpPost("/apply", req.payload);
                    handleApplyResponse(response);
                    break;
                }
                case Request::Play:
                {
                    auto response = httpPost("/play", req.payload);
                    handlePlayResponse(response);
                    break;
                }
                case Request::Health:
                {
                    auto startTime = juce::Time::getMillisecondCounterHiRes();
                    auto response = httpGet("/health");
                    auto elapsed = static_cast<int64_t>(
                        juce::Time::getMillisecondCounterHiRes() - startTime);
                    handleHealthResponse(response, elapsed);
                    break;
                }
            }
        }
        else
        {
            // Wait for new requests (with timeout for clean shutdown)
            wait(500);
        }
    }
}

void MCEClient::timerCallback()
{
    // Queue periodic health check
    juce::ScopedLock lock(queueLock);
    requestQueue.push_back({ Request::Health, {} });
    notify();
}

// =============================================================================
// HTTP Helpers
// =============================================================================

juce::String MCEClient::httpPost(const juce::String& endpoint, const juce::String& jsonBody)
{
    juce::URL url = juce::URL(serverURL + endpoint).withPOSTData(jsonBody);

    auto stream = url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
            .withExtraHeaders("Content-Type: application/json")
            .withConnectionTimeoutMs(30000));

    if (stream == nullptr)
        return {};

    return stream->readEntireStreamAsString();
}

juce::String MCEClient::httpGet(const juce::String& endpoint)
{
    juce::URL url(serverURL + endpoint);

    auto stream = url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withConnectionTimeoutMs(3000));

    if (stream == nullptr)
        return {};

    return stream->readEntireStreamAsString();
}

// =============================================================================
// Response Handlers
// =============================================================================

void MCEClient::handleChatResponse(const juce::String& responseBody)
{
    if (responseBody.isEmpty())
    {
        juce::MessageManager::callAsync([this]() {
            if (onMessageReceived)
                onMessageReceived("System", "No response from MCE server. Is it running?");
        });
        return;
    }

    auto json = juce::JSON::parse(responseBody);

    if (json.isObject())
    {
        auto message = json.getProperty("message", "").toString();
        auto sender = json.getProperty("sender", "Claude").toString();
        auto hasParams = json.getProperty("has_params", false);

        juce::MessageManager::callAsync([this, sender, message, hasParams]() {
            if (onMessageReceived)
                onMessageReceived(sender, message);
        });
    }
    else
    {
        // Treat raw text as Claude response
        juce::MessageManager::callAsync([this, responseBody]() {
            if (onMessageReceived)
                onMessageReceived("Claude", responseBody);
        });
    }
}

void MCEClient::handleApplyResponse(const juce::String& responseBody)
{
    if (responseBody.isEmpty())
        return;

    auto json = juce::JSON::parse(responseBody);

    if (json.isObject())
    {
        auto params = json.getProperty("params", juce::var());

        if (params.isObject())
        {
            juce::StringPairArray paramPairs;
            auto* obj = params.getDynamicObject();

            if (obj != nullptr)
            {
                for (auto& prop : obj->getProperties())
                    paramPairs.set(prop.name.toString(), prop.value.toString());
            }

            juce::MessageManager::callAsync([this, paramPairs]() {
                if (onParamSuggestion)
                    onParamSuggestion(paramPairs);
            });
        }

        auto message = json.getProperty("message", "").toString();
        if (message.isNotEmpty())
        {
            juce::MessageManager::callAsync([this, message]() {
                if (onMessageReceived)
                    onMessageReceived("Claude", message);
            });
        }
    }
}

void MCEClient::handlePlayResponse(const juce::String& responseBody)
{
    if (responseBody.isEmpty())
    {
        juce::MessageManager::callAsync([this]() {
            if (onMessageReceived)
                onMessageReceived("System", "No play response from MCE server.");
        });
        return;
    }

    auto json = juce::JSON::parse(responseBody);

    if (json.isObject())
    {
        auto notes = json.getProperty("notes", juce::var());
        auto message = json.getProperty("message", "").toString();

        if (notes.isArray())
        {
            auto* arr = notes.getArray();
            if (arr != nullptr && arr->size() > 0)
            {
                midiInjector.clear();

                // Track which notes are on so we can force note-offs
                int lastTimeMs = 0;
                bool noteIsOn[128] = {};

                for (auto& noteVar : *arr)
                {
                    if (noteVar.isObject())
                    {
                        MidiInjector::MidiEvent evt;
                        evt.note = static_cast<uint8_t>(
                            static_cast<int>(noteVar.getProperty("note", 60)));
                        evt.velocity = static_cast<uint8_t>(
                            static_cast<int>(noteVar.getProperty("vel", 100)));
                        evt.deltaMs = static_cast<uint16_t>(
                            static_cast<int>(noteVar.getProperty("time", 0)));
                        midiInjector.push(evt);

                        if (evt.deltaMs > static_cast<uint16_t>(lastTimeMs))
                            lastTimeMs = evt.deltaMs;

                        if (evt.velocity > 0)
                            noteIsOn[evt.note & 127] = true;
                        else
                            noteIsOn[evt.note & 127] = false;
                    }
                }

                // Safety: append note-off for any note still on at the end
                uint16_t safetyTime = static_cast<uint16_t>(
                    std::min(lastTimeMs + 200, 65535));
                for (int n = 0; n < 128; ++n)
                {
                    if (noteIsOn[n])
                    {
                        MidiInjector::MidiEvent off;
                        off.note = static_cast<uint8_t>(n);
                        off.velocity = 0;
                        off.deltaMs = safetyTime;
                        midiInjector.push(off);
                    }
                }

                // Begin playback — audio thread will stage all events from
                // the FIFO, then consume them per-block based on timing.
                // Do NOT call endSequence() here — the audio thread will
                // detect completion when stagedIndex >= stagedCount.
                midiInjector.beginSequence();

                int noteCount = arr->size();
                juce::MessageManager::callAsync([this, noteCount, message]() {
                    if (onMessageReceived)
                    {
                        juce::String msg = "Playing " + juce::String(noteCount / 2) + " notes...";
                        if (message.isNotEmpty())
                            msg = message + "\n" + msg;
                        onMessageReceived("Claude", msg);
                    }
                });
            }
        }

        if (message.isNotEmpty() && (notes.isVoid() || !notes.isArray()))
        {
            juce::MessageManager::callAsync([this, message]() {
                if (onMessageReceived)
                    onMessageReceived("Claude", message);
            });
        }
    }
}

void MCEClient::handleHealthResponse(const juce::String& responseBody, int64_t elapsedMs)
{
    bool wasConnected = connected.load();
    bool nowConnected = responseBody.isNotEmpty();

    connected.store(nowConnected);
    latencyMs.store(static_cast<int>(elapsedMs));

    if (nowConnected != wasConnected || nowConnected)
    {
        int lat = static_cast<int>(elapsedMs);
        juce::MessageManager::callAsync([this, nowConnected, lat]() {
            if (onConnectionStatusChanged)
                onConnectionStatusChanged(nowConnected, lat);
        });
    }
}

// =============================================================================
// Parameter Context Builder
// =============================================================================

juce::String MCEClient::buildParamContext()
{
    // Build a simple JSON object of all param values
    juce::String json = "{";
    bool first = true;

    auto& params = apvts.processor.getParameters();

    for (int i = 0; i < params.size(); ++i)
    {
        if (auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(params[i]))
        {
            if (!first) json += ",";
            first = false;

            auto id = rangedParam->getParameterID();
            auto value = rangedParam->getValue();
            auto denorm = rangedParam->getNormalisableRange()
                .convertFrom0to1(value);

            json += "\"" + id + "\":" + juce::String(denorm, 4);
        }
    }

    json += "}";
    return json;
}
