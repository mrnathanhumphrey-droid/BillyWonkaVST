#include "OscillatorBank.h"
#include <cmath>

OscillatorBank::OscillatorBank() = default;

void OscillatorBank::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    osc1.prepare(sampleRate, blockSize);
    osc2.prepare(sampleRate, blockSize);
    osc3.prepare(sampleRate, blockSize);
    noise.prepare(sampleRate, blockSize);
}

void OscillatorBank::reset()
{
    osc1.reset();
    osc2.reset();
    osc3.reset();
    noise.reset();
    lastOsc3Value = 0.0f;
}

double OscillatorBank::rangeToMultiplier(int rangeIndex)
{
    // 32' = lowest (0.25x of 8'), 16' = 0.5x, 8' = 1x, 4' = 2x, 2' = 4x
    // Range index: 0=32', 1=16', 2=8', 3=4', 4=2'
    switch (rangeIndex)
    {
        case 0: return 0.25;   // 32'
        case 1: return 0.5;    // 16'
        case 2: return 1.0;    // 8'  (reference)
        case 3: return 2.0;    // 4'
        case 4: return 4.0;    // 2'
        default: return 1.0;
    }
}

double OscillatorBank::tuneToRatio(int semitones, float cents)
{
    // Convert semitones + cents to a frequency ratio using equal temperament
    double totalSemitones = static_cast<double>(semitones) + static_cast<double>(cents) / 100.0;
    return std::pow(2.0, totalSemitones / 12.0);
}

OscillatorBank::OscOutput OscillatorBank::tick(double baseFreqHz, bool osc3AsLFO)
{
    OscOutput out;

    // OSC1: base frequency * range * tune offset
    double osc1Freq = baseFreqHz * rangeToMultiplier(osc1Range) * tuneToRatio(osc1Semi, osc1Cents);
    osc1.setFrequency(osc1Freq);
    out.osc1 = osc1.tick();

    // OSC2: base frequency * range * tune offset (includes detune from OSC1)
    double osc2Freq = baseFreqHz * rangeToMultiplier(osc2Range) * tuneToRatio(osc2Semi, osc2Detune);
    osc2.setFrequency(osc2Freq);
    out.osc2 = osc2.tick();

    // OSC3: either tracks keyboard or runs as LFO at fixed frequency
    if (osc3AsLFO)
    {
        osc3.setFrequency(osc3LFOFreq);
    }
    else
    {
        double osc3Freq = baseFreqHz * rangeToMultiplier(osc3Range) * tuneToRatio(osc3Semi, 0.0f);
        osc3.setFrequency(osc3Freq);
    }
    out.osc3 = osc3.tick();
    lastOsc3Value = out.osc3;

    // Noise
    out.noise = noise.tick();

    return out;
}

// --- OSC1 setters ---
void OscillatorBank::setOsc1Waveform(BLOscillator::Waveform wf)  { osc1.setWaveform(wf); }
void OscillatorBank::setOsc1Range(int rangeIndex)                 { osc1Range = rangeIndex; }
void OscillatorBank::setOsc1TuneSemitones(int semitones)          { osc1Semi = semitones; }
void OscillatorBank::setOsc1TuneCents(float cents)                { osc1Cents = cents; }

// --- OSC2 setters ---
void OscillatorBank::setOsc2Waveform(BLOscillator::Waveform wf)  { osc2.setWaveform(wf); }
void OscillatorBank::setOsc2Range(int rangeIndex)                 { osc2Range = rangeIndex; }
void OscillatorBank::setOsc2TuneSemitones(int semitones)          { osc2Semi = semitones; }
void OscillatorBank::setOsc2Detune(float cents)                   { osc2Detune = cents; }

// --- OSC3 setters ---
void OscillatorBank::setOsc3Waveform(BLOscillator::Waveform wf)  { osc3.setWaveform(wf); }
void OscillatorBank::setOsc3Range(int rangeIndex)                 { osc3Range = rangeIndex; }
void OscillatorBank::setOsc3TuneSemitones(int semitones)          { osc3Semi = semitones; }
void OscillatorBank::setOsc3LFOFrequency(double freqHz)           { osc3LFOFreq = freqHz; }

// --- Noise ---
void OscillatorBank::setNoiseType(NoiseGenerator::NoiseType type) { noise.setType(type); }
