#pragma once
#include <cmath>
#include <variant>
#include "DriveMode.h"
#include "SubHarmonicExciter.h"
#include "TransientWaveshaper.h"
#include "SymmetricFolder.h"
#include "TapeSaturatorDriver.h"
#include "RnBCompressor.h"
#include "BassReverb.h"

// =========================================================================
// Biquad primitives — Direct Form II Transposed
// =========================================================================

struct BiquadCoeffs { float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; };
struct BiquadState  { float s1 = 0.0f, s2 = 0.0f; };

inline float processBiquad(float x, const BiquadCoeffs& c, BiquadState& s)
{
    float y = c.b0 * x + s.s1;
    s.s1 = c.b1 * x - c.a1 * y + s.s2;
    s.s2 = c.b2 * x - c.a2 * y;
    return y;
}

/**
 * Output Stage.
 *
 * Final processing chain after the amplitude envelope:
 * 1. Drive / Saturation — soft clipper using tanh() waveshaping.
 * 2. Per-mode bass driver (IBassDriver via std::variant, no heap).
 * 3. RnBCompressor — 3-stage serial + parallel compression.
 * 4. BillyWonka Bass EQ — 4-stage 8-pole cascaded biquad parametric EQ.
 * 5. BassReverb — 4-mode frequency-split reverb.
 * 6. Master volume.
 * 7. Stereo width — mid/side matrix.
 *
 * Reverb DSP is fully bypassed and delay buffers are flushed when Mix = 0.0
 * or when the reverb toggle is off.
 */
class OutputStage
{
public:
    using DriverVariant = std::variant<
        SubHarmonicExciter,
        TransientWaveshaper,
        SymmetricFolder,
        TapeSaturatorDriver
    >;

    OutputStage();

    void prepare(double sampleRate, int blockSize);
    void reset();

    float process(float input);

    void applyStereoWidth(float& left, float& right);

    void setDrive(float amount);           // 0.0 to 1.0
    void setStereoWidth(float width);      // 0.0 (mono) to 1.0 (wide)
    void setMasterVolume(float level);     // 0.0 to 1.0

    // Per-mode bass driver — routed through active variant
    void setTapeDrive(float drive);
    void setTapeSaturation(float sat);
    void setTapeBump(float bump);
    void setTapeMix(float mix);
    void triggerTapeOnset();
    void setTapeBumpFrequency(float frequencyHz);
    void setDriverEnvelopeGain(float env);
    void setDriverLFOGain(float lfo);

    // Drive mode selection
    void setDriveMode(DriveMode mode);
    void setBassMode(int bassMode);        // 0=Pluck, 1=808, 2=Reese
    DriveMode getDriveMode() const { return currentDriveMode; }

    // Compressor
    void setCompThreshold(float dBFS);
    void setCompTransientAttack(float ms);
    void setCompTransientRelease(float ms);
    void setCompOpticalRatio(float ratio);
    void setCompParallelMix(float mix);
    void setCompOutputGain(float dB);

    // BillyWonka Bass EQ
    void setEQEnabled(bool on);
    void setHPFFreq(float hz);
    void setSubFreq(float hz);     void setSubGain(float dB);
    void setFundFreq(float hz);    void setFundGain(float dB);
    void setFundQ(float q);
    void setMudFreq(float hz);     void setMudGain(float dB);
    void setMudQ(float q);

    // Bass Reverb
    void setReverbEnabled(bool on);
    void setReverbMode(int modeIndex);
    void setReverbMix(float mix);
    void setReverbDecay(float decay);
    void setReverbTone(float tone);

    float getMasterVolume() const { return masterVolume; }

private:
    // EQ coefficient computation helpers (double precision internally)
    void updateEQCoeffs();
    void computeHPF(double fc, double sr, BiquadCoeffs& c);
    void computeLowShelf(double fc, double gainDB, double sr, BiquadCoeffs& c);
    void computePeaking(double fc, double gainDB, double Q, double sr, BiquadCoeffs& c);

    /** Emplace the correct driver variant for a given BassMode index. */
    void emplaceDriverForBassMode(int bassMode);

    double sampleRate_ = 44100.0;
    int blockSize_ = 512;

    // Drive
    float driveAmount = 0.0f;
    float driveGain = 1.0f;

    // Per-mode driver (all four held by value — no heap)
    DriverVariant driver;
    DriveMode currentDriveMode = DriveMode::Auto;
    bool userHasOverriddenDriver = false;
    int currentBassMode = 0;  // Cached for Auto re-evaluation

    // Compressor + Bass Reverb
    RnBCompressor compressor;
    BassReverb bassReverb;

    // BillyWonka Bass EQ — 4-stage 8-pole parametric
    bool eqEnabled = true;

    float hpfFreq    = 30.0f;
    float subFreq    = 60.0f;   float subGainDB  = 3.0f;
    float fundFreq   = 100.0f;  float fundGainDB = 2.0f;  float fundQ = 1.5f;
    float mudFreq    = 280.0f;  float mudGainDB  = -3.0f; float mudQ  = 1.2f;

    BiquadCoeffs hpfCoeffs, subCoeffs, fundCoeffs, mudCoeffs;
    BiquadState  hpfL, hpfR, subL, subR, fundL, fundR, mudL, mudR;

    // Stereo width
    float stereoWidth = 0.0f;

    // Master volume
    float masterVolume = 0.8f;
};
