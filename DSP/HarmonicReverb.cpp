#include "HarmonicReverb.h"

static constexpr float PI_F = 3.14159265358979323846f;

// Static constexpr member definitions
constexpr int HarmonicReverb::COMB_REF_LENGTHS[4];
constexpr int HarmonicReverb::ALLPASS_REF_LENGTHS[2];

HarmonicReverb::HarmonicReverb() = default;

void HarmonicReverb::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;

    // Scale delay lengths proportionally to sample rate
    double rateRatio = sampleRate / static_cast<double>(REF_RATE);

    for (int i = 0; i < 4; ++i)
    {
        combs[i].length = std::min(static_cast<int>(COMB_REF_LENGTHS[i] * rateRatio + 0.5),
                                   MAX_COMB_LEN - 1);
    }

    for (int i = 0; i < 2; ++i)
    {
        allpasses[i].length = std::min(static_cast<int>(ALLPASS_REF_LENGTHS[i] * rateRatio + 0.5),
                                       MAX_ALLPASS_LEN - 1);
    }

    // Pre-delay
    preDelaySamples = std::min(static_cast<int>(preDelayMs * 0.001f * static_cast<float>(sampleRate) + 0.5f),
                               MAX_PREDELAY - 1);

    updateCrossoverCoefficients();
    updateCombFeedback();

    wetSm.prepare(sampleRate, 20.0f);
    wetSm.snap(wetLevel);
}

void HarmonicReverb::reset()
{
    lpState = 0.0f;

    for (int i = 0; i < MAX_PREDELAY; ++i) preDelayBuf[i] = 0.0f;
    preDelayWriteIdx = 0;

    for (int i = 0; i < 4; ++i) combs[i].clear();
    for (int i = 0; i < 2; ++i) allpasses[i].clear();
}

// =============================================================================
// Parameter Setters
// =============================================================================

void HarmonicReverb::setCrossoverFreq(float hz)
{
    crossoverFreq = std::max(80.0f, std::min(400.0f, hz));
    updateCrossoverCoefficients();
}

void HarmonicReverb::setDecay(float amount)
{
    decayAmount = std::max(0.0f, std::min(1.0f, amount));
    updateCombFeedback();
}

void HarmonicReverb::setPreDelay(float ms)
{
    preDelayMs = std::max(0.0f, std::min(30.0f, ms));
    preDelaySamples = std::min(static_cast<int>(preDelayMs * 0.001f * static_cast<float>(sampleRate) + 0.5f),
                               MAX_PREDELAY - 1);
}

void HarmonicReverb::setWet(float wet)
{
    wetLevel = std::max(0.0f, std::min(1.0f, wet));
    wetSm.set(wetLevel);
}

void HarmonicReverb::setEnabled(bool on)
{
    enabled = on;
}

void HarmonicReverb::updateCrossoverCoefficients()
{
    // First-order Butterworth lowpass: y[n] = (1-c)*x[n] + c*y[n-1]
    // where c = exp(-2*pi*fc/fs)
    float wc = 2.0f * PI_F * crossoverFreq / static_cast<float>(sampleRate);
    lpCoeff = std::exp(-wc);
}

void HarmonicReverb::updateCombFeedback()
{
    // Map decay (0-1) to comb feedback (0.50-0.84)
    combFeedback = 0.50f + decayAmount * 0.34f;
    for (int i = 0; i < 4; ++i)
        combs[i].feedback = combFeedback;
}

// =============================================================================
// Processing
// =============================================================================

float HarmonicReverb::processSample(float input)
{
    if (!enabled)
        return input;

    // =========================================================================
    // Crossover split: first-order lowpass
    // =========================================================================
    lpState = (1.0f - lpCoeff) * input + lpCoeff * lpState;
    float lowPath = lpState;                   // Sub/fundamental (dry)
    float highPath = input - lowPath;          // Harmonics (to be reverbed)

    // =========================================================================
    // Pre-delay on high path
    // =========================================================================
    preDelayBuf[preDelayWriteIdx] = highPath;
    int readIdx = preDelayWriteIdx - preDelaySamples;
    if (readIdx < 0) readIdx += MAX_PREDELAY;
    float preDelayed = preDelayBuf[readIdx];
    preDelayWriteIdx++;
    if (preDelayWriteIdx >= MAX_PREDELAY) preDelayWriteIdx = 0;

    // =========================================================================
    // Schroeder reverb: 4 parallel combs → 2 series allpasses
    // =========================================================================
    // Sum the 4 parallel comb filter outputs
    float combSum = 0.0f;
    for (int i = 0; i < 4; ++i)
        combSum += combs[i].process(preDelayed);
    combSum *= 0.25f;  // Normalize

    // Pass through 2 series allpass filters
    float reverbOut = combSum;
    for (int i = 0; i < 2; ++i)
        reverbOut = allpasses[i].process(reverbOut);

    // =========================================================================
    // Blend and recombine
    // =========================================================================
    float wet = wetSm.tick();
    float highOut = highPath + reverbOut * wet;

    // Recombine: dry sub + processed harmonics
    return lowPath + highOut;
}
