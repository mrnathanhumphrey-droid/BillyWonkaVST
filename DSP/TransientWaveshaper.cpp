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
    dcX1 = dcY1 = 0.0f;
    envelopeLevel = 0.0f;
}

void TransientWaveshaper::setDrive(float v)       { driveSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setSaturation(float v)   { satSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setMix(float v)          { mixSm.set(std::max(0.0f, std::min(1.0f, v))); }
void TransientWaveshaper::setEnvelopeGain(float e) { envelopeLevel = std::max(0.0f, std::min(1.0f, e)); }

void TransientWaveshaper::triggerOnset()
{
    // Full state clear on note-on. The previous design bled JA only by 10%
    // and left pre/de-emphasis filter state entirely untouched — when the
    // render loop is gated by `noteActive`, that state freezes mid-decay
    // and the next note's first sample triggers a ringing transient out of
    // the stale filter coefficients. Click was audible as a "mechanical pop".
    M_prev = 0.0f;
    H_prev = 0.0f;
    preX1 = preY1 = 0.0f;
    deX1 = deY1 = 0.0f;
    dcX1 = dcY1 = 0.0f;
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

    // Envelope-modulated drive amount: at attack peak (envelopeLevel = 1) the
    // signal gets the full driveVal * 6 saturation; during sustain / release
    // (envelopeLevel → 0) saturation drops out but cpDrive floors at 1 so the
    // dry signal still passes through stage 1's soft-clip in its linear region.
    // Previous formula `(1 + driveVal*6) * envelopeLevel` zeroed cpDrive when
    // the env hit 0, multiplying the input by 0 and silencing the entire
    // sustain of any pluck-shaped envelope (only the transient was audible).
    float drvAmount = driveVal * envelopeLevel;
    cpDrive = 1.0f + drvAmount * 6.0f;
    ja_inputGain = cpDrive;
    ja_a = 3.0f - satVal * 2.5f;
    ja_a = std::max(0.5f, ja_a);

    float dry = input;

    // Stage 1: SYMMETRIC soft-clip. Was CP3 asymmetric (alpha=2 vs beta=1.44),
    // which is "correct" for the Pluck topology but puts a few-percent DC
    // bias on any symmetric input (Pulse + Square are Pluck's defaults).
    // The DC blocker downstream can only catch static DC — when the envelope
    // modulates cpDrive, the DC bias is modulated too and leaks through the
    // blocker's ~22 ms settling time, audible as a sub-audio pop on every
    // note-on. Symmetric clip eliminates the DC source entirely; the saturation
    // character (soft-knee compression of peaks) is unchanged.
    float x = input * cpDrive;
    float stage1 = x / (1.0f + alpha * std::abs(x));

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

    // Stage 3 was a Jiles-Atherton hysteresis integrator:
    //     M[n] = M[n-1] + (Man - M[n-1]) / (k*delta) * ja_dt
    // The JA model integrates dM/dH, not dM/dt — the discretization should
    // be ` + dM * dH` (change in H), not ` + dM * dt` (change in time).
    // With the dt form and k=0.5, every sample's increment is ~4.5e-5 of
    // the gap to Man, which is a first-order LP with a 500 ms time constant.
    // Result: M effectively never reached Man within a bass note, the wet
    // path stayed near zero, and with mix=1 the driver was *silencing* the
    // signal instead of saturating it. (That's why Pluck sounded "mostly
    // click" — the click was bleed-through and there was no body to speak of.)
    // Switching to dM*dH instead produces an audible buzz on every Pulse-wave
    // edge (M slams to ±1.5 per edge). Neither integration form is right at
    // audio rate, so we collapse stage 3 to the anhysteretic curve directly:
    //   stage3 = Man / Ms = tanh(H / a)
    // That's the same tonal target as the JA loop's steady state, smooth
    // through zero crossings and edges, no integration pathology.
    float stage3 = std::tanh(H / ja_a);
    // Keep H_prev / M_prev maintained for downstream callers that may read
    // them via the IBassDriver interface (currently none, but cheap to do).
    H_prev = H;
    M_prev = stage3 * ja_Ms;

    // Stage 4: De-emphasis only (no head bump for Pluck)
    float deOut = deB0 * stage3 + deB1 * deX1 - deA1 * deY1;
    deX1 = stage3;
    deY1 = deOut;

    // DC blocker — strips the DC offset put on the signal by the asymmetric
    // CP3 clip (stage 1). Without this, a symmetric input (Pulse/Square)
    // exits the driver with a few-percent DC bias that becomes audible as
    // a sub-audio pop when the amp VCA opens it from silence.
    // Standard 1st-order DC-blocker: y[n] = x[n] - x[n-1] + R*y[n-1]
    float wet = deOut - dcX1 + DC_BLOCK_R * dcY1;
    dcX1 = deOut;
    dcY1 = wet;

    return dry + mixVal * (wet - dry);
}
