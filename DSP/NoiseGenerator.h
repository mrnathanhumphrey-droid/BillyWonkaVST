#pragma once
#include <cstdint>
#include <random>

/**
 * White and Pink noise generator.
 *
 * White noise uses a fast xorshift PRNG for efficiency in the audio thread
 * (no heap allocations, deterministic timing).
 *
 * Pink noise uses the Voss-McCartney algorithm with 3 octave bands,
 * producing a -3dB/octave spectral slope.
 */
class NoiseGenerator
{
public:
    enum class NoiseType
    {
        White = 0,
        Pink
    };

    NoiseGenerator();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /** Return the next noise sample. */
    float tick();

    void setType(NoiseType type);
    void setAmplitude(float amp);

private:
    /** Fast xorshift32 PRNG — returns a float in [-1, 1]. */
    float nextWhiteSample();

    NoiseType noiseType = NoiseType::White;
    float amplitude = 1.0f;

    // xorshift32 state
    uint32_t rngState = 0x12345678u;

    // Pink noise: Voss-McCartney running-sum with 3 octave bands
    float pinkRows[3] = { 0.0f, 0.0f, 0.0f };
    float pinkRunningSum = 0.0f;
    int pinkCounter = 0;
};
