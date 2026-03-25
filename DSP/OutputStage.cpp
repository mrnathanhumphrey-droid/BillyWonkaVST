#include "OutputStage.h"
#include <algorithm>

static constexpr float  PI_F = 3.14159265358979323846f;
static constexpr double PI_D = 3.14159265358979323846;

OutputStage::OutputStage()
{
    // Default: SubHarmonicExciter (808 is index 0 in the variant, matches Pluck=0 default)
    // But BassMode::Pluck is the default mode, so emplace TransientWaveshaper
    driver.emplace<TransientWaveshaper>();
}

void OutputStage::prepare(double newSampleRate, int blockSize)
{
    sampleRate_ = newSampleRate;
    blockSize_ = blockSize;

    std::visit([&](auto& d) { d.prepare(newSampleRate, blockSize); }, driver);
    compressor.prepare(newSampleRate, blockSize);
    bassReverb.prepare(newSampleRate, blockSize);
    updateEQCoeffs();
    hpfL = {}; hpfR = {}; subL = {}; subR = {};
    fundL = {}; fundR = {}; mudL = {}; mudR = {};
}

void OutputStage::reset()
{
    std::visit([](auto& d) { d.reset(); }, driver);
    hpfL = {}; hpfR = {}; subL = {}; subR = {};
    fundL = {}; fundR = {}; mudL = {}; mudR = {};
    compressor.reset();
    bassReverb.reset();
}

void OutputStage::setDrive(float amount)
{
    driveAmount = std::max(0.0f, std::min(1.0f, amount));
    driveGain = 1.0f + driveAmount * 7.0f;
}

void OutputStage::setStereoWidth(float width)
{
    stereoWidth = std::max(0.0f, std::min(1.0f, width));
}

void OutputStage::setMasterVolume(float level)
{
    masterVolume = std::max(0.0f, std::min(1.0f, level));
}

// =============================================================================
// Per-mode driver forwarding (routes through active variant via IBassDriver)
// =============================================================================

void OutputStage::setTapeDrive(float v)
{
    std::visit([v](auto& d) { d.setDrive(v); }, driver);
}

void OutputStage::setTapeSaturation(float v)
{
    std::visit([v](auto& d) { d.setSaturation(v); }, driver);
}

void OutputStage::setTapeBump(float v)
{
    std::visit([v](auto& d) { d.setBumpAmount(v); }, driver);
}

void OutputStage::setTapeMix(float v)
{
    std::visit([v](auto& d) { d.setMix(v); }, driver);
}

void OutputStage::triggerTapeOnset()
{
    std::visit([](auto& d) { d.triggerOnset(); }, driver);
}

void OutputStage::setTapeBumpFrequency(float hz)
{
    std::visit([hz](auto& d) { d.setBumpFrequency(hz); }, driver);
}

void OutputStage::setDriverEnvelopeGain(float env)
{
    std::visit([env](auto& d) { d.setEnvelopeGain(env); }, driver);
}

void OutputStage::setDriverLFOGain(float lfo)
{
    std::visit([lfo](auto& d) { d.setLFOGain(lfo); }, driver);
}

// =============================================================================
// Drive mode selection
// =============================================================================

void OutputStage::emplaceDriverForBassMode(int bassMode)
{
    switch (bassMode)
    {
        case 0:  // Pluck
            if (!std::holds_alternative<TransientWaveshaper>(driver))
            {
                driver.emplace<TransientWaveshaper>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        case 1:  // 808
            if (!std::holds_alternative<SubHarmonicExciter>(driver))
            {
                driver.emplace<SubHarmonicExciter>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        case 2:  // Reese
            if (!std::holds_alternative<SymmetricFolder>(driver))
            {
                driver.emplace<SymmetricFolder>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        default:
            break;
    }
}

void OutputStage::setDriveMode(DriveMode mode)
{
    currentDriveMode = mode;

    if (mode == DriveMode::Auto)
    {
        userHasOverriddenDriver = false;
        emplaceDriverForBassMode(currentBassMode);
        return;
    }

    userHasOverriddenDriver = true;

    switch (mode)
    {
        case DriveMode::Tape:
            if (!std::holds_alternative<TapeSaturatorDriver>(driver))
            {
                driver.emplace<TapeSaturatorDriver>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        case DriveMode::SubHarmonic:
            if (!std::holds_alternative<SubHarmonicExciter>(driver))
            {
                driver.emplace<SubHarmonicExciter>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        case DriveMode::Transient:
            if (!std::holds_alternative<TransientWaveshaper>(driver))
            {
                driver.emplace<TransientWaveshaper>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        case DriveMode::Symmetric:
            if (!std::holds_alternative<SymmetricFolder>(driver))
            {
                driver.emplace<SymmetricFolder>();
                std::visit([&](auto& d) { d.prepare(sampleRate_, blockSize_); }, driver);
            }
            break;
        default:
            break;
    }
}

void OutputStage::setBassMode(int bassMode)
{
    currentBassMode = bassMode;
    if (!userHasOverriddenDriver)
        emplaceDriverForBassMode(bassMode);
}

// =============================================================================
// Compressor parameter forwarding
// =============================================================================

void OutputStage::setCompThreshold(float dBFS)         { compressor.setThreshold(dBFS); }
void OutputStage::setCompTransientAttack(float ms)     { compressor.setTransientAttack(ms); }
void OutputStage::setCompTransientRelease(float ms)    { compressor.setTransientRelease(ms); }
void OutputStage::setCompOpticalRatio(float ratio)     { compressor.setOpticalRatio(ratio); }
void OutputStage::setCompParallelMix(float mix)        { compressor.setParallelMix(mix); }
void OutputStage::setCompOutputGain(float dB)          { compressor.setOutputGain(dB); }

// =============================================================================
// Bass Reverb parameter forwarding
// =============================================================================

void OutputStage::setReverbEnabled(bool on)            { bassReverb.setEnabled(on); }
void OutputStage::setReverbMode(int modeIndex)         { bassReverb.setMode(modeIndex); }
void OutputStage::setReverbMix(float mix)              { bassReverb.setMix(mix); }
void OutputStage::setReverbDecay(float decay)          { bassReverb.setDecay(decay); }
void OutputStage::setReverbTone(float tone)            { bassReverb.setTone(tone); }

// =============================================================================
// BillyWonka Bass EQ — Parameter Setters
// =============================================================================

void OutputStage::setEQEnabled(bool on)    { eqEnabled = on; }

void OutputStage::setHPFFreq(float hz)     { hpfFreq = std::max(20.0f, std::min(80.0f, hz)); updateEQCoeffs(); }
void OutputStage::setSubFreq(float hz)     { subFreq = std::max(30.0f, std::min(120.0f, hz)); updateEQCoeffs(); }
void OutputStage::setSubGain(float dB)     { subGainDB = std::max(-12.0f, std::min(12.0f, dB)); updateEQCoeffs(); }
void OutputStage::setFundFreq(float hz)    { fundFreq = std::max(60.0f, std::min(250.0f, hz)); updateEQCoeffs(); }
void OutputStage::setFundGain(float dB)    { fundGainDB = std::max(-12.0f, std::min(12.0f, dB)); updateEQCoeffs(); }
void OutputStage::setFundQ(float q)        { fundQ = std::max(0.5f, std::min(4.0f, q)); updateEQCoeffs(); }
void OutputStage::setMudFreq(float hz)     { mudFreq = std::max(150.0f, std::min(600.0f, hz)); updateEQCoeffs(); }
void OutputStage::setMudGain(float dB)     { mudGainDB = std::max(-12.0f, std::min(12.0f, dB)); updateEQCoeffs(); }
void OutputStage::setMudQ(float q)         { mudQ = std::max(0.5f, std::min(3.0f, q)); updateEQCoeffs(); }

// =============================================================================
// EQ Coefficient Computation (all double precision internally)
// =============================================================================

void OutputStage::updateEQCoeffs()
{
    double sr = sampleRate_;
    computeHPF(static_cast<double>(hpfFreq), sr, hpfCoeffs);
    computeLowShelf(static_cast<double>(subFreq), static_cast<double>(subGainDB), sr, subCoeffs);
    computePeaking(static_cast<double>(fundFreq), static_cast<double>(fundGainDB),
                   static_cast<double>(fundQ), sr, fundCoeffs);
    computePeaking(static_cast<double>(mudFreq), static_cast<double>(mudGainDB),
                   static_cast<double>(mudQ), sr, mudCoeffs);
}

void OutputStage::computeHPF(double fc, double sr, BiquadCoeffs& c)
{
    double Q = 0.7071067811865476;
    double wc = std::tan(PI_D * fc / sr);
    double wc2 = wc * wc;
    double norm = 1.0 / (1.0 + wc / Q + wc2);
    c.b0 = static_cast<float>(norm);
    c.b1 = static_cast<float>(-2.0 * norm);
    c.b2 = static_cast<float>(norm);
    c.a1 = static_cast<float>(2.0 * (wc2 - 1.0) * norm);
    c.a2 = static_cast<float>((1.0 - wc / Q + wc2) * norm);
}

void OutputStage::computeLowShelf(double fc, double gainDB, double sr, BiquadCoeffs& c)
{
    if (std::abs(gainDB) < 0.01)
    {
        c = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        return;
    }
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = 2.0 * PI_D * fc / sr;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double sqrtA = std::sqrt(A);
    double alpha = sinw0 * 0.5 * std::sqrt(2.0 * (A + 1.0 / A));
    double Ap1 = A + 1.0, Am1 = A - 1.0;
    double twoSqrtAalpha = 2.0 * sqrtA * alpha;
    double a0 = Ap1 + Am1 * cosw0 + twoSqrtAalpha;
    double inv_a0 = 1.0 / a0;
    c.b0 = static_cast<float>(A * (Ap1 - Am1 * cosw0 + twoSqrtAalpha) * inv_a0);
    c.b1 = static_cast<float>(2.0 * A * (Am1 - Ap1 * cosw0) * inv_a0);
    c.b2 = static_cast<float>(A * (Ap1 - Am1 * cosw0 - twoSqrtAalpha) * inv_a0);
    c.a1 = static_cast<float>(-2.0 * (Am1 + Ap1 * cosw0) * inv_a0);
    c.a2 = static_cast<float>((Ap1 + Am1 * cosw0 - twoSqrtAalpha) * inv_a0);
}

void OutputStage::computePeaking(double fc, double gainDB, double Q,
                                  double sr, BiquadCoeffs& c)
{
    if (std::abs(gainDB) < 0.01)
    {
        c = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        return;
    }
    double A = std::pow(10.0, gainDB / 40.0);
    double w0 = 2.0 * PI_D * fc / sr;
    double cosw0 = std::cos(w0);
    double sinw0 = std::sin(w0);
    double alpha = sinw0 / (2.0 * Q);
    double a0 = 1.0 + alpha / A;
    double inv_a0 = 1.0 / a0;
    c.b0 = static_cast<float>((1.0 + alpha * A) * inv_a0);
    c.b1 = static_cast<float>((-2.0 * cosw0) * inv_a0);
    c.b2 = static_cast<float>((1.0 - alpha * A) * inv_a0);
    c.a1 = static_cast<float>((-2.0 * cosw0) * inv_a0);
    c.a2 = static_cast<float>((1.0 - alpha / A) * inv_a0);
}

// =============================================================================
// Process
// =============================================================================

float OutputStage::process(float input)
{
    float sample = input;

    // --- Drive / Saturation (tanh soft clip) ---
    if (driveAmount > 0.001f)
    {
        sample *= driveGain;
        sample = std::tanh(sample);
        sample *= 1.0f + driveAmount * 0.3f;
    }

    // --- Per-mode bass driver (via std::variant) ---
    // Gate processing when signal is below noise floor to prevent
    // amplifying floating-point noise through compressor makeup gain
    if (std::abs(sample) > 1.0e-6f)
    {
        sample = std::visit([sample](auto& d) { return d.processSample(sample); }, driver);
        sample = compressor.processSample(sample);
    }

    // --- BillyWonka Bass EQ (after compressor, before reverb) ---
    if (eqEnabled)
    {
        sample = processBiquad(sample, hpfCoeffs,  hpfL);
        sample = processBiquad(sample, subCoeffs,  subL);
        sample = processBiquad(sample, fundCoeffs, fundL);
        sample = processBiquad(sample, mudCoeffs,  mudL);
    }

    // --- Bass Reverb (after EQ, before master vol) ---
    sample = bassReverb.processSample(sample);

    // --- Master Volume ---
    sample *= masterVolume;

    return sample;
}

void OutputStage::applyStereoWidth(float& left, float& right)
{
    if (stereoWidth < 0.001f)
    {
        float mono = (left + right) * 0.5f;
        left = mono;
        right = mono;
        return;
    }
    float mid = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;
    left  = mid + side * stereoWidth;
    right = mid - side * stereoWidth;
}
