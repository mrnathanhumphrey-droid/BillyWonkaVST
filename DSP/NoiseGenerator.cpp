#include "NoiseGenerator.h"

NoiseGenerator::NoiseGenerator() = default;

void NoiseGenerator::prepare(double /*sampleRate*/, int /*blockSize*/)
{
    reset();
}

void NoiseGenerator::reset()
{
    rngState = 0x12345678u;
    pinkRows[0] = 0.0f;
    pinkRows[1] = 0.0f;
    pinkRows[2] = 0.0f;
    pinkRunningSum = 0.0f;
    pinkCounter = 0;
}

void NoiseGenerator::setType(NoiseType type)
{
    noiseType = type;
}

void NoiseGenerator::setAmplitude(float amp)
{
    amplitude = amp;
}

float NoiseGenerator::nextWhiteSample()
{
    // xorshift32 — fast, non-allocating PRNG
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;

    // Convert uint32 to float in [-1, 1]
    // Divide by half of uint32 max to get [0, 2], then subtract 1
    return (static_cast<float>(rngState) / 2147483648.0f) - 1.0f;
}

float NoiseGenerator::tick()
{
    float sample = 0.0f;

    switch (noiseType)
    {
        case NoiseType::White:
        {
            sample = nextWhiteSample();
            break;
        }

        case NoiseType::Pink:
        {
            // Voss-McCartney algorithm: update one row per sample based on
            // the counter's trailing zeros. This distributes updates across
            // octave bands, producing a -3dB/octave slope.
            float white = nextWhiteSample();

            // Determine which row to update based on counter bits
            int rowToUpdate = -1;
            if ((pinkCounter & 1) == 0)  rowToUpdate = 0;  // every 2 samples
            if ((pinkCounter & 3) == 0)  rowToUpdate = 1;  // every 4 samples
            if ((pinkCounter & 7) == 0)  rowToUpdate = 2;  // every 8 samples

            if (rowToUpdate >= 0)
            {
                pinkRunningSum -= pinkRows[rowToUpdate];
                pinkRows[rowToUpdate] = white;
                pinkRunningSum += pinkRows[rowToUpdate];
            }

            // Mix running sum with current white noise and normalize
            sample = (pinkRunningSum + white) * 0.25f;

            pinkCounter = (pinkCounter + 1) & 7;  // wrap at 8
            break;
        }
    }

    return sample * amplitude;
}
