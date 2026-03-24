#pragma once
#include <cmath>

/**
 * LFO (Low Frequency Oscillator) Engine.
 *
 * Provides cyclic modulation for filter cutoff, pitch, and amplitude.
 * Supports three waveform shapes: Triangle, Square, and Sine.
 *
 * Features:
 * - Free-running rate (0.01 to 20 Hz) or DAW tempo sync.
 * - Depth control (0 to 1) scales the output.
 * - Bipolar output (-1 to +1) for symmetric modulation.
 * - Phase reset on note-on (optional, disabled by default for organic feel).
 */
class LFOEngine
{
public:
    enum class Waveform
    {
        Triangle = 0,
        Square,
        Sine
    };

    enum class Target
    {
        FilterCutoff = 0,
        Pitch,
        Amplitude
    };

    LFOEngine();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Advance LFO by one sample and return the current value.
     * Returns a bipolar value in [-depth, +depth].
     */
    float tick();

    void setRate(float hz);
    void setDepth(float d);
    void setWaveform(Waveform wf);
    void setTarget(Target t);

    /** Enable tempo sync. When synced, rate is interpreted as a beat division. */
    void setTempoSync(bool sync);

    /** Set DAW tempo in BPM (used when tempo sync is enabled). */
    void setTempoBPM(double bpm);

    Waveform getWaveform() const { return waveform; }
    Target getTarget() const { return target; }
    float getRate() const { return rate; }
    float getDepth() const { return depth; }

    /** Reset phase to zero (call on note-on if phase reset is desired). */
    void resetPhase();

private:
    double sampleRate = 44100.0;

    float rate = 1.0f;          // Hz (or beat division when synced)
    float depth = 0.0f;         // Modulation depth [0, 1]
    Waveform waveform = Waveform::Triangle;
    Target target = Target::FilterCutoff;

    double phase = 0.0;          // 0.0 to 1.0 (normalized)
    double phaseIncrement = 0.0;

    bool tempoSync = false;
    double tempoBPM = 120.0;
};
