#include "SymmetricFolder.h"

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float TWO_OVER_PI = 0.63661977236758f;
static constexpr float PI_OVER_TWO = 1.57079632679490f;

SymmetricFolder::SymmetricFolder() = default;

void SymmetricFolder::prepare(double sr, int /*blockSize*/)
{
    driveSm.prepare(sr); satSm.prepare(sr); mixSm.prepare(sr);
}

void SymmetricFolder::reset()
{
    lfoValue = 0.0f;
}

void SymmetricFolder::setDrive(float v)      { driveSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SymmetricFolder::setSaturation(float v)  { satSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SymmetricFolder::setMix(float v)         { mixSm.set(std::max(0.0f, std::min(1.0f, v))); }
void SymmetricFolder::setLFOGain(float lfo)   { lfoValue = std::max(-1.0f, std::min(1.0f, lfo)); }

float SymmetricFolder::processSample(float input)
{
    float driveVal = driveSm.tick();
    float satVal = satSm.tick();
    float mixVal = mixSm.tick();

    // Base drive + LFO modulation of drive depth
    // LFO adds ±50% of drive range when at extremes
    baseDrive = 1.0f + driveVal * 6.0f;
    float modulatedDrive = baseDrive * (1.0f + lfoValue * 0.5f);

    // Fold threshold from saturation: lower sat = higher threshold (gentler)
    // Range: sat 0 → threshold 2.0 (barely folds), sat 1 → threshold 0.3 (aggressive)
    foldThreshold = 2.0f - satVal * 1.7f;
    foldThreshold = std::max(0.3f, foldThreshold);

    float dry = input;

    // Apply drive
    float x = input * modulatedDrive;

    // Symmetric wavefolding: f(x) = (2/π) * arcsin(sin(π/2 * x / thresh))
    // This folds the waveform symmetrically at ±threshold — zero DC offset,
    // odd-harmonic dominant (3rd, 5th, 7th).
    float normalizedX = x / foldThreshold;
    float folded = TWO_OVER_PI * std::asin(std::sin(PI_OVER_TWO * normalizedX));
    folded *= foldThreshold;  // Scale back to original amplitude range
    folded *= 0.5f;          // -6 dB gain compensation to match other drivers

    return dry + mixVal * (folded - dry);
}
