#pragma once
#include "IBassDriver.h"
#include "MoogTapeSaturator.h"

/**
 * TapeSaturatorDriver — Thin IBassDriver adaptor around MoogTapeSaturator.
 *
 * Full 4-stage Moog tape chain: CP3 pre-clip → NAB pre-emphasis →
 * Jiles-Atherton hysteresis → de-emphasis + head bump.
 *
 * setEnvelopeGain() and setLFOGain() are no-ops — the tape saturator
 * is statically driven, which is intentional and part of its character.
 *
 * Best on: 808 slides at moderate drive, slow-attack Pluck patches,
 * any mode where the user wants explicit 80 Hz head bump character.
 * Not recommended as default for Reese (DC bias with detuned saws).
 */
class TapeSaturatorDriver final : public IBassDriver
{
public:
    void prepare(double sr, int bs) override { sat.prepare(sr, bs); }
    void reset()                    override { sat.reset(); }
    float processSample(float x)    override { return sat.processSample(x); }
    void triggerOnset()             override { sat.triggerOnset(); }
    void setDrive(float v)          override { sat.setDrive(v); }
    void setSaturation(float v)     override { sat.setSaturation(v); }
    void setBumpAmount(float v)     override { sat.setBumpAmount(v); }
    void setMix(float v)            override { sat.setMix(v); }
    void setBumpFrequency(float hz) override { sat.setBumpFrequency(hz); }
    void setEnvelopeGain(float)     override { /* no-op: JA is not envelope-gated */ }
    void setLFOGain(float)          override { /* no-op: drive is not LFO-coupled */ }

private:
    MoogTapeSaturator sat;
};
