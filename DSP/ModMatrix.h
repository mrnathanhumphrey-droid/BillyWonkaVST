#pragma once

/**
 * Modulation Matrix.
 *
 * Routes modulation sources to destinations with configurable amounts.
 * Keeps the modulation routing decoupled from the individual DSP components,
 * allowing flexible patching without refactoring core modules.
 *
 * Sources: LFO, Filter Envelope, Amplitude Envelope, Pitch Envelope,
 *          Velocity, Mod Wheel, OSC3-as-LFO.
 *
 * Destinations: Filter Cutoff, OSC Pitch (all), Amplitude, Drive.
 *
 * The matrix collects all source values, applies routing and scaling,
 * and outputs a set of modulation offsets that are added to the base
 * parameter values by SynthEngine.
 */
class ModMatrix
{
public:
    // Modulation source identifiers
    enum class Source
    {
        LFO = 0,
        FilterEnvelope,
        AmpEnvelope,
        PitchEnvelope,
        Velocity,
        ModWheel,
        Osc3LFO,
        NumSources
    };

    // Modulation destination identifiers
    enum class Destination
    {
        FilterCutoff = 0,
        Pitch,
        Amplitude,
        Drive,
        NumDestinations
    };

    // Input: all current source values
    struct SourceValues
    {
        float lfo = 0.0f;
        float filterEnv = 0.0f;
        float ampEnv = 0.0f;
        float pitchEnv = 0.0f;
        float velocity = 1.0f;
        float modWheel = 0.0f;
        float osc3LFO = 0.0f;
    };

    // Output: modulation offsets to add to base parameter values
    struct ModOutputs
    {
        float filterCutoffMod = 0.0f;   // Additive Hz offset
        float pitchMod = 0.0f;          // Additive semitones offset
        float ampMod = 0.0f;            // Multiplicative gain factor (centered at 1.0)
        float driveMod = 0.0f;          // Additive drive offset
    };

    ModMatrix();

    void prepare(double sampleRate, int blockSize);
    void reset();

    /**
     * Process all modulation routes and return the combined outputs.
     *
     * @param sources  Current values of all modulation sources.
     * @return Combined modulation offsets for each destination.
     */
    ModOutputs process(const SourceValues& sources) const;

    // --- Primary LFO routing (controlled by the main LFO target parameter) ---

    /** Set the LFO's primary target destination. */
    void setLFOTarget(Destination dest);

    /** Set the filter envelope modulation depth (bipolar, -1 to +1). */
    void setFilterEnvAmount(float amount);

    // --- Additional fixed routes ---

    /** Set mod wheel → filter cutoff depth. */
    void setModWheelToFilterAmount(float amount);

    /** Set velocity → amplitude sensitivity (0 = none, 1 = full). */
    void setVelocitySensitivity(float amount);

private:
    // Primary LFO routing target
    Destination lfoTarget = Destination::FilterCutoff;

    // Fixed route amounts
    float filterEnvAmount = 0.5f;         // Filter envelope → filter cutoff
    float modWheelToFilter = 0.0f;        // Mod wheel → filter cutoff
    float velocitySensitivity = 0.5f;     // Velocity → amplitude
};
