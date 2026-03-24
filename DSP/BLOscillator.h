#pragma once
#include <cmath>
#include <cstdint>

/**
 * Band-Limited Oscillator using PolyBLEP anti-aliasing.
 *
 * Generates Sine, Triangle, Sawtooth, Reverse Sawtooth, Square,
 * and Variable-width Pulse waveforms without aliasing artifacts.
 *
 * PolyBLEP (Polynomial Band-Limited Step) works by adding a polynomial
 * correction near waveform discontinuities, smoothing the step transitions
 * that would otherwise produce aliasing.
 */
class BLOscillator
{
public:
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Sawtooth,
        ReverseSaw,
        Square,
        Pulse
    };

    BLOscillator();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /** Advance phase and return the next sample. */
    float tick();

    /** Set frequency in Hz. Recomputes phase increment. */
    void setFrequency(double freqHz);

    /** Set waveform type. */
    void setWaveform(Waveform type);

    /** Set amplitude (0.0 to 1.0). */
    void setAmplitude(float amp);

    /** Set pulse width for Pulse waveform (0.01 to 0.99). */
    void setPulseWidth(float pw);

    /** Get current frequency. */
    double getFrequency() const { return frequency; }

    /** Get current waveform. */
    Waveform getWaveform() const { return waveform; }

private:
    /** PolyBLEP correction applied near discontinuities. */
    float polyBLEP(float t) const;

    double sampleRate = 44100.0;
    double frequency = 440.0;
    double phase = 0.0;           // 0.0 to 1.0 (normalized)
    double phaseIncrement = 0.0;
    float amplitude = 1.0f;
    float pulseWidth = 0.5f;
    Waveform waveform = Waveform::Sawtooth;

    // For triangle generation via integrated square wave
    float triangleState = 0.0f;
};
