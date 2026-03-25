#pragma once
#include "IBassDriver.h"
#include <cmath>
#include <algorithm>

/**
 * SymmetricFolder — Reese-voiced bass driver.
 *
 * Symmetric wavefolding with no DC offset. Models the CZ-5000's phase
 * distortion symmetry and the SH-101's back-to-back diode clipping.
 * Odd-harmonic dominant (3rd, 5th, 7th).
 *
 * JA hysteresis is fully bypassed — its stateful memory flattens LFO
 * wobble dynamics and the asymmetric CP3 clip causes DC bias on detuned saws.
 *
 * Drive depth is modulated by LFO value per sample via setLFOGain().
 * No head bump. No pre/de-emphasis (unnecessary without JA).
 */
class SymmetricFolder final : public IBassDriver
{
public:
    SymmetricFolder();

    void prepare(double sampleRate, int blockSize) override;
    void reset() override;
    float processSample(float input) override;
    void triggerOnset() override { /* no-op: no stateful memory to bleed */ }
    void setDrive(float drive) override;
    void setSaturation(float sat) override;
    void setBumpAmount(float) override { /* no-op: no bump for Reese */ }
    void setMix(float mix) override;
    void setBumpFrequency(float) override { /* no-op */ }
    void setEnvelopeGain(float) override { /* no-op */ }
    void setLFOGain(float lfo) override;

private:
    // Symmetric wavefold: f(x) = (2/π) * arcsin(sin(π/2 * x / thresh))
    float foldThreshold = 1.0f;
    float baseDrive = 1.0f;
    float lfoValue = 0.0f;

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

    Smooth driveSm, satSm, mixSm;
};
