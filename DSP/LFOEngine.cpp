#include "LFOEngine.h"
#include <algorithm>

static constexpr double TWO_PI = 6.283185307179586476925286766559;

LFOEngine::LFOEngine() = default;

void LFOEngine::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    phaseIncrement = static_cast<double>(rate) / sampleRate;
}

void LFOEngine::reset()
{
    phase = 0.0;
}

void LFOEngine::resetPhase()
{
    phase = 0.0;
}

void LFOEngine::setRate(float hz)
{
    rate = std::max(0.01f, std::min(20.0f, hz));

    if (tempoSync)
    {
        // When tempo sync is enabled, 'rate' represents a beat division.
        // Convert to Hz: frequency = (BPM / 60) * beatDivision
        // For simplicity, rate maps: 1 = quarter note, 0.5 = half note, 2 = eighth, etc.
        double beatsPerSecond = tempoBPM / 60.0;
        phaseIncrement = (beatsPerSecond * static_cast<double>(rate)) / sampleRate;
    }
    else
    {
        phaseIncrement = static_cast<double>(rate) / sampleRate;
    }
}

void LFOEngine::setDepth(float d)
{
    depth = std::max(0.0f, std::min(1.0f, d));
}

void LFOEngine::setWaveform(Waveform wf)
{
    waveform = wf;
}

void LFOEngine::setTarget(Target t)
{
    target = t;
}

void LFOEngine::setTempoSync(bool sync)
{
    tempoSync = sync;
    // Recompute phase increment with current rate
    setRate(rate);
}

void LFOEngine::setTempoBPM(double bpm)
{
    tempoBPM = std::max(20.0, std::min(300.0, bpm));
    if (tempoSync)
        setRate(rate);  // Recompute
}

float LFOEngine::tick()
{
    float sample = 0.0f;
    const float t = static_cast<float>(phase);

    switch (waveform)
    {
        case Waveform::Triangle:
        {
            // Triangle: rises from -1 to +1 in first half, falls back in second half
            if (t < 0.5f)
                sample = 4.0f * t - 1.0f;      // -1 to +1
            else
                sample = 3.0f - 4.0f * t;      // +1 to -1
            break;
        }

        case Waveform::Square:
        {
            sample = (t < 0.5f) ? 1.0f : -1.0f;
            break;
        }

        case Waveform::Sine:
        {
            sample = std::sin(static_cast<float>(phase * TWO_PI));
            break;
        }
    }

    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    // Scale by depth and return
    return sample * depth;
}
