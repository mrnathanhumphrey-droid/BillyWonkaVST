#pragma once

/**
 * DriveMode — Selects which IBassDriver implementation is active in OutputStage.
 *
 * Auto follows the current BassMode default. Explicit selections override
 * the auto-mapping and persist across mode changes until reset to Auto.
 */
enum class DriveMode
{
    Auto = 0,       // Follows BassMode default (clears userHasOverriddenDriver)
    Tape,           // TapeSaturatorDriver — full 4-stage Moog tape chain
    SubHarmonic,    // SubHarmonicExciter — even-order poly, 808-voiced
    Transient,      // TransientWaveshaper — envelope-gated CP3, Pluck-voiced
    Symmetric       // SymmetricFolder — odd-harmonic fold, Reese-voiced
};
