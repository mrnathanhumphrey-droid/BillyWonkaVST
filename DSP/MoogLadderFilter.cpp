#include "MoogLadderFilter.h"
#include <algorithm>

static constexpr float PI_F = 3.14159265358979323846f;

MoogLadderFilter::MoogLadderFilter() = default;

void MoogLadderFilter::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    oversampledRate = newSampleRate * 2.0;  // 2x oversampling
    updateCoefficients();
}

void MoogLadderFilter::reset()
{
    for (int i = 0; i < 4; ++i)
    {
        stage[i] = 0.0f;
        stageZ1[i] = 0.0f;
    }
    feedbackDelay = 0.0f;
}

void MoogLadderFilter::setCutoff(float freqHz)
{
    cutoffHz = std::max(20.0f, std::min(20000.0f, freqHz));
    updateCoefficients();
}

void MoogLadderFilter::setResonance(float r)
{
    resonance = std::max(0.0f, std::min(1.0f, r));
    updateCoefficients();
}

void MoogLadderFilter::setSaturation(float amount)
{
    saturationAmount = std::max(0.0f, std::min(1.0f, amount));
    // Map saturation amount to drive multiplier (1.0 = clean, up to 4.0 = heavy)
    drive = 1.0f + saturationAmount * 3.0f;
}

void MoogLadderFilter::setSlope(Slope slope)
{
    slopeMode = slope;
}

void MoogLadderFilter::updateCoefficients()
{
    // Compute the normalized cutoff coefficient using the bilinear pre-warped
    // frequency. This maps the analog cutoff to the digital domain accurately.
    //
    // g = tan(pi * fc / fs_oversampled)
    // We clamp to avoid instability near Nyquist.
    float fc = cutoffHz / static_cast<float>(oversampledRate);
    fc = std::min(fc, 0.45f);  // safety clamp below Nyquist/2

    g = static_cast<float>(std::tan(PI_F * static_cast<double>(fc)));
    // Normalize: g / (1 + g) for the one-pole sections
    g = g / (1.0f + g);

    // Map resonance (0-1) to feedback gain.
    // The Moog ladder self-oscillates when feedback gain reaches ~4.0
    // (the loop gain through 4 stages each contributing -6dB compensates).
    // We map 0-1 to 0-4.0 for the full range including self-oscillation.
    gRes = resonance * 4.0f;

    // Gain compensation: resonance boosts frequencies near cutoff but
    // reduces overall passband gain. Compensate proportionally.
    gComp = 1.0f + resonance * 0.5f;
}

float MoogLadderFilter::fastTanh(float x)
{
    // Pade approximant (3,2) — accurate to ~0.001 for |x| < 3,
    // gracefully saturates outside that range.
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

float MoogLadderFilter::process(float input)
{
    float output = 0.0f;

    // Process at 2x oversampled rate (two iterations per input sample)
    for (int os = 0; os < 2; ++os)
    {
        // Apply input drive for saturation character
        float in = input * drive * gComp;

        // Subtract resonance feedback from the input.
        // The feedback comes from the 4th stage output, delayed by one sample
        // at the oversampled rate to break the delay-free loop.
        in -= gRes * feedbackDelay;

        // Apply saturation to the feedback-subtracted input
        in = fastTanh(in);

        // Four cascaded one-pole lowpass stages with per-stage nonlinearity.
        // Each stage uses a one-pole filter: y[n] = y[n-1] + g * (tanh(x) - y[n-1])
        // The tanh at each stage input models the transistor nonlinearity.
        for (int i = 0; i < 4; ++i)
        {
            float stageInput = (i == 0) ? in : stage[i - 1];

            // Apply tanh nonlinearity at stage input (transistor saturation)
            float saturatedInput = fastTanh(stageInput * drive);

            // One-pole lowpass: trapezoidal integration for better accuracy
            // y = y_prev + g * (saturated_input - y_prev)
            stage[i] = stage[i] + g * (saturatedInput - stage[i]);
        }

        // Store feedback value (delayed by one oversampled sample)
        feedbackDelay = stage[3];

        // Select output tap based on slope mode
        if (slopeMode == Slope::TwoPole)
            output = stage[1];  // After 2nd stage = 12 dB/oct
        else
            output = stage[3];  // After 4th stage = 24 dB/oct
    }

    return output;
}
