#include "SubHarmonicExciter.h"

SubHarmonicExciter::SubHarmonicExciter() = default;

void SubHarmonicExciter::prepare(double sr, int /*blockSize*/)
{
    sampleRate_ = sr;
    ja_dt = 1.0f / static_cast<float>(sr);
    driveSm.prepare(sr); satSm.prepare(sr); bumpSm.prepare(sr); mixSm.prepare(sr);
    updatePreEmphasisCoefficients();
    updateDeEmphasisCoefficients();
    updateBumpCoefficients();
}

void SubHarmonicExciter::reset()
{
    preX1 = preY1 = 0.0f;
    M_prev = H_prev = 0.0f;
    deX1 = deY1 = 0.0f;
    bumpX1 = bumpX2 = bumpY1 = bumpY2 = 0.0f;
}

void SubHarmonicExciter::setDrive(float v)      { driveSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SubHarmonicExciter::setSaturation(float v)  { satSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SubHarmonicExciter::setBumpAmount(float v)  { bumpSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SubHarmonicExciter::setMix(float v)         { mixSm.set(std::max(0.0f, std::min(1.0f, v))); }

void SubHarmonicExciter::triggerOnset()
{
    M_prev *= 0.1f;
    H_prev *= 0.1f;
    onsetSamples = ONSET_RAMP_SAMPLES;
}

void SubHarmonicExciter::setBumpFrequency(float hz)
{
    float f0 = std::max(30.0f, std::min(200.0f, hz * 1.2f));
    float A = std::pow(10.0f, (2.0f * bumpAmountVal) / 40.0f);
    float w0 = 2.0f * PI_F * f0 / static_cast<float>(sampleRate_);
    float cosw0 = std::cos(w0), sinw0 = std::sin(w0);
    float alphaQ = sinw0 / (2.0f * 1.8f);
    float a0 = 1.0f + alphaQ / A;
    bumpB0 = (1.0f + alphaQ * A) / a0;
    bumpB1 = (-2.0f * cosw0) / a0;
    bumpB2 = (1.0f - alphaQ * A) / a0;
    bumpA1 = (-2.0f * cosw0) / a0;
    bumpA2 = (1.0f - alphaQ / A) / a0;
}

void SubHarmonicExciter::updatePreEmphasisCoefficients()
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

void SubHarmonicExciter::updateDeEmphasisCoefficients()
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

void SubHarmonicExciter::updateBumpCoefficients()
{
    float fc = 80.0f, Q = 1.8f;
    float A = std::pow(10.0f, bumpGainDB / 40.0f);
    float w0 = 2.0f * PI_F * fc / static_cast<float>(sampleRate_);
    float sinw0 = std::sin(w0), cosw0 = std::cos(w0);
    float alphaQ = sinw0 / (2.0f * Q);
    float a0 = 1.0f + alphaQ / A;
    bumpB0 = (1.0f + alphaQ * A) / a0;
    bumpB1 = (-2.0f * cosw0) / a0;
    bumpB2 = (1.0f - alphaQ * A) / a0;
    bumpA1 = (-2.0f * cosw0) / a0;
    bumpA2 = (1.0f - alphaQ / A) / a0;
}

float SubHarmonicExciter::processSample(float input)
{
    float driveVal = driveSm.tick();
    float satVal = satSm.tick();
    float bumpVal = bumpSm.tick();
    float mixVal = mixSm.tick();

    polyDrive = 0.5f + driveVal * 3.5f;
    ja_inputGain = polyDrive;
    ja_a = 3.0f - satVal * 2.5f;
    ja_a = std::max(0.5f, ja_a);
    bumpAmountVal = bumpVal;

    float dry = input;

    // Stage 1: Even-order polynomial soft shaper (symmetric — no DC offset)
    // f(x) = x / (1 + α|x|) — generates even harmonics via amplitude-dependent gain
    float x = input * polyDrive;
    float stage1 = x / (1.0f + polyAlpha * std::abs(x));

    // Stage 2: Pre-emphasis
    float preOut = preB0 * stage1 + preB1 * preX1 - preA1 * preY1;
    preX1 = stage1;
    preY1 = preOut;

    // Stage 3: Softened JA hysteresis (ja_k = 0.2, lower than tape's 0.5)
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

    // Stage 4: De-emphasis + head bump
    float deOut = deB0 * stage3 + deB1 * deX1 - deA1 * deY1;
    deX1 = stage3;
    deY1 = deOut;

    float bumpIn = deOut;
    float bumpOut = bumpB0 * bumpIn + bumpB1 * bumpX1 + bumpB2 * bumpX2
                    - bumpA1 * bumpY1 - bumpA2 * bumpY2;
    bumpX2 = bumpX1; bumpX1 = bumpIn;
    bumpY2 = bumpY1; bumpY1 = bumpOut;

    float stage4 = deOut + bumpVal * (bumpOut - deOut);

    return dry + mixVal * (stage4 - dry);
}
