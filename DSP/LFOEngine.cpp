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

// Standard musical beat divisions expressed as cycles-per-beat.
// 1/4 note = 1.0 cycle per beat is the anchor; faster divisions are >1.
static constexpr float kBeatDivisions[] = {
    0.25f,  // 1/1
    0.5f,   // 1/2
    0.75f,  // 1/2T  (dotted/triplet pair around 1/2)
    1.0f,   // 1/4
    1.5f,   // 1/4T
    2.0f,   // 1/8
    3.0f,   // 1/8T
    4.0f,   // 1/16
    6.0f,   // 1/16T
    8.0f    // 1/32
};
static constexpr int kNumBeatDivisions =
    static_cast<int>(sizeof(kBeatDivisions) / sizeof(kBeatDivisions[0]));

static float quantizeToBeatDivision(float rate)
{
    float best = kBeatDivisions[0];
    float bestErr = std::abs(rate - best);
    for (int i = 1; i < kNumBeatDivisions; ++i)
    {
        float err = std::abs(rate - kBeatDivisions[i]);
        if (err < bestErr) { bestErr = err; best = kBeatDivisions[i]; }
    }
    return best;
}

void LFOEngine::setRate(float hz)
{
    rate = std::max(0.01f, std::min(20.0f, hz));

    if (tempoSync)
    {
        // Sync on: snap rate to the nearest standard beat division.
        // 'rate' now means cycles-per-beat (1/4 note = 1.0).
        float quantised = quantizeToBeatDivision(rate);
        double beatsPerSecond = tempoBPM / 60.0;
        phaseIncrement = (beatsPerSecond * static_cast<double>(quantised)) / sampleRate;
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
