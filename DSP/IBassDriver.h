#pragma once

/**
 * IBassDriver — Abstract interface for per-mode bass drive/saturation engines.
 *
 * Four concrete implementations:
 *   SubHarmonicExciter   — 808: even-order poly shaper, soft JA, note-tracked bump
 *   TransientWaveshaper  — Pluck: envelope-gated CP3 asymmetric clip, no bump
 *   SymmetricFolder      — Reese: symmetric fold/clip, no JA, LFO-coupled drive
 *   TapeSaturatorDriver  — All modes: full MoogTapeSaturator (CP3 → JA → bump)
 *
 * OutputStage holds a std::variant of all four by value (no heap).
 * SynthEngine routes per-sample envelope and LFO values via setEnvelopeGain()
 * and setLFOGain() for drivers that use them; others implement as no-ops.
 *
 * Pure C++ — no JUCE dependencies.
 */
class IBassDriver
{
public:
    virtual ~IBassDriver() = default;

    /** Called once when sample rate or block size changes. Recalculate coefficients here. */
    virtual void prepare(double sampleRate, int blockSize) = 0;

    /** Zero all internal state (filter states, integrators, delay lines, magnetization). */
    virtual void reset() = 0;

    /** Process a single sample through the drive chain. */
    virtual float processSample(float input) = 0;

    /** Called on note-on. Bleed stateful memory and ramp input to prevent pops. */
    virtual void triggerOnset() = 0;

    /** Overall drive amount, 0–1. Meaning is driver-specific. */
    virtual void setDrive(float drive) = 0;

    /** Saturation depth, 0–1. Controls nonlinearity intensity (JA ja_a, fold threshold, etc). */
    virtual void setSaturation(float sat) = 0;

    /** Head bump amount, 0–1. Low-frequency resonant boost. No-op on drivers without bump. */
    virtual void setBumpAmount(float bump) = 0;

    /** Wet/dry mix, 0–1. 0 = fully dry (bypass), 1 = fully wet. */
    virtual void setMix(float mix) = 0;

    /** Note-track the head bump frequency. Called per-note in 808 mode. */
    virtual void setBumpFrequency(float hz) = 0;

    /** Per-sample amplitude envelope level, 0–1. Used by TransientWaveshaper to gate drive. */
    virtual void setEnvelopeGain(float env) = 0;

    /** Per-sample LFO value, -1 to +1. Used by SymmetricFolder for drive depth modulation. */
    virtual void setLFOGain(float lfo) = 0;
};
