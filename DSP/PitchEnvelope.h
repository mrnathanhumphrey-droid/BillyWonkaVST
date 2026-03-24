#pragma once
#include <cmath>

/**
 * Pitch Envelope — AD (Attack-Decay) envelope for pitch transients.
 *
 * Used for:
 * - 808 punch: fast upward pitch transient (+4 to +12 semitones, ~5-20ms)
 * - Pluck attack: subtle pitch rise (+2 to +5 semitones, ~30-50ms)
 *
 * The envelope outputs a value in [0, 1] that is scaled by the amount
 * parameter (in semitones) and converted to a frequency multiplier:
 *   freqMultiplier = pow(2.0, envelope_output * amount / 12.0)
 *
 * Uses exponential decay for natural-sounding pitch fall.
 */
class PitchEnvelope
{
public:
    PitchEnvelope();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Advance the envelope and return the current frequency multiplier.
     * Returns 1.0 when inactive (no pitch shift).
     */
    float tick();

    /** Trigger the pitch envelope (note on). */
    void trigger();

    /** Set the pitch amount in semitones (0 to 24). */
    void setAmount(float semitones);

    /** Set the decay time in seconds (0.001 to 0.5). */
    void setDecayTime(float seconds);

    /** Check if the envelope is still active. */
    bool isActive() const { return active; }

private:
    double sampleRate = 44100.0;

    float amount = 0.0f;        // Pitch offset in semitones
    float decayTime = 0.05f;    // Decay time in seconds
    float decayCoeff = 0.0f;    // Exponential decay coefficient

    float currentLevel = 0.0f;  // Current envelope value [0, 1]
    bool active = false;

    static constexpr float LOG_9 = 2.19722457734f;
    static constexpr float SILENCE_THRESHOLD = 0.001f;
};
