#include "MoogTapeSaturator.h"

static constexpr float PI_F = 3.14159265358979323846f;

MoogTapeSaturator::MoogTapeSaturator() = default;

void MoogTapeSaturator::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    ja_dt = 1.0f / static_cast<float>(sampleRate);

    driveSm.prepare(sampleRate, 20.0f);
    satSm.prepare(sampleRate, 20.0f);
    bumpSm.prepare(sampleRate, 20.0f);
    mixSm.prepare(sampleRate, 20.0f);

    updatePreEmphasisCoefficients();
    updateDeEmphasisCoefficients();
    updateBumpCoefficients();
}

void MoogTapeSaturator::reset()
{
    // Stage 2: pre-emphasis state
    preEmph_x1 = 0.0f;
    preEmph_y1 = 0.0f;

    // Stage 3: hysteresis state
    M_prev = 0.0f;
    H_prev = 0.0f;

    // Stage 4: de-emphasis + bump state
    deEmph_x1 = 0.0f;
    deEmph_y1 = 0.0f;
    bumpX1 = 0.0f; bumpX2 = 0.0f;
    bumpY1 = 0.0f; bumpY2 = 0.0f;
}

// =============================================================================
// Parameter Setters
// =============================================================================

void MoogTapeSaturator::setDrive(float drive)
{
    float d = std::max(0.0f, std::min(1.0f, drive));
    driveSm.set(d);
}

void MoogTapeSaturator::setSaturation(float sat)
{
    float s = std::max(0.0f, std::min(1.0f, sat));
    satSm.set(s);
}

void MoogTapeSaturator::setBumpAmount(float bump)
{
    float b = std::max(0.0f, std::min(1.0f, bump));
    bumpSm.set(b);
}

void MoogTapeSaturator::setMix(float wetDry)
{
    float m = std::max(0.0f, std::min(1.0f, wetDry));
    mixSm.set(m);
}

void MoogTapeSaturator::triggerOnset()
{
    M_prev *= 0.1f;   // Bleed 90% of residual magnetization
    H_prev *= 0.1f;   // Bleed corresponding field state
    onsetSamples = ONSET_RAMP_SAMPLES;
}

void MoogTapeSaturator::setBumpFrequency(float frequencyHz)
{
    // Clamp to safe range for stable biquad coefficients
    float f0 = std::max(30.0f, std::min(200.0f, frequencyHz * 1.2f));

    // Recompute peaking biquad (Audio EQ Cookbook)
    float A = std::pow(10.0f, (2.0f * bumpAmount) / 40.0f);
    float w0 = 2.0f * PI_F * f0 / static_cast<float>(sampleRate);
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alphaQ = sinw0 / (2.0f * 1.8f);  // Q = 1.8
    float a0 = 1.0f + alphaQ / A;

    bumpB0 = (1.0f + alphaQ * A) / a0;
    bumpB1 = (-2.0f * cosw0)     / a0;
    bumpB2 = (1.0f - alphaQ * A) / a0;
    bumpA1 = (-2.0f * cosw0)     / a0;
    bumpA2 = (1.0f - alphaQ / A) / a0;
}

// =============================================================================
// Coefficient Calculations
// =============================================================================

void MoogTapeSaturator::updatePreEmphasisCoefficients()
{
    // First-order high shelf at 400Hz (the NAB/IEC emphasis knee).
    // Boosts frequencies above 400Hz by ~6dB, modeling tape pre-emphasis.
    //
    // H(z) = (b0 + b1*z^-1) / (1 + a1*z^-1)
    //
    // Design: analog high shelf prototype with ~6dB boost,
    // bilinear-transformed to digital.
    float fc = 400.0f;
    float boostDB = 6.0f;
    float G = std::pow(10.0f, boostDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate));

    // High shelf: H(s) = (sqrt(G)*s + wc) / (s/sqrt(G) + wc)
    float a0 = wc + 1.0f / sqrtG;
    preEmph_b0 = (wc + sqrtG) / a0;
    preEmph_b1 = (wc - sqrtG) / a0;
    preEmph_a1 = (wc - 1.0f / sqrtG) / a0;
}

void MoogTapeSaturator::updateDeEmphasisCoefficients()
{
    // Partial inverse of pre-emphasis: only 70% cancellation.
    // Use a high shelf with -4.2dB cut (70% of -6dB) at 400Hz.
    float fc = 400.0f;
    float cutDB = -4.2f;  // 70% of 6dB = 4.2dB cut
    float G = std::pow(10.0f, cutDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate));

    float a0 = wc + 1.0f / sqrtG;
    deEmph_b0 = (wc + sqrtG) / a0;
    deEmph_b1 = (wc - sqrtG) / a0;
    deEmph_a1 = (wc - 1.0f / sqrtG) / a0;
}

void MoogTapeSaturator::updateBumpCoefficients()
{
    // Second-order peaking EQ (biquad) at 80Hz, Q=1.8, gain up to +2dB.
    // Audio EQ Cookbook formula (Robert Bristow-Johnson).
    float fc = 80.0f;
    float Q = 1.8f;
    float gainDB = bumpGainDB;  // +2dB at full bump_amount

    float A = std::pow(10.0f, gainDB / 40.0f);  // sqrt of linear gain
    float w0 = 2.0f * PI_F * fc / static_cast<float>(sampleRate);
    float sinw0 = std::sin(w0);
    float cosw0 = std::cos(w0);
    float alphaQ = sinw0 / (2.0f * Q);

    // Peaking EQ coefficients
    float b0 = 1.0f + alphaQ * A;
    float b1 = -2.0f * cosw0;
    float b2 = 1.0f - alphaQ * A;
    float a0 = 1.0f + alphaQ / A;
    float a1 = -2.0f * cosw0;
    float a2 = 1.0f - alphaQ / A;

    // Normalize by a0
    bumpB0 = b0 / a0;
    bumpB1 = b1 / a0;
    bumpB2 = b2 / a0;
    bumpA1 = a1 / a0;
    bumpA2 = a2 / a0;
}

// =============================================================================
// Processing
// =============================================================================

float MoogTapeSaturator::processSample(float input)
{
    // Smooth parameters
    float driveVal = driveSm.tick();
    float satVal = satSm.tick();
    float bumpVal = bumpSm.tick();
    float mixVal = mixSm.tick();

    // Map parameters to algorithm values
    cpDrive = 0.5f + driveVal * 3.5f;           // drive → [0.5, 4.0]
    ja_inputGain = cpDrive;
    ja_a = 3.0f - satVal * 2.5f;                // saturation → a: [3.0, 0.5] (lower = harder)
    ja_a = std::max(0.5f, ja_a);
    bumpAmount = bumpVal;                        // Track for setBumpFrequency()

    float dry = input;

    // =====================================================================
    // STAGE 1: Asymmetric CP3 Pre-Clip
    // =====================================================================
    float x = input * cpDrive;

    float stage1;
    if (x >= 0.0f)
    {
        // Positive half: soft Padé approximation
        stage1 = x / (1.0f + alpha * x);
    }
    else
    {
        // Negative half: different coefficient (NPN asymmetry)
        stage1 = x / (1.0f + beta * (-x));
    }

    // =====================================================================
    // STAGE 2: Frequency-Weighted Pre-Emphasis
    // =====================================================================
    // First-order IIR: y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
    float preEmph_out = preEmph_b0 * stage1 + preEmph_b1 * preEmph_x1
                        - preEmph_a1 * preEmph_y1;
    preEmph_x1 = stage1;
    preEmph_y1 = preEmph_out;

    // =====================================================================
    // STAGE 3: Jiles-Atherton Hysteresis (Euler Forward)
    // =====================================================================
    // Onset ramp prevents JA transient pop on note-on
    float onsetRamp = (onsetSamples > 0)
        ? 1.0f - static_cast<float>(onsetSamples) / static_cast<float>(ONSET_RAMP_SAMPLES)
        : 1.0f;
    if (onsetSamples > 0) --onsetSamples;

    float H = preEmph_out * ja_inputGain * onsetRamp;

    // Direction flag: sign of dH/dt
    float dH = H - H_prev;
    float delta = (dH >= 0.0f) ? 1.0f : -1.0f;

    // Anhysteretic magnetization: Man = Ms * tanh(H / a)
    float Man = ja_Ms * std::tanh(H / ja_a);

    // Magnetization rate of change
    float denom = ja_k * delta + 1.0e-6f;  // epsilon avoids division by zero
    float dM = (Man - M_prev) / denom;

    // Update magnetization state (Euler Forward integration)
    float M = M_prev + dM * ja_dt;

    // Clamp magnetization to prevent runaway
    M = std::max(-ja_Ms * 1.5f, std::min(ja_Ms * 1.5f, M));

    // Store state for next sample
    M_prev = M;
    H_prev = H;

    // Scale output — normalize by Ms so output is in [-1.5, 1.5] range
    float stage3 = M / ja_Ms;

    // =====================================================================
    // STAGE 4: De-Emphasis (partial) + Head Bump
    // =====================================================================
    // De-emphasis IIR (70% inverse of pre-emphasis)
    float deEmph_out = deEmph_b0 * stage3 + deEmph_b1 * deEmph_x1
                       - deEmph_a1 * deEmph_y1;
    deEmph_x1 = stage3;
    deEmph_y1 = deEmph_out;

    // Head bump biquad (second-order peaking at 80Hz)
    // Scale the bump effect by the bump amount parameter
    float bumpInput = deEmph_out;
    float bumpOut = bumpB0 * bumpInput + bumpB1 * bumpX1 + bumpB2 * bumpX2
                    - bumpA1 * bumpY1 - bumpA2 * bumpY2;
    bumpX2 = bumpX1;
    bumpX1 = bumpInput;
    bumpY2 = bumpY1;
    bumpY1 = bumpOut;

    // Blend between bypassed de-emphasis output and bumped output
    float stage4 = deEmph_out + bumpVal * (bumpOut - deEmph_out);

    // =====================================================================
    // WET/DRY MIX
    // =====================================================================
    float output = dry + mixVal * (stage4 - dry);

    return output;
}

void MoogTapeSaturator::processBlock(float* buffer, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        buffer[i] = processSample(buffer[i]);
    }
}
