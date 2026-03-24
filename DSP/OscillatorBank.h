#pragma once
#include "BLOscillator.h"
#include "NoiseGenerator.h"

/**
 * Three-oscillator bank inspired by the Minimoog's oscillator section.
 *
 * - OSC1: Primary voice with full waveform selection and octave range.
 * - OSC2: Detunable voice for Reese-style beating and thickening.
 * - OSC3: Sub-oscillator or LFO source (switchable).
 *
 * Each oscillator has independent waveform, range (octave), coarse tune
 * (semitones), and fine tune (cents). The bank also contains a noise
 * generator that can be mixed in.
 */
class OscillatorBank
{
public:
    OscillatorBank();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Render one sample from all three oscillators at the given base frequency.
     * Returns raw oscillator mix (before external mixing stage).
     *
     * @param baseFreqHz  The base note frequency after portamento and pitch envelope.
     * @param osc3AsLFO   If true, OSC3 runs at its own fixed frequency (not tracking keyboard).
     */
    struct OscOutput
    {
        float osc1 = 0.0f;
        float osc2 = 0.0f;
        float osc3 = 0.0f;
        float noise = 0.0f;
    };

    OscOutput tick(double baseFreqHz, bool osc3AsLFO);

    // --- OSC1 parameters ---
    void setOsc1Waveform(BLOscillator::Waveform wf);
    void setOsc1Range(int rangeIndex);         // 0=32', 1=16', 2=8', 3=4', 4=2'
    void setOsc1TuneSemitones(int semitones);
    void setOsc1TuneCents(float cents);

    // --- OSC2 parameters ---
    void setOsc2Waveform(BLOscillator::Waveform wf);
    void setOsc2Range(int rangeIndex);
    void setOsc2TuneSemitones(int semitones);
    void setOsc2Detune(float cents);           // Fine detune from OSC1

    // --- OSC3 parameters ---
    void setOsc3Waveform(BLOscillator::Waveform wf);
    void setOsc3Range(int rangeIndex);
    void setOsc3TuneSemitones(int semitones);
    void setOsc3LFOFrequency(double freqHz);   // Used when OSC3 is in LFO mode

    // --- Noise ---
    void setNoiseType(NoiseGenerator::NoiseType type);

    /** Get OSC3 output when used as an LFO modulation source. */
    float getOsc3LFOValue() const { return lastOsc3Value; }

private:
    /** Convert range index (0-4) to frequency multiplier. */
    static double rangeToMultiplier(int rangeIndex);

    /** Convert semitones + cents offset to frequency ratio. */
    static double tuneToRatio(int semitones, float cents);

    BLOscillator osc1;
    BLOscillator osc2;
    BLOscillator osc3;
    NoiseGenerator noise;

    int osc1Range = 2;  // 8' default
    int osc1Semi = 0;
    float osc1Cents = 0.0f;

    int osc2Range = 2;
    int osc2Semi = 0;
    float osc2Detune = 0.0f;

    int osc3Range = 2;
    int osc3Semi = 0;
    double osc3LFOFreq = 1.0;

    float lastOsc3Value = 0.0f;

    double currentSampleRate = 44100.0;
};
