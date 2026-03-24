#pragma once
#include <cmath>
#include <algorithm>

/**
 * HarmonicReverb — Frequency-split reverb for R&B bass.
 *
 * Applies reverb ONLY to the harmonic layer (above crossover frequency),
 * leaving the sub/fundamental completely dry. This is the signature R&B
 * bass reverb approach: sub stays tight and centered, harmonics get a
 * short plate-style ambience.
 *
 * Signal flow:
 *   input → crossover split → low (dry) + high (Schroeder reverb) → recombine
 *
 * Reverb topology: 4 parallel comb filters + 2 series allpass filters
 * (Freeverb-style, tuned for short 0.1–0.3s decay).
 *
 * Pure C++ — no JUCE dependencies.
 */
class HarmonicReverb
{
public:
    HarmonicReverb();

    void prepare(double sampleRate, int blockSize);
    void reset();

    float processSample(float input);

    void setCrossoverFreq(float hz);    // default 200 Hz
    void setDecay(float amount);        // 0-1, maps to comb feedback 0.50-0.84
    void setPreDelay(float ms);         // 0-30ms
    void setWet(float wet);             // 0-1, default 0.12
    void setEnabled(bool on);           // bypass

private:
    void updateCrossoverCoefficients();
    void updateCombFeedback();

    double sampleRate = 44100.0;
    bool enabled = true;

    // --- Crossover: first-order Butterworth lowpass ---
    float crossoverFreq = 200.0f;
    float lpCoeff = 0.0f;       // IIR coefficient
    float lpState = 0.0f;       // Filter state (z^-1)

    // --- Pre-delay circular buffer ---
    // Max 30ms at 96kHz = 2880 samples
    static constexpr int MAX_PREDELAY = 2880;
    float preDelayBuf[MAX_PREDELAY] = {};
    int preDelayWriteIdx = 0;
    int preDelaySamples = 0;

    // --- Comb filters (4 parallel) ---
    // Delay lengths at 44100 Hz reference rate
    static constexpr int REF_RATE = 44100;
    static constexpr int COMB_REF_LENGTHS[4] = { 1116, 1188, 1277, 1356 };

    // Max delay length scaled for 96kHz: ceil(1356 * 96000/44100) = ~2952
    static constexpr int MAX_COMB_LEN = 3000;

    struct CombFilter
    {
        float buffer[MAX_COMB_LEN] = {};
        int length = 0;
        int writeIdx = 0;
        float feedback = 0.7f;

        void clear()
        {
            for (int i = 0; i < MAX_COMB_LEN; ++i) buffer[i] = 0.0f;
            writeIdx = 0;
        }

        float process(float input)
        {
            int readIdx = writeIdx - length;
            if (readIdx < 0) readIdx += MAX_COMB_LEN;

            float delayed = buffer[readIdx];
            float output = delayed;

            buffer[writeIdx] = input + delayed * feedback;
            writeIdx++;
            if (writeIdx >= MAX_COMB_LEN) writeIdx = 0;

            return output;
        }
    };

    CombFilter combs[4];

    // --- Allpass filters (2 series) ---
    static constexpr int ALLPASS_REF_LENGTHS[2] = { 556, 441 };
    static constexpr int MAX_ALLPASS_LEN = 1300;
    static constexpr float ALLPASS_FEEDBACK = 0.5f;

    struct AllpassFilter
    {
        float buffer[MAX_ALLPASS_LEN] = {};
        int length = 0;
        int writeIdx = 0;

        void clear()
        {
            for (int i = 0; i < MAX_ALLPASS_LEN; ++i) buffer[i] = 0.0f;
            writeIdx = 0;
        }

        float process(float input)
        {
            int readIdx = writeIdx - length;
            if (readIdx < 0) readIdx += MAX_ALLPASS_LEN;

            float delayed = buffer[readIdx];
            float output = -input + delayed;

            buffer[writeIdx] = input + delayed * ALLPASS_FEEDBACK;
            writeIdx++;
            if (writeIdx >= MAX_ALLPASS_LEN) writeIdx = 0;

            return output;
        }
    };

    AllpassFilter allpasses[2];

    // --- Parameters ---
    float decayAmount = 0.3f;
    float combFeedback = 0.6f;     // Mapped from decay
    float wetLevel = 0.12f;
    float preDelayMs = 8.0f;

    // Smoothers
    struct Smooth
    {
        float current = 0.0f, target = 0.0f, coeff = 0.999f;
        void prepare(double sr, float timeMs = 20.0f)
        {
            float samples = static_cast<float>(sr) * timeMs * 0.001f;
            coeff = (samples > 1.0f) ? std::exp(-2.302585f / samples) : 0.0f;
        }
        void set(float v) { target = v; }
        void snap(float v) { current = v; target = v; }
        float tick() { current = current * coeff + target * (1.0f - coeff); return current; }
    };

    Smooth wetSm;
};
