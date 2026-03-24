#include "RnBCompressor.h"

RnBCompressor::RnBCompressor() = default;

void RnBCompressor::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;

    // Stage 1 coefficients
    s1_attackCoeff = msToCoeff(transientAttackMs);
    s1_releaseCoeff = msToCoeff(transientReleaseMs);

    // Stage 2 coefficients (fixed time constants)
    s2_fastCoeff = msToCoeff(30.0f);
    s2_slowCoeff = msToCoeff(300.0f);

    // Stage 3 coefficients (fixed)
    s3_attackCoeff = msToCoeff(5.0f);
    s3_releaseCoeff = msToCoeff(200.0f);

    // Smoothers
    parallelMixSm.prepare(sampleRate, 20.0f);
    parallelMixSm.snap(0.45f);
    outputGainSm.prepare(sampleRate, 20.0f);
    outputGainSm.snap(outputGainLin);
}

void RnBCompressor::reset()
{
    s1_envelope = 0.0f;
    s2_fastEnv = 0.0f;
    s2_slowEnv = 0.0f;
    s2_prevLevel = 0.0f;
    s3_envelope = 0.0f;
}

float RnBCompressor::msToCoeff(float ms) const
{
    float timeSeconds = ms * 0.001f;
    return std::exp(-1.0f / (timeSeconds * static_cast<float>(sampleRate)));
}

// =============================================================================
// Parameter Setters
// =============================================================================

void RnBCompressor::setThreshold(float dBFS)
{
    thresholdDB = std::max(-30.0f, std::min(0.0f, dBFS));
    thresholdLin = dBToLinear(thresholdDB);
}

void RnBCompressor::setTransientAttack(float ms)
{
    transientAttackMs = std::max(10.0f, std::min(120.0f, ms));
    s1_attackCoeff = msToCoeff(transientAttackMs);
}

void RnBCompressor::setTransientRelease(float ms)
{
    transientReleaseMs = std::max(20.0f, std::min(300.0f, ms));
    s1_releaseCoeff = msToCoeff(transientReleaseMs);
}

void RnBCompressor::setOpticalRatio(float ratio)
{
    opticalRatio = std::max(4.0f, std::min(8.0f, ratio));
}

void RnBCompressor::setParallelMix(float mix)
{
    parallelMixSm.set(std::max(0.0f, std::min(1.0f, mix)));
}

void RnBCompressor::setOutputGain(float dB)
{
    outputGainLin = dBToLinear(std::max(-6.0f, std::min(12.0f, dB)));
    outputGainSm.set(outputGainLin);
}

// =============================================================================
// Processing
// =============================================================================

float RnBCompressor::processSample(float input)
{
    float absInput = std::abs(input);

    // =========================================================================
    // STAGE 1: Transient VCA (slow attack, preserves punch)
    // =========================================================================
    // Feed-forward peak detector with asymmetric attack/release
    if (absInput > s1_envelope)
        s1_envelope = s1_attackCoeff * s1_envelope + (1.0f - s1_attackCoeff) * absInput;
    else
        s1_envelope = s1_releaseCoeff * s1_envelope + (1.0f - s1_releaseCoeff) * absInput;

    // Compute gain reduction in dB domain
    float s1_envDB = linearToDB(s1_envelope);
    float s1_gainReduction = 0.0f;
    if (s1_envDB > thresholdDB)
    {
        float overDB = s1_envDB - thresholdDB;
        // Gain reduction = over - over/ratio = over * (1 - 1/ratio)
        s1_gainReduction = overDB * (1.0f - 1.0f / s1_ratio);
    }

    // Apply stage 1 gain reduction
    float s1_gainLin = dBToLinear(-s1_gainReduction);
    float afterStage1 = input * s1_gainLin;

    // =========================================================================
    // STAGE 2: Optical/program-dependent compressor (body levelling)
    // =========================================================================
    float absAfterS1 = std::abs(afterStage1);
    float afterS1_DB = linearToDB(absAfterS1);

    // Dual time-constant envelope follower
    // Update both fast and slow envelopes
    if (absAfterS1 > s2_fastEnv)
        s2_fastEnv = s2_fastEnv + (1.0f - s2_fastCoeff) * (absAfterS1 - s2_fastEnv);
    else
        s2_fastEnv = s2_fastCoeff * s2_fastEnv;

    if (absAfterS1 > s2_slowEnv)
        s2_slowEnv = s2_slowEnv + (1.0f - s2_slowCoeff) * (absAfterS1 - s2_slowEnv);
    else
        s2_slowEnv = s2_slowCoeff * s2_slowEnv;

    // Blend based on rate of level change (program-dependent behavior)
    float levelDelta = std::abs(afterS1_DB - s2_prevLevel);
    s2_prevLevel = afterS1_DB;

    // Fast blend when signal is changing rapidly, slow when settled
    float blendFactor = std::min(1.0f, levelDelta / 0.5f);  // Normalized 0-1
    float s2_envelope = s2_fastEnv * blendFactor + s2_slowEnv * (1.0f - blendFactor);

    // Compute optical gain reduction
    float s2_envDB = linearToDB(s2_envelope);
    float s2_gainReduction = 0.0f;
    if (s2_envDB > thresholdDB)
    {
        float overDB = s2_envDB - thresholdDB;
        s2_gainReduction = overDB * (1.0f - 1.0f / opticalRatio);
    }

    float s2_gainLin = dBToLinear(-s2_gainReduction);
    float afterStage2 = afterStage1 * s2_gainLin;

    // =========================================================================
    // STAGE 3: Parallel blend
    // =========================================================================
    // Heavy parallel compressor on post-stage-2 signal
    float absAfterS2 = std::abs(afterStage2);

    if (absAfterS2 > s3_envelope)
        s3_envelope = s3_attackCoeff * s3_envelope + (1.0f - s3_attackCoeff) * absAfterS2;
    else
        s3_envelope = s3_releaseCoeff * s3_envelope + (1.0f - s3_releaseCoeff) * absAfterS2;

    float s3_envDB = linearToDB(s3_envelope);
    float s3_gainReduction = 0.0f;
    if (s3_envDB > thresholdDB)
    {
        float overDB = s3_envDB - thresholdDB;
        s3_gainReduction = overDB * (1.0f - 1.0f / s3_ratio);
    }

    float s3_gainLin = dBToLinear(-s3_gainReduction);
    float heavyCompressed = afterStage2 * s3_gainLin;

    // Blend: dry (post-stage-1 transient snap) + wet (heavily compressed density)
    float mix = parallelMixSm.tick();
    float blended = afterStage1 * (1.0f - mix) + heavyCompressed * mix;

    // Makeup gain
    float gain = outputGainSm.tick();
    return blended * gain;
}
