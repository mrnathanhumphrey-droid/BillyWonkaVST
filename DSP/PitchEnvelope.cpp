#include "PitchEnvelope.h"
#include <algorithm>

PitchEnvelope::PitchEnvelope() = default;

void PitchEnvelope::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    // Recompute decay coefficient
    float t = std::max(0.001f, decayTime);
    decayCoeff = std::exp(static_cast<float>(-LOG_9 / (static_cast<double>(t) * sampleRate)));
}

void PitchEnvelope::reset()
{
    currentLevel = 0.0f;
    active = false;
}

void PitchEnvelope::setAmount(float semitones)
{
    amount = std::max(0.0f, std::min(24.0f, semitones));
}

void PitchEnvelope::setDecayTime(float seconds)
{
    decayTime = std::max(0.001f, std::min(0.5f, seconds));
    float t = decayTime;
    decayCoeff = std::exp(static_cast<float>(-LOG_9 / (static_cast<double>(t) * sampleRate)));
}

void PitchEnvelope::trigger()
{
    currentLevel = 1.0f;  // Start at maximum (full pitch offset)
    active = true;
}

float PitchEnvelope::tick()
{
    if (!active)
        return 1.0f;  // No pitch shift — multiplier of 1.0

    // Exponential decay toward zero
    currentLevel *= decayCoeff;

    if (currentLevel < SILENCE_THRESHOLD)
    {
        currentLevel = 0.0f;
        active = false;
        return 1.0f;
    }

    // Convert envelope level to frequency multiplier:
    // level=1.0 → shift by 'amount' semitones up
    // level=0.0 → no shift (multiplier = 1.0)
    float semitoneOffset = currentLevel * amount;
    float multiplier = std::pow(2.0f, semitoneOffset / 12.0f);
    return multiplier;
}
