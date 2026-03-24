#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "BWColours.h"

/**
 * ADSRDisplay — Interactive ADSR envelope visualizer.
 *
 * Draws the envelope curve in BW::Pink on BW::Deep background.
 * Listens to 4 APVTS parameters and updates in real time.
 * Draggable handles for A, D, S, R nodes.
 */
class ADSRDisplay : public juce::Component,
                    public juce::AudioProcessorValueTreeState::Listener
{
public:
    ADSRDisplay();
    ~ADSRDisplay() override;

    void attachToAPVTS(juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& attackID, const juce::String& decayID,
                       const juce::String& sustainID, const juce::String& releaseID);

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    juce::Point<float> getAttackPoint() const;
    juce::Point<float> getDecayPoint() const;
    juce::Point<float> getSustainPoint() const;
    juce::Point<float> getReleasePoint() const;

    float attack = 0.01f, decay = 0.3f, sustain = 0.5f, release = 0.2f;
    float maxTime = 5.0f; // Max axis time in seconds

    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    juce::String attackID, decayID, sustainID, releaseID;

    enum DragTarget { None, Attack, Decay, Sustain, Release };
    DragTarget currentDrag = None;

    juce::Rectangle<float> drawArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ADSRDisplay)
};
