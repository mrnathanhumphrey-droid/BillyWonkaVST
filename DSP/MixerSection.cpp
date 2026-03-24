#include "MixerSection.h"
#include <cmath>
#include <algorithm>

MixerSection::MixerSection() = default;

void MixerSection::prepare(double /*sampleRate*/, int /*blockSize*/)
{
    // No sample-rate-dependent state
}

void MixerSection::reset()
{
    // No internal state to reset (feedback delay is managed by SynthEngine)
}

void MixerSection::setOsc1Level(float level)   { osc1Level = std::max(0.0f, std::min(1.0f, level)); }
void MixerSection::setOsc2Level(float level)   { osc2Level = std::max(0.0f, std::min(1.0f, level)); }
void MixerSection::setOsc3Level(float level)   { osc3Level = std::max(0.0f, std::min(1.0f, level)); }
void MixerSection::setNoiseLevel(float level)  { noiseLevel = std::max(0.0f, std::min(1.0f, level)); }
void MixerSection::setFeedbackAmount(float amount) { feedbackAmount = std::max(0.0f, std::min(1.0f, amount)); }

float MixerSection::process(const OscillatorBank::OscOutput& oscOut, float feedback)
{
    float mixed = 0.0f;

    mixed += oscOut.osc1 * osc1Level;
    mixed += oscOut.osc2 * osc2Level;
    mixed += oscOut.osc3 * osc3Level;
    mixed += oscOut.noise * noiseLevel;

    // Feedback loop: add previous output with soft clipping.
    // The tanh prevents the feedback from blowing up while creating
    // the characteristic Minimoog overdrive warmth.
    if (feedbackAmount > 0.0001f)
    {
        float fb = feedback * feedbackAmount * 1.5f;  // Scale for useful range
        // Soft-clip the feedback to prevent runaway
        float clipped = std::tanh(fb);
        mixed += clipped;
    }

    return mixed;
}
