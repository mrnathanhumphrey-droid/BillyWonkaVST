#include "PluginEditor.h"

GrooveEngineRnBAudioProcessorEditor::GrooveEngineRnBAudioProcessorEditor(
    GrooveEngineRnBAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      headerBar(p.getAPVTS()),
      mainTab(p.getAPVTS()),
      filterTab(p.getAPVTS()),
      settingsTab(p.getAPVTS()),
      aiAssistTab(p.getAPVTS()),
      keyboardComponent(p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard),
      mceClient(p.getAPVTS(), p.getMidiInjector())
{
    setLookAndFeel(&bwLookAndFeel);

    // --- Header ---
    headerBar.onTabChanged = [this](int idx) { showTab(idx); };
    headerBar.onPresetNav = [this](int dir) { loadPresetByIndex(currentPresetIndex + dir); };
    headerBar.onPresetBrowserOpen = [this]()
    {
        // Simple popup with preset list
        juce::PopupMenu menu;
        menu.addItem(1, "-- Init --");
        for (int i = 0; i < presetNames.size(); ++i)
            menu.addItem(i + 2, presetNames[i], true, i == currentPresetIndex);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(headerBar),
            [this](int result)
            {
                if (result == 1)
                {
                    currentPresetIndex = -1;
                    headerBar.setPresetName("-- Init --");
                }
                else if (result >= 2)
                    loadPresetByIndex(result - 2);
            });
    };
    headerBar.onInit = [this]()
    {
        currentPresetIndex = -1;
        headerBar.setPresetName("-- Init --");
    };
    addAndMakeVisible(headerBar);

    // --- Tabs ---
    addAndMakeVisible(mainTab);
    addChildComponent(filterTab);
    addChildComponent(settingsTab);
    addChildComponent(aiAssistTab);

    // --- Keyboard ---
    keyboardComponent.setKeyWidth(28.0f);
    keyboardComponent.setScrollButtonsVisible(true);
    keyboardComponent.setAvailableRange(24, 96);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, BW::Purple);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, BW::Deep.brighter(0.2f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId, BW::Black);
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, BW::Grey);
    addAndMakeVisible(keyboardComponent);

    // --- MCE Client ---
    // Each callback captures a Component::SafePointer to the editor so that
    // async lambdas queued by the MCEClient background thread cannot deref
    // a destroyed editor. Without this, ARM64 PAC catches the freed-vtable
    // access immediately (EXC_BAD_ACCESS); x86 silently rolls dice.
    mceClient.onMessageReceived = [safeThis = juce::Component::SafePointer<GrooveEngineRnBAudioProcessorEditor>(this)]
                                  (const juce::String& sender, const juce::String& text)
    {
        if (auto* self = safeThis.getComponent())
            self->aiAssistTab.addMessage(sender, text);
    };
    mceClient.onConnectionStatusChanged = [safeThis = juce::Component::SafePointer<GrooveEngineRnBAudioProcessorEditor>(this)]
                                          (bool conn, int lat)
    {
        if (auto* self = safeThis.getComponent())
            self->aiAssistTab.setConnectionStatus(conn, lat);
    };
    mceClient.onParamSuggestion = [safeThis = juce::Component::SafePointer<GrooveEngineRnBAudioProcessorEditor>(this)]
                                  (const juce::StringPairArray& params)
    {
        auto* self = safeThis.getComponent();
        if (self == nullptr) return;
        auto& apvts = self->processorRef.getAPVTS();
        for (auto& key : params.getAllKeys())
        {
            if (auto* param = apvts.getParameter(key))
            {
                float rawValue = params.getValue(key, "0").getFloatValue();
                float normalized = param->getNormalisableRange().convertTo0to1(rawValue);
                param->setValueNotifyingHost(normalized);
            }
        }
        self->aiAssistTab.addMessage("System", "Applied " + juce::String(params.size()) + " parameter changes.");
    };

    // Wire AIAssistTab callbacks to MCEClient
    aiAssistTab.onSendMessage = [this](const juce::String& msg)
    {
        mceClient.sendChatMessage(msg);
    };
    aiAssistTab.onApplySuggestion = [this]()
    {
        mceClient.requestApplySuggestion();
    };
    aiAssistTab.onPlayRequest = [this](const juce::String& desc)
    {
        mceClient.requestPlaySequence(desc);
    };
    aiAssistTab.onStopRequest = [this]()
    {
        processorRef.stopAIPlayback();
    };

    // Wire reconnect button in AIAssistTab
    // The reconnectBtn onClick needs to be set from here since MCEClient is owned by editor
    // We'll trigger connect via a lambda after the URL field changes
    mceClient.connect();

    // --- Presets ---
    scanPresets();

    setSize(900, 680);
    setResizable(true, true);
    setResizeLimits(700, 500, 1400, 1000);

    startTimerHz(30); // UI refresh for meters
}

GrooveEngineRnBAudioProcessorEditor::~GrooveEngineRnBAudioProcessorEditor()
{
    // Order matters here. The MCEClient background thread can queue async
    // lambdas via juce::MessageManager::callAsync up until the very moment
    // its thread is joined. If we just let normal member destruction run,
    // a queued lambda may fire after aiAssistTab (or other editor members)
    // has been destroyed, dereferencing freed memory.
    //
    // On ARM64 with pointer authentication this is an instant SIGSEGV
    // (EXC_BAD_ACCESS with PAC failure subtype). On x86 it tends to silently
    // hit garbage and sometimes "work." This destructor sequencing — plus
    // the SafePointer captures in the lambdas above — covers both layers.

    // 1. Null the std::function callbacks so any in-flight lambda noops
    //    when it checks `if (onMessageReceived)` at fire time.
    mceClient.onMessageReceived = nullptr;
    mceClient.onConnectionStatusChanged = nullptr;
    mceClient.onParamSuggestion = nullptr;

    // 2. Synchronously join the MCEClient background thread. After this,
    //    no new callAsync will be scheduled by the worker. Any already-
    //    queued callAsync that fires later will hit the WeakReference
    //    guards in MCEClient.cpp (which protect against MCEClient
    //    destruction) and the SafePointer guards in this file's lambdas
    //    (which protect against editor destruction).
    mceClient.disconnect();

    stopTimer();
    setLookAndFeel(nullptr);
}

void GrooveEngineRnBAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(BW::Black);
}

void GrooveEngineRnBAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Header: taller now that it carries DRIVER selector + LEVEL knob
    headerBar.setBounds(bounds.removeFromTop(80));

    // Keyboard: 70px at bottom
    keyboardComponent.setBounds(bounds.removeFromBottom(70));

    // Tab content fills middle
    mainTab.setBounds(bounds);
    filterTab.setBounds(bounds);
    settingsTab.setBounds(bounds);
    aiAssistTab.setBounds(bounds);
}

void GrooveEngineRnBAudioProcessorEditor::showTab(int index)
{
    currentTab = index;
    mainTab.setVisible(index == 0);
    filterTab.setVisible(index == 1);
    settingsTab.setVisible(index == 2);
    aiAssistTab.setVisible(index == 3);
}

void GrooveEngineRnBAudioProcessorEditor::timerCallback()
{
    // Could add peak meter levels here in future
    // headerBar.setMeterLevels(leftDB, rightDB);
}

// =============================================================================
// Preset Management
// =============================================================================

void GrooveEngineRnBAudioProcessorEditor::scanPresets()
{
    // Search multiple locations for the Factory presets folder
    juce::Array<juce::File> searchPaths;

    // 1. macOS: ~/Library/Application Support/BW BASS/Presets/Factory (install.sh target)
    auto appSupport = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("BW BASS")
                          .getChildFile("Presets")
                          .getChildFile("Factory");
    searchPaths.add(appSupport);

    // 2. Windows: %APPDATA%/BW BASS/Presets/Factory (Install.bat target)
    // (same as above on Windows — userApplicationDataDirectory maps to %APPDATA%)

    // 3. Relative to executable (standalone dev builds)
    auto exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    searchPaths.add(exeFile.getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getChildFile("Presets")
                           .getChildFile("Factory"));

    // 4. Windows dev fallback
    searchPaths.add(juce::File("C:\\GrooveEngineRnB\\Presets\\Factory"));

    // 5. macOS: next to the .vst3/.component bundle
    searchPaths.add(exeFile.getParentDirectory()
                           .getParentDirectory()
                           .getParentDirectory()
                           .getChildFile("Presets")
                           .getChildFile("Factory"));

    // Use the first directory that exists
    juce::File presetsDir;
    for (auto& path : searchPaths)
    {
        if (path.isDirectory())
        {
            presetsDir = path;
            break;
        }
    }

    if (presetsDir.isDirectory())
    {
        auto files = presetsDir.findChildFiles(juce::File::findFiles, false, "*.xml");
        files.sort();

        for (auto& f : files)
        {
            presetFiles.add(f);
            presetNames.add(f.getFileNameWithoutExtension());
        }
    }
}

void GrooveEngineRnBAudioProcessorEditor::loadPresetByIndex(int index)
{
    if (index < 0 || index >= presetFiles.size()) return;

    auto file = presetFiles[index];
    auto xml = juce::XmlDocument::parse(file.loadFileAsString());

    if (xml == nullptr || !xml->hasTagName("GrooveEngineRnBState"))
        return;

    auto& apvts = processorRef.getAPVTS();

    for (auto* paramElement : xml->getChildIterator())
    {
        if (paramElement->hasTagName("PARAM"))
        {
            auto id = paramElement->getStringAttribute("id");
            auto value = paramElement->getDoubleAttribute("value");

            if (auto* param = apvts.getParameter(id))
            {
                auto range = param->getNormalisableRange();
                float normalizedValue = range.convertTo0to1(static_cast<float>(value));
                param->setValueNotifyingHost(normalizedValue);
            }
        }
    }

    currentPresetIndex = index;
    headerBar.setPresetName(presetNames[index]);
}
