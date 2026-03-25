#include "TransientWaveshaper.h"

TransientWaveshaper::TransientWaveshaper() = default;

void TransientWaveshaper::prepare(double sr, int /*blockSize*/)
{
    sampleRate_ = sr;
    ja_dt = 1.0f / static_cast<float>(sr);
    driveSm.prepare(sr); satSm.prepare(sr); mixSm.prepare(sr);
    updatePreEmphasisCoefficients();
    updateDeEmphasisCoefficients();
}

void TransientWaveshaper::reset()
{
    preX1 = preY1 = 0.0f;
    M_prev = H_prev = 0.0f;
    deX1 = deY1 = 0.0f;
    envelopeLevel = 0.0f;
}

void TransientWaveshaper::setDrive(float v)       { driveSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setSaturation(float v)   { satSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setMix(float v)          { mixSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setEnvelopeGain(float e) { envelopeLevel = std::max(0.0f, std::min(1.0f, e)); }

void TransientWaveshaper::triggerOnset()
{
    M_prev *= 0.1f;
    H_prev *= 0.1f;
    onsetSamples = ONSET_RAMP_SAMPLES;
}

void TransientWaveshaper::updatePreEmphasisCoefficients()
{
    float fc = 400.0f, boostDB = 6.0f;
    float G = std::pow(10.0f, boostDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate_));
    float a0 = wc + 1.0f / sqrtG;
    preB0 = (wc + sqrtG) / a0;
    preB1 = (wc - sqrtG) / a0;
    preA1 = (wc - 1.0f / sqrtG) / a0;
}

void TransientWaveshaper::updateDeEmphasisCoefficients()
{
    float fc = 400.0f, cutDB = -4.2f;
    float G = std::pow(10.0f, cutDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate_));
    float a0 = wc + 1.0f / sqrtG;
    deB0 = (wc + sqrtG) / a0;
    deB1 = (wc - sqrtG) / a0;
    deA1 = (wc - 1.0f / sqrtG) / a0;
}

float TransientWaveshaper::processSample(float input)
{
    float driveVal = driveSm.tick();
    float satVal = satSm.tick();
    float mixVal = mixSm.tick();

    // Envelope-gated drive: cpDrive scales with amp envelope
    cpDrive = (0.5f + driveVal * 3.5f) * envelopeLevel;
    ja_inputGain = cpDrive;
    ja_a = 3.0f - satVal * 2.5f;
    ja_a = std::max(0.5f, ja_a);

    float dry = input;

    // Stage 1: CP3 asymmetric clip (correct for Pluck — cross-channel intermod)
    float x = input * cpDrive;
    float stage1;
    if (x >= 0.0f)
        stage1 = x / (1.0f + alpha * x);
    else
        stage1 = x / (1.0f + beta * (-x));

    // Stage 2: Pre-emphasis
    float preOut = preB0 * stage1 + preB1 * preX1 - preA1 * preY1;
    preX1 = stage1;
    preY1 = preOut;

    // Stage 3: JA hysteresis (envelope-scaled — only colors the transient)
    float onsetRamp = (onsetSamples > 0)
        ? 1.0f - static_cast<float>(onsetSamples) / static_cast<float>(ONSET_RAMP_SAMPLES)
        : 1.0f;
    if (onsetSamples > 0) --onsetSamples;

    float H = preOut * ja_inputGain * onsetRamp;
    float dH = H - H_prev;
    float delta = (dH >= 0.0f) ? 1.0f : -1.0f;
    float Man = ja_Ms * std::tanh(H / ja_a);
    float denom = ja_k * delta + 1.0e-6f;
    float dM = (Man - M_prev) / denom;
    float M = M_prev + dM * ja_dt;
    M = std::max(-ja_Ms * 1.5f, std::min(ja_Ms * 1.5f, M));
    M_prev = M;
    H_prev = H;
    float stage3 = M / ja_Ms;

    // Stage 4: De-emphasis only (no head bump for Pluck)
    float deOut = deB0 * stage3 + deB1 * deX1 - deA1 * deY1;
    deX1 = stage3;
    deY1 = deOut;

    return dry + mixVal * (deOut - dry);
}
