#pragma once
#include <cmath>
#include "MoogTapeSaturator.h"
#include "RnBCompressor.h"
#include "HarmonicReverb.h"

/**
 * Output Stage.
 *
 * Final processing chain after the amplitude envelope:
 * 1. Drive / Saturation — soft clipper using tanh() waveshaping.
 * 2. Tape Saturation — MoogTapeSaturator (CP3 pre-clip, hysteresis, head bump).
 * 3. Bass shelf EQ — boost/cut around 80 Hz.
 * 4. Presence shelf EQ — boost/cut around 3 kHz.
 * 5. Stereo width — mid/side matrix for mono-to-wide control.
 * 6. Master volume.
 *
 * Shelf EQs use first-order IIR filters in Direct Form I,
 * with proper input and output state tracking.
 */
class OutputStage
{
public:
    OutputStage();

    void prepare(double sampleRate, int blockSize);
    void reset();

    float process(float input);

    void applyStereoWidth(float& left, float& right);

    void setDrive(float amount);           // 0.0 to 1.0
    void setBassShelf(float dB);           // -12 to +12 dB at 80 Hz
    void setPresenceShelf(float dB);       // -6 to +6 dB at 3000 Hz
    void setStereoWidth(float width);      // 0.0 (mono) to 1.0 (wide)
    void setMasterVolume(float level);     // 0.0 to 1.0

    // Tape saturator parameters
    void setTapeDrive(float drive);
    void setTapeSaturation(float sat);
    void setTapeBump(float bump);
    void setTapeMix(float mix);

    /** Trigger onset ramp on note-on (bleeds residual magnetization). */
    void triggerTapeOnset();

    /** Retune head bump to track note frequency (808 mode). */
    void setTapeBumpFrequency(float frequencyHz);

    // Compressor
    void setCompThreshold(float dBFS);
    void setCompTransientAttack(float ms);
    void setCompTransientRelease(float ms);
    void setCompOpticalRatio(float ratio);
    void setCompParallelMix(float mix);
    void setCompOutputGain(float dB);

    // Harmonic Reverb
    void setReverbCrossover(float hz);
    void setReverbDecay(float amount);
    void setReverbPreDelay(float ms);
    void setReverbWet(float wet);
    void setReverbEnabled(bool on);

    float getMasterVolume() const { return masterVolume; }

private:
    void updateBassShelf();
    void updatePresenceShelf();

    double sampleRate_ = 44100.0;

    // Drive
    float driveAmount = 0.0f;
    float driveGain = 1.0f;

    // Tape saturation
    MoogTapeSaturator tapeSat;

    // Compressor + Harmonic Reverb
    RnBCompressor compressor;
    HarmonicReverb harmonicReverb;

    // First-order shelf filter: y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
    // Bass shelf (80 Hz)
    float bassGainDB = 0.0f;
    float bassB0 = 1.0f, bassB1 = 0.0f, bassA1 = 0.0f;
    float bassX1 = 0.0f;
    float bassY1 = 0.0f;

    // Presence shelf (3 kHz)
    float presGainDB = 0.0f;
    float presB0 = 1.0f, presB1 = 0.0f, presA1 = 0.0f;
    float presX1 = 0.0f;
    float presY1 = 0.0f;

    // Stereo width
    float stereoWidth = 0.0f;

    // Master volume
    float masterVolume = 0.8f;
};
