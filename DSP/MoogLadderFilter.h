#pragma once
#include <cmath>

/**
 * Moog Ladder Filter — Huovilainen (2004) improved model.
 *
 * This implements the 4-stage nonlinear transistor ladder filter that defines
 * the Minimoog's legendary warm, "fat" sound. The model uses:
 *
 * - Four cascaded one-pole sections with tanh() saturation at each stage,
 *   modeling the transistor nonlinearities of the original circuit.
 * - 2x internal oversampling to reduce aliasing from the nonlinear processing.
 * - Selectable 2-pole (12 dB/oct) or 4-pole (24 dB/oct) output taps.
 * - Self-oscillation capability at high resonance (~0.95+).
 *
 * Reference: Antti Huovilainen, "Non-Linear Digital Implementation of the
 * Moog Ladder Filter", Proc. DAFx-04, Naples, Italy, 2004.
 */
class MoogLadderFilter
{
public:
    enum class Slope
    {
        TwoPole = 0,   // 12 dB/oct
        FourPole = 1   // 24 dB/oct
    };

    MoogLadderFilter();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /** Process one input sample through the ladder filter. */
    float process(float input);

    /** Set cutoff frequency in Hz (20 – 20000). */
    void setCutoff(float freqHz);

    /** Set resonance amount (0.0 – 1.0). Self-oscillation near 0.95+. */
    void setResonance(float r);

    /** Set saturation amount (0.0 – 1.0). Controls tanh() drive at each stage. */
    void setSaturation(float amount);

    /** Select 2-pole or 4-pole output. */
    void setSlope(Slope slope);

    float getCutoff() const { return cutoffHz; }
    float getResonance() const { return resonance; }

private:
    /** Compute filter coefficients from cutoff frequency. */
    void updateCoefficients();

    /** Fast tanh approximation (Pade 3/2) — avoids std::tanh overhead. */
    static float fastTanh(float x);

    double sampleRate = 44100.0;
    double oversampledRate = 88200.0;  // 2x oversampled

    float cutoffHz = 2000.0f;
    float resonance = 0.2f;
    float saturationAmount = 0.2f;
    Slope slopeMode = Slope::FourPole;

    // Filter state: 4 cascaded stages
    float stage[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float stageZ1[4] = { 0.0f, 0.0f, 0.0f, 0.0f };  // previous stage outputs (for trapezoidal integration)

    // Feedback delay (1 sample at oversampled rate)
    float feedbackDelay = 0.0f;

    // Precomputed coefficients
    float g = 0.0f;     // Cutoff coefficient (normalized, pre-warped)
    float gRes = 0.0f;  // Resonance feedback gain (mapped from 0-1 to usable range)
    float gComp = 0.0f; // Gain compensation factor (resonance reduces bass, this compensates)
    float drive = 1.0f;  // Saturation drive multiplier
};
