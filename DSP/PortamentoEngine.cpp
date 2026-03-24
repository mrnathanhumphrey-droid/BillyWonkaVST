#include "PortamentoEngine.h"
#include <algorithm>

PortamentoEngine::PortamentoEngine() = default;

void PortamentoEngine::prepare(double newSampleRate, int /*blockSize*/)
{
    sampleRate = newSampleRate;
    updateCoefficient();
}

void PortamentoEngine::reset()
{
    currentFreq = 440.0;
    targetFreq = 440.0;
    isGliding = false;
}

void PortamentoEngine::updateCoefficient()
{
    // Compute per-sample coefficient for exponential interpolation.
    // glideCoeff determines what fraction of the remaining distance
    // is covered each sample.
    //
    // For a glide time T, we want ~99% convergence in T seconds:
    //   (1 - coeff)^(T * sampleRate) ≈ 0.01
    //   coeff ≈ 1 - exp(ln(0.01) / (T * sampleRate))
    //
    // A glideTime of 0 means instant (no glide).
    if (glideTime <= 0.0001f)
    {
        glideCoeff = 1.0f;  // Instant snap
    }
    else
    {
        double numSamples = static_cast<double>(glideTime) * sampleRate;
        glideCoeff = static_cast<float>(1.0 - std::exp(std::log(0.01) / numSamples));
    }
}

void PortamentoEngine::setEnabled(bool on)
{
    enabled = on;
}

void PortamentoEngine::setGlideTime(float seconds)
{
    glideTime = std::max(0.0f, std::min(0.5f, seconds));
    updateCoefficient();
}

void PortamentoEngine::setLegatoMode(bool legatoOnly)
{
    legatoMode = legatoOnly;
}

void PortamentoEngine::snapToFrequency(double freqHz)
{
    currentFreq = freqHz;
    targetFreq = freqHz;
    isGliding = false;
}

void PortamentoEngine::setTarget(double freqHz, bool isLegato)
{
    targetFreq = freqHz;

    if (!enabled)
    {
        // No glide — snap immediately
        currentFreq = freqHz;
        isGliding = false;
        return;
    }

    if (legatoMode && !isLegato)
    {
        // Legato mode is on but this is not a legato transition — no glide
        currentFreq = freqHz;
        isGliding = false;
        return;
    }

    // Start gliding from current frequency to target
    isGliding = true;
}

double PortamentoEngine::tick()
{
    if (!isGliding)
        return currentFreq;

    // Exponential interpolation toward target
    currentFreq += (targetFreq - currentFreq) * static_cast<double>(glideCoeff);

    // Check if we've converged
    double ratio = std::abs(currentFreq - targetFreq) / targetFreq;
    if (ratio < SNAP_THRESHOLD)
    {
        currentFreq = targetFreq;
        isGliding = false;
    }

    return currentFreq;
}
