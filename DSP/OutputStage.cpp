#include "OutputStage.h"
#include <algorithm>

static constexpr float PI_F = 3.14159265358979323846f;

OutputStage::OutputStage() = default;

void OutputStage::prepare(double newSampleRate, int blockSize)
{
    sampleRate_ = newSampleRate;
    tapeSat.prepare(newSampleRate, blockSize);
    compressor.prepare(newSampleRate, blockSize);
    harmonicReverb.prepare(newSampleRate, blockSize);
    updateBassShelf();
    updatePresenceShelf();
}

void OutputStage::reset()
{
    bassX1 = 0.0f;
    bassY1 = 0.0f;
    presX1 = 0.0f;
    presY1 = 0.0f;
    tapeSat.reset();
    compressor.reset();
    harmonicReverb.reset();
}

void OutputStage::setDrive(float amount)
{
    driveAmount = std::max(0.0f, std::min(1.0f, amount));
    driveGain = 1.0f + driveAmount * 7.0f;
}

void OutputStage::setBassShelf(float dB)
{
    float clamped = std::max(-12.0f, std::min(12.0f, dB));
    if (std::abs(clamped - bassGainDB) > 0.01f)
    {
        bassGainDB = clamped;
        updateBassShelf();
    }
}

void OutputStage::setPresenceShelf(float dB)
{
    float clamped = std::max(-6.0f, std::min(6.0f, dB));
    if (std::abs(clamped - presGainDB) > 0.01f)
    {
        presGainDB = clamped;
        updatePresenceShelf();
    }
}

void OutputStage::setStereoWidth(float width)
{
    stereoWidth = std::max(0.0f, std::min(1.0f, width));
}

void OutputStage::setMasterVolume(float level)
{
    masterVolume = std::max(0.0f, std::min(1.0f, level));
}

// --- Tape saturator parameter forwarding ---
void OutputStage::setTapeDrive(float drive)       { tapeSat.setDrive(drive); }
void OutputStage::setTapeSaturation(float sat)    { tapeSat.setSaturation(sat); }
void OutputStage::setTapeBump(float bump)         { tapeSat.setBumpAmount(bump); }
void OutputStage::setTapeMix(float mix)           { tapeSat.setMix(mix); }
void OutputStage::triggerTapeOnset()              { tapeSat.triggerOnset(); }
void OutputStage::setTapeBumpFrequency(float hz)  { tapeSat.setBumpFrequency(hz); }

// --- Compressor parameter forwarding ---
void OutputStage::setCompThreshold(float dBFS)         { compressor.setThreshold(dBFS); }
void OutputStage::setCompTransientAttack(float ms)     { compressor.setTransientAttack(ms); }
void OutputStage::setCompTransientRelease(float ms)    { compressor.setTransientRelease(ms); }
void OutputStage::setCompOpticalRatio(float ratio)     { compressor.setOpticalRatio(ratio); }
void OutputStage::setCompParallelMix(float mix)        { compressor.setParallelMix(mix); }
void OutputStage::setCompOutputGain(float dB)          { compressor.setOutputGain(dB); }

// --- Harmonic Reverb parameter forwarding ---
void OutputStage::setReverbCrossover(float hz)         { harmonicReverb.setCrossoverFreq(hz); }
void OutputStage::setReverbDecay(float amount)         { harmonicReverb.setDecay(amount); }
void OutputStage::setReverbPreDelay(float ms)          { harmonicReverb.setPreDelay(ms); }
void OutputStage::setReverbWet(float wet)              { harmonicReverb.setWet(wet); }
void OutputStage::setReverbEnabled(bool on)            { harmonicReverb.setEnabled(on); }

void OutputStage::updateBassShelf()
{
    if (std::abs(bassGainDB) < 0.01f)
    {
        bassB0 = 1.0f; bassB1 = 0.0f; bassA1 = 0.0f;
        return;
    }

    float G = std::pow(10.0f, bassGainDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float fc = 80.0f;
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate_));

    float a0 = 1.0f + wc / sqrtG;
    bassB0 = (1.0f + wc * sqrtG) / a0;
    bassB1 = (wc * sqrtG - 1.0f) / a0;
    bassA1 = (wc / sqrtG - 1.0f) / a0;
}

void OutputStage::updatePresenceShelf()
{
    if (std::abs(presGainDB) < 0.01f)
    {
        presB0 = 1.0f; presB1 = 0.0f; presA1 = 0.0f;
        return;
    }

    float G = std::pow(10.0f, presGainDB / 20.0f);
    float sqrtG = std::sqrt(G);
    float fc = 3000.0f;
    float wc = std::tan(PI_F * fc / static_cast<float>(sampleRate_));

    float a0 = wc + 1.0f / sqrtG;
    presB0 = (wc + sqrtG) / a0;
    presB1 = (wc - sqrtG) / a0;
    presA1 = (wc - 1.0f / sqrtG) / a0;
}

float OutputStage::process(float input)
{
    float sample = input;

    // --- Drive / Saturation ---
    if (driveAmount > 0.001f)
    {
        sample *= driveGain;
        sample = std::tanh(sample);
        sample *= 1.0f + driveAmount * 0.3f;
    }

    // --- Tape Saturation (after drive, before compressor) ---
    // Gate processing when signal is below noise floor to prevent
    // amplifying floating-point noise through compressor makeup gain
    if (std::abs(sample) > 1.0e-6f)
    {
        sample = tapeSat.processSample(sample);
        sample = compressor.processSample(sample);
    }

    // --- Bass Shelf EQ (80 Hz) ---
    if (std::abs(bassGainDB) > 0.01f)
    {
        float y = bassB0 * sample + bassB1 * bassX1 - bassA1 * bassY1;
        bassX1 = sample;
        bassY1 = y;
        sample = y;
    }

    // --- Presence Shelf EQ (3 kHz) ---
    if (std::abs(presGainDB) > 0.01f)
    {
        float y = presB0 * sample + presB1 * presX1 - presA1 * presY1;
        presX1 = sample;
        presY1 = y;
        sample = y;
    }

    // --- Harmonic Reverb (after EQ, before master vol) ---
    sample = harmonicReverb.processSample(sample);

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
