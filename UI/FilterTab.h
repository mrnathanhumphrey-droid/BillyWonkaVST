#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWKnob.h"
#include "ADSRDisplay.h"
#include "BWColours.h"

/**
 * FilterTab — Tab 2: Filter controls, filter envelope, LFO.
 */
class FilterTab : public juce::Component
{
public:
    FilterTab(juce::AudioProcessorValueTreeState& apvts);

    void resized() override;
    void paint(juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& apvts;

    // Main filter controls
    BWKnob knobCutoff    { "CUTOFF",    "Hz" };
    BWKnob knobResonance { "RESONANCE", ""   };
    BWKnob knobEnvAmount { "ENV AMT",   ""   };
    BWKnob knobKeyTrack  { "KEYTRACK",  ""   };
    BWKnob knobFilterSat { "DRIVE",     ""   };

    // Filter slope selector
    juce::ComboBox slopeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slopeAttach;

    // Filter envelope
    ADSRDisplay filterAdsrDisplay;
    BWKnob knobFAttack   { "A", "s"  };
    BWKnob knobFDecay    { "D", "s"  };
    BWKnob knobFSustain  { "S", "%"  };
    BWKnob knobFRelease  { "R", "s"  };

    // LFO section
    BWKnob knobLFORate   { "RATE",  "Hz" };
    BWKnob knobLFODepth  { "DEPTH", ""   };
    juce::ComboBox lfoShapeBox;
    juce::ComboBox lfoTargetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoShapeAttach, lfoTargetAttach;
    juce::ToggleButton lfoSyncToggle { "SYNC" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lfoSyncAttach;

    // Reverb section
    juce::ToggleButton reverbOnToggle { "REVERB" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbOnAttach;
    juce::ComboBox reverbModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbModeAttach;
    BWKnob knobReverbMix   { "MIX",   "%" };
    BWKnob knobReverbDecay { "DECAY", ""  };
    BWKnob knobReverbTone  { "TONE",  ""  };

    // Labels
    juce::Label filterLabel, filterEnvLabel, lfoLabel, reverbLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterTab)
};
