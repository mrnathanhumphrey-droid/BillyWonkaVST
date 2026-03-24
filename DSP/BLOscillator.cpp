#include "BLOscillator.h"
#include <algorithm>

static constexpr double twoPi = 6.283185307179586476925286766559;

BLOscillator::BLOscillator() = default;

void BLOscillator::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    phaseIncrement = frequency / sampleRate;
}

void BLOscillator::reset()
{
    phase = 0.0;
    triangleState = 0.0f;
}

void BLOscillator::setFrequency(double freqHz)
{
    frequency = freqHz;
    phaseIncrement = frequency / sampleRate;
}

void BLOscillator::setWaveform(Waveform type)
{
    waveform = type;
}

void BLOscillator::setAmplitude(float amp)
{
    amplitude = amp;
}

void BLOscillator::setPulseWidth(float pw)
{
    pulseWidth = std::max(0.01f, std::min(0.99f, pw));
}

float BLOscillator::polyBLEP(float t) const
{
    // t is the normalized phase position relative to the discontinuity.
    // dt is the normalized frequency (phase increment per sample).
    const float dt = static_cast<float>(phaseIncrement);

    if (t < dt)
    {
        // Discontinuity is in the current sample
        float n = t / dt;  // 0 to 1
        return n + n - n * n - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
        // Discontinuity was in the previous sample
        float n = (t - 1.0f) / dt;  // -1 to 0
        return n * n + n + n + 1.0f;
    }

    return 0.0f;
}

float BLOscillator::tick()
{
    float sample = 0.0f;
    const float t = static_cast<float>(phase);
    const float dt = static_cast<float>(phaseIncrement);

    switch (waveform)
    {
        case Waveform::Sine:
        {
            sample = std::sin(t * static_cast<float>(twoPi));
            break;
        }

        case Waveform::Sawtooth:
        {
            // Naive sawtooth: rises from -1 to +1 over one period
            sample = 2.0f * t - 1.0f;
            // Apply PolyBLEP at the discontinuity (phase wrap at 1.0)
            sample -= polyBLEP(t);
            break;
        }

        case Waveform::ReverseSaw:
        {
            // Reverse sawtooth: falls from +1 to -1
            sample = 1.0f - 2.0f * t;
            sample += polyBLEP(t);
            break;
        }

        case Waveform::Square:
        {
            // Naive square: +1 for first half, -1 for second half
            sample = (t < 0.5f) ? 1.0f : -1.0f;

            // PolyBLEP at rising edge (phase = 0)
            sample += polyBLEP(t);

            // PolyBLEP at falling edge (phase = 0.5)
            float t2 = t + 0.5f;
            if (t2 >= 1.0f) t2 -= 1.0f;
            sample -= polyBLEP(t2);
            break;
        }

        case Waveform::Pulse:
        {
            // Variable-width pulse wave
            sample = (t < pulseWidth) ? 1.0f : -1.0f;

            // PolyBLEP at rising edge (phase = 0)
            sample += polyBLEP(t);

            // PolyBLEP at falling edge (phase = pulseWidth)
            float t2 = t - pulseWidth;
            if (t2 < 0.0f) t2 += 1.0f;
            sample -= polyBLEP(t2);
            break;
        }

        case Waveform::Triangle:
        {
            // Generate triangle by integrating a PolyBLEP square wave.
            // This produces a band-limited triangle without harmonics issues.
            float sq = (t < 0.5f) ? 1.0f : -1.0f;
            sq += polyBLEP(t);
            float t2 = t + 0.5f;
            if (t2 >= 1.0f) t2 -= 1.0f;
            sq -= polyBLEP(t2);

            // Leaky integrator: integrates the square to form a triangle.
            // The scaling factor (4 * dt) normalizes the amplitude to [-1, 1].
            triangleState += 4.0f * dt * sq;
            // Gentle DC leak to prevent drift
            triangleState *= 0.999f;
            sample = triangleState;
            break;
        }
    }

    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * amplitude;
}
