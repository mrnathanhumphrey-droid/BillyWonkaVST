#include "EnvelopeGenerator.h"
#include <algorithm>

EnvelopeGenerator::EnvelopeGenerator() = default;

void EnvelopeGenerator::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;

    // Recompute coefficients with new sample rate
    attackCoeff = computeCoefficient(attackTime);
    decayCoeff = computeCoefficient(decayTime);
    releaseCoeff = computeCoefficient(releaseTime);
}

void EnvelopeGenerator::reset()
{
    state = State::Idle;
    currentLevel = 0.0f;
}

float EnvelopeGenerator::computeCoefficient(float timeSeconds) const
{
    // Clamp to minimum time to prevent division issues and clicks
    float t = std::max(MIN_TIME, timeSeconds);

    // Exponential coefficient: how much of the remaining distance
    // to the target is covered each sample.
    // exp(-ln(9) / (time * sampleRate)) gives ~99% in 'time' seconds.
    return std::exp(static_cast<float>(-LOG_9 / (static_cast<double>(t) * sampleRate)));
}

void EnvelopeGenerator::setAttack(float seconds)
{
    attackTime = seconds;
    attackCoeff = computeCoefficient(seconds);
}

void EnvelopeGenerator::setDecay(float seconds)
{
    decayTime = seconds;
    decayCoeff = computeCoefficient(seconds);
}

void EnvelopeGenerator::setSustain(float level)
{
    sustainLevel = std::max(0.0f, std::min(1.0f, level));
}

void EnvelopeGenerator::setRelease(float seconds)
{
    releaseTime = seconds;
    releaseCoeff = computeCoefficient(seconds);
}

void EnvelopeGenerator::noteOn(float velocity)
{
    velocityScale = std::max(0.0f, std::min(1.0f, velocity));
    state = State::Attack;
    // Note: we do NOT reset currentLevel to 0 here.
    // This allows retriggering mid-envelope without a click —
    // the attack simply starts from wherever the level currently is.
}

void EnvelopeGenerator::noteOff()
{
    if (state != State::Idle)
        state = State::Release;
}

void EnvelopeGenerator::forceIdle()
{
    state = State::Idle;
    currentLevel = 0.0f;
}

float EnvelopeGenerator::tick()
{
    switch (state)
    {
        case State::Idle:
        {
            currentLevel = 0.0f;
            break;
        }

        case State::Attack:
        {
            // Exponential approach toward ATTACK_TARGET (>1.0).
            // Once we cross 1.0, transition to Decay.
            currentLevel = attackCoeff * currentLevel + (1.0f - attackCoeff) * ATTACK_TARGET;

            if (currentLevel >= ATTACK_THRESHOLD)
            {
                currentLevel = 1.0f;
                state = State::Decay;
            }
            break;
        }

        case State::Decay:
        {
            // Exponential decay toward sustain level
            float target = sustainLevel * velocityScale;
            currentLevel = decayCoeff * currentLevel + (1.0f - decayCoeff) * target;

            // Once we're close enough to sustain, transition
            float diff = currentLevel - target;
            if (diff < 0.001f && diff > -0.001f)
            {
                currentLevel = target;
                state = State::Sustain;
            }
            break;
        }

        case State::Sustain:
        {
            // Hold at sustain level (scaled by velocity)
            currentLevel = sustainLevel * velocityScale;
            break;
        }

        case State::Release:
        {
            // Exponential decay toward zero
            currentLevel = releaseCoeff * currentLevel;

            // Once sufficiently quiet, go idle
            if (currentLevel < 0.0001f)
            {
                currentLevel = 0.0f;
                state = State::Idle;
            }
            break;
        }
    }

    return currentLevel;
}
