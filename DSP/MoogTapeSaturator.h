#pragma once
#include <cmath>
#include <algorithm>

/**
 * MoogTapeSaturator — Magneto-Ladder Saturation.
 *
 * A four-stage tape saturation processor designed specifically for the
 * Moog bass signal path. Chains two distinct nonlinear origins:
 *
 * 1. ASYMMETRIC CP3 PRE-CLIP: Models the Moog CP3 mixer's NPN transistor
 *    input stage. Positive and negative half-cycles clip at different
 *    thresholds, generating even harmonics before the ladder filter.
 *
 * 2. FREQUENCY-WEIGHTED PRE-EMPHASIS: Boosts high-mids (800Hz–4kHz) and
 *    cuts extreme lows before hysteresis, modeling the NAB/IEC tape
 *    recording pre-emphasis curve. Preserves sub-bass weight while
 *    saturating pluck transients and filter sweeps.
 *
 * 3. JILES-ATHERTON HYSTERESIS: Simplified real-time magnetic hysteresis
 *    model. A stateful, memory-dependent process where output depends on
 *    both current input AND previous magnetization state. Generates warm
 *    even harmonics (2nd, 4th) that complement the Moog's odd harmonics.
 *    Uses Euler Forward discretization — no Newton-Raphson, fully real-time safe.
 *
 * 4. DE-EMPHASIS + HEAD BUMP: Partial inverse of pre-emphasis (70%) plus
 *    a resonant 80Hz boost modeling the playback head's mechanical resonance.
 *    This is what makes tape make bass sounds fatter.
 */
class MoogTapeSaturator
{
public:
    MoogTapeSaturator();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /** Process a single sample through all four stages. */
    float processSample(float input);

    /** Process a block of samples in-place. */
    void processBlock(float* buffer, int numSamples);

    void setDrive(float drive);         // 0.0–1.0
    void setSaturation(float sat);      // 0.0–1.0 (controls JA 'a' parameter)
    void setBumpAmount(float bump);     // 0.0–1.0 (head bump gain)
    void setMix(float wetDry);          // 0.0–1.0 (parallel wet/dry blend)

    /** Reset onset ramp and bleed residual magnetization on note-on. */
    void triggerOnset();

    /** Retune head bump frequency for note-tracking (808 mode). */
    void setBumpFrequency(float frequencyHz);

private:
    void updatePreEmphasisCoefficients();
    void updateDeEmphasisCoefficients();
    void updateBumpCoefficients();

    double sampleRate = 44100.0;

    // === Stage 1: CP3 asymmetric shaper ===
    float alpha = 2.0f;          // Positive half-cycle clip coefficient
    float beta  = 1.44f;         // Negative half-cycle (alpha * 0.72, NPN asymmetry)
    float cpDrive = 1.0f;        // Input drive scaling

    // === Stage 2: Pre-emphasis IIR (first-order shelf at 400Hz) ===
    float preEmph_b0 = 1.0f, preEmph_b1 = 0.0f;
    float preEmph_a1 = 0.0f;
    float preEmph_x1 = 0.0f;    // Previous input
    float preEmph_y1 = 0.0f;    // Previous output

    // === Stage 3: Jiles-Atherton hysteresis state ===
    float M_prev = 0.0f;        // Previous magnetization (persistent)
    float H_prev = 0.0f;        // Previous applied field (persistent)
    float ja_a = 1.5f;          // Shape parameter (from saturation param)
    float ja_Ms = 1.0f;         // Saturation magnetization (normalized, fixed)
    float ja_k = 0.5f;          // Coercivity (fixed for musical result)
    float ja_inputGain = 1.0f;  // From drive param
    float ja_outputGain = 1.0f; // Output normalization
    float ja_dt = 1.0f / 44100.0f;

    // === Stage 4: De-emphasis IIR + biquad head bump ===
    // De-emphasis (partial inverse of pre-emphasis, 70% cancellation)
    float deEmph_b0 = 1.0f, deEmph_b1 = 0.0f;
    float deEmph_a1 = 0.0f;
    float deEmph_x1 = 0.0f;
    float deEmph_y1 = 0.0f;

    // Head bump: second-order peaking biquad at 80Hz
    float bumpB0 = 1.0f, bumpB1 = 0.0f, bumpB2 = 0.0f;
    float bumpA1 = 0.0f, bumpA2 = 0.0f;
    float bumpX1 = 0.0f, bumpX2 = 0.0f;
    float bumpY1 = 0.0f, bumpY2 = 0.0f;
    float bumpGainDB = 2.0f;    // Max bump gain at full amount
    float bumpAmount = 0.5f;    // Current bump amount for setBumpFrequency

    // Onset ramp to prevent JA transient pop on note-on
    static constexpr int ONSET_RAMP_SAMPLES = 96;  // ~2ms at 44.1kHz
    int onsetSamples = 0;

    // === Parameter targets (smoothed per-sample) ===
    // Simple one-pole smoothers (no JUCE dependency in DSP code)
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

    Smooth driveSm;
    Smooth satSm;
    Smooth bumpSm;
    Smooth mixSm;
};
