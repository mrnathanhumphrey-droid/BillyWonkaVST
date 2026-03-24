#pragma once
#include "OscillatorBank.h"

/**
 * Mixer Section.
 *
 * Sums the outputs from all three oscillators and the noise generator,
 * each with independent level control. Also implements the Minimoog's
 * legendary feedback loop: a portion of the final output is fed back
 * into the mixer input with a 1-sample delay, creating overdrive
 * from mild warmth to full saturation.
 *
 * Signal flow:
 *   (OSC1 * level1) + (OSC2 * level2) + (OSC3 * level3)
 *   + (Noise * noiseLevel) + (feedback * feedbackAmount)
 *   → mixed output → to filter
 */
class MixerSection
{
public:
    MixerSection();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Mix all oscillator outputs with levels and feedback.
     *
     * @param oscOut     Individual oscillator outputs from OscillatorBank.
     * @param feedback   The feedback sample from the previous block's output.
     * @return           The mixed signal ready for the filter.
     */
    float process(const OscillatorBank::OscOutput& oscOut, float feedback);

    void setOsc1Level(float level);
    void setOsc2Level(float level);
    void setOsc3Level(float level);
    void setNoiseLevel(float level);
    void setFeedbackAmount(float amount);

private:
    float osc1Level = 0.8f;
    float osc2Level = 0.0f;
    float osc3Level = 0.0f;
    float noiseLevel = 0.0f;
    float feedbackAmount = 0.0f;
};
