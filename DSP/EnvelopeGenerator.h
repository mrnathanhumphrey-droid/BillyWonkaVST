#pragma once
#include <cmath>

/**
 * Exponential ADSR Envelope Generator.
 *
 * Uses time-constant (RC circuit) calculation for musically natural
 * exponential curves, matching the analog envelope behavior of the Minimoog.
 *
 * The coefficient for each stage is calculated as:
 *   coefficient = exp(-log(9) / (time_seconds * sampleRate))
 *
 * This produces a curve that reaches ~99% of its target in the specified time,
 * matching the charging/discharging behavior of a capacitor through a resistor.
 *
 * State machine: IDLE → ATTACK → DECAY → SUSTAIN → RELEASE → IDLE
 *
 * Velocity sensitivity scales the envelope's peak amplitude.
 */
class EnvelopeGenerator
{
public:
    enum class State
    {
        Idle = 0,
        Attack,
        Decay,
        Sustain,
        Release
    };

    EnvelopeGenerator();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /** Advance the envelope by one sample and return the current level [0, 1]. */
    float tick();

    /** Trigger the envelope (note on). Velocity in range [0, 1]. */
    void noteOn(float velocity = 1.0f);

    /** Release the envelope (note off). */
    void noteOff();

    /** Force immediate return to idle (for hard note cuts). */
    void forceIdle();

    // --- Parameter setters (times in seconds) ---
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);     // 0.0 to 1.0
    void setRelease(float seconds);

    /** Get the current envelope state. */
    State getState() const { return state; }

    /** Get the current output level. */
    float getLevel() const { return currentLevel; }

    /** Check if envelope is active (not idle). */
    bool isActive() const { return state != State::Idle; }

private:
    /** Compute the exponential coefficient for a given time in seconds. */
    float computeCoefficient(float timeSeconds) const;

    static constexpr float MIN_TIME = 0.0005f;   // 0.5ms minimum to avoid clicks
    static constexpr float LOG_9 = 2.19722457734f; // ln(9) — time to reach ~99%

    double sampleRate = 44100.0;
    State state = State::Idle;

    float currentLevel = 0.0f;
    float velocityScale = 1.0f;

    // Parameter values (in seconds, except sustain which is a level)
    float attackTime = 0.001f;
    float decayTime = 0.3f;
    float sustainLevel = 0.0f;
    float releaseTime = 0.1f;

    // Precomputed coefficients for each stage
    float attackCoeff = 0.0f;
    float decayCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Target overshoot values for exponential curves:
    // Attack goes slightly above 1.0 so the exponential curve actually reaches 1.0
    // before transitioning to decay. Without this, an exponential approach to 1.0
    // would take infinite time.
    static constexpr float ATTACK_TARGET = 1.2f;
    static constexpr float ATTACK_THRESHOLD = 1.0f;
};
