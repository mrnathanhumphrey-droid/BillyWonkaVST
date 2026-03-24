#pragma once
#include <cmath>
#include <algorithm>

/**
 * RnBCompressor — Three-stage serial + parallel compression chain
 * for R&B bass production.
 *
 * Stage 1: Transient VCA — slow attack (40-80ms) preserves the initial
 *          pluck/808 punch, only compressing the sustain body.
 *
 * Stage 2: Optical/program-dependent compressor — dual time-constant
 *          envelope follower that breathes with the signal. Fast constant
 *          catches transients, slow constant shapes the body.
 *
 * Stage 3: Parallel blend — mixes uncompressed post-stage-1 signal with
 *          a heavily compressed (12:1) version for density control.
 *
 * Pure C++ — no JUCE dependencies.
 */
class RnBCompressor
{
public:
    RnBCompressor();

    void prepare(double sampleRate, int blockSize);
    void reset();

    float processSample(float input);

    void setThreshold(float dBFS);          // default -12.0
    void setTransientAttack(float ms);      // default 60.0
    void setTransientRelease(float ms);     // default 100.0
    void setOpticalRatio(float ratio);      // default 6.0, range [4, 8]
    void setParallelMix(float mix);         // default 0.45, range [0, 1]
    void setOutputGain(float dB);           // makeup gain, default +3.0 dB

private:
    // One-pole smoother (matches MoogTapeSaturator::Smooth pattern)
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

    /** Convert time in ms to one-pole coefficient. */
    float msToCoeff(float ms) const;

    /** Convert dBFS to linear amplitude. */
    static float dBToLinear(float dB) { return std::pow(10.0f, dB / 20.0f); }

    /** Convert linear amplitude to dBFS. */
    static float linearToDB(float lin) { return 20.0f * std::log10(std::max(lin, 1.0e-10f)); }

    double sampleRate = 44100.0;

    // --- Stage 1: Transient VCA ---
    float s1_attackCoeff = 0.0f;
    float s1_releaseCoeff = 0.0f;
    float s1_envelope = 0.0f;       // Envelope follower state
    float s1_ratio = 4.0f;          // Fixed 4:1

    // --- Stage 2: Optical (dual time-constant) ---
    float s2_fastCoeff = 0.0f;      // 30ms
    float s2_slowCoeff = 0.0f;      // 300ms
    float s2_fastEnv = 0.0f;
    float s2_slowEnv = 0.0f;
    float s2_prevLevel = 0.0f;      // For delta detection

    // --- Stage 3: Parallel heavy compressor ---
    float s3_attackCoeff = 0.0f;    // 5ms fast attack
    float s3_releaseCoeff = 0.0f;   // 200ms slow release
    float s3_envelope = 0.0f;
    float s3_ratio = 12.0f;         // Fixed 12:1

    // --- Parameters ---
    float thresholdDB = -12.0f;
    float thresholdLin = 0.25119f;  // 10^(-12/20)
    float transientAttackMs = 60.0f;
    float transientReleaseMs = 100.0f;
    float opticalRatio = 6.0f;
    float outputGainLin = 1.4125f;  // 10^(3/20)

    Smooth parallelMixSm;
    Smooth outputGainSm;
};
