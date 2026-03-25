#pragma once
#include "IBassDriver.h"
#include <cmath>
#include <algorithm>

/**
 * SubHarmonicExciter — 808-voiced bass driver.
 *
 * Models the TR-808 bass drum's incidental nonlinearities:
 * - Even-order polynomial soft shaper (replaces CP3 asymmetric clip)
 * - Softened JA hysteresis (ja_k ~0.2, lower coercivity than tape)
 * - NAB pre/de-emphasis retained
 * - Note-tracked head bump retained (the most 808-authentic element)
 *
 * DC-coupled. Even-harmonic dominant (2nd, 4th). Warm, not gritty.
 */
class SubHarmonicExciter final : public IBassDriver
{
public:
    SubHarmonicExciter();

    void prepare(double sampleRate, int blockSize) override;
    void reset() override;
    float processSample(float input) override;
    void triggerOnset() override;
    void setDrive(float drive) override;
    void setSaturation(float sat) override;
    void setBumpAmount(float bump) override;
    void setMix(float mix) override;
    void setBumpFrequency(float hz) override;
    void setEnvelopeGain(float) override { /* no-op */ }
    void setLFOGain(float) override { /* no-op */ }

private:
    static constexpr float PI_F = 3.14159265358979323846f;

    void updatePreEmphasisCoefficients();
    void updateDeEmphasisCoefficients();
    void updateBumpCoefficients();

    double sampleRate_ = 44100.0;

    // Stage 1: even-order poly shaper — f(x) = x / (1 + α|x|)
    float polyAlpha = 2.0f;
    float polyDrive = 1.0f;

    // Stage 2: pre-emphasis (same as tape — 400 Hz shelf +6 dB)
    float preB0 = 1.0f, preB1 = 0.0f, preA1 = 0.0f;
    float preX1 = 0.0f, preY1 = 0.0f;

    // Stage 3: softened JA hysteresis
    float M_prev = 0.0f, H_prev = 0.0f;
    float ja_a = 1.5f;
    float ja_Ms = 1.0f;
    float ja_k = 0.2f;       // Lower coercivity than tape (0.5)
    float ja_inputGain = 1.0f;
    float ja_dt = 1.0f / 44100.0f;

    static constexpr int ONSET_RAMP_SAMPLES = 96;
    int onsetSamples = 0;

    // Stage 4: de-emphasis + head bump
    float deB0 = 1.0f, deB1 = 0.0f, deA1 = 0.0f;
    float deX1 = 0.0f, deY1 = 0.0f;

    float bumpB0 = 1.0f, bumpB1 = 0.0f, bumpB2 = 0.0f;
    float bumpA1 = 0.0f, bumpA2 = 0.0f;
    float bumpX1 = 0.0f, bumpX2 = 0.0f;
    float bumpY1 = 0.0f, bumpY2 = 0.0f;
    float bumpGainDB = 2.0f;
    float bumpAmountVal = 0.5f;

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

    Smooth driveSm, satSm, bumpSm, mixSm;
};
