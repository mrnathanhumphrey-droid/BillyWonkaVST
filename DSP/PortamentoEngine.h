#pragma once
#include <cmath>

/**
 * Portamento / Glide Engine.
 *
 * Provides smooth exponential pitch interpolation between notes,
 * essential for RnB bass performance:
 * - 808 sliding glide (80-120ms)
 * - Pluck melodic slides (50-150ms)
 * - Reese legato transitions (150-200ms)
 *
 * Uses exponential interpolation:
 *   currentFreq += (targetFreq - currentFreq) * glideCoeff
 *
 * This produces a musically natural pitch slide where the rate of change
 * slows as it approaches the target (like an analog slew limiter).
 *
 * Supports legato mode: glide only when a new note is played while
 * the previous note is still held.
 */
class PortamentoEngine
{
public:
    PortamentoEngine();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Advance portamento by one sample and return the current frequency.
     * If glide is disabled or complete, returns the target frequency directly.
     */
    double tick();

    /**
     * Set a new target note frequency.
     * @param freqHz    Target frequency in Hz.
     * @param isLegato  True if previous note was still held when this note started.
     */
    void setTarget(double freqHz, bool isLegato);

    /** Enable or disable portamento. */
    void setEnabled(bool enabled);

    /** Set glide time in seconds (0 to 0.5). */
    void setGlideTime(float seconds);

    /** Set legato-only mode: if true, glide only occurs on legato transitions. */
    void setLegatoMode(bool legatoOnly);

    /** Get current interpolated frequency. */
    double getCurrentFrequency() const { return currentFreq; }

    /** Snap immediately to a frequency (no glide). Used for initial note. */
    void snapToFrequency(double freqHz);

private:
    void updateCoefficient();

    double sampleRate = 44100.0;

    double currentFreq = 440.0;
    double targetFreq = 440.0;

    float glideTime = 0.08f;    // 80ms default
    float glideCoeff = 1.0f;    // Per-sample interpolation coefficient
    bool enabled = false;
    bool legatoMode = true;     // Only glide on legato by default
    bool isGliding = false;

    // Convergence threshold: when current is within this ratio of target, snap
    static constexpr double SNAP_THRESHOLD = 0.0001;
};
