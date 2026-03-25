#pragma once
#include <cmath>
#include <algorithm>
#include <cstring>

/**
 * BassReverb — Four-mode frequency-split reverb for R&B bass.
 *
 * Modes:
 *   Room   — Schroeder-Moorer (4 damped comb + 2 allpass), HPF 220Hz
 *   Plate  — Dattorro tank (4 input allpass + modulated tank), HPF 200Hz
 *   Spring — Dispersive waveguide (coherent + allpass cascade chirp), HPF 180Hz
 *   Hall   — 8-line FDN with Hadamard matrix + early reflections, HPF 300Hz
 *
 * Signal flow:
 *   input → HPF crossover → low(dry) + high(reverb engine) → wet/dry mix → recombine
 *
 * Reverb DSP is fully bypassed and delay buffers are flushed when Mix = 0.0
 * or when the reverb toggle is off.
 *
 * Pure C++ — no JUCE dependencies. Zero heap allocation in process().
 */
class BassReverb
{
public:
    enum class Mode { Room = 0, Plate, Spring, Hall };

    BassReverb();

    void prepare(double sampleRate, int blockSize);
    void reset();

    float processSample(float input);

    void setEnabled(bool on);
    void setMode(int modeIndex);           // 0=Room, 1=Plate, 2=Spring, 3=Hall
    void setMix(float mix);                // 0.0–1.0
    void setDecay(float decay);            // 0.1–1.0, scales feedback gain
    void setTone(float tone);              // 0.0–1.0, dark to bright damping

private:
    static constexpr float PI_F = 3.14159265358979323846f;
    static constexpr int REF_RATE = 44100;
    static constexpr float DENORMAL_DC = 1.0e-25f;

    // --- One-pole smoother ---
    struct Smooth
    {
        float current = 0.0f, target = 0.0f, coeff = 0.999f;
        void prepare(double sr, float timeMs = 20.0f)
        {
            float samples = static_cast<float>(sr) * timeMs * 0.001f;
            coeff = (samples > 1.0f) ? std::exp(-2.302585f / samples) : 0.0f;
        }
        void set(float v) { target = v; }
        void snap(float v) { current = v; target = v; }
        float tick() { current = current * coeff + target * (1.0f - coeff); return current; }
        bool isSettled() const { return std::abs(current - target) < 1.0e-7f; }
    };

    // --- Generic delay line (fixed max size, no heap) ---
    template <int MAX_LEN>
    struct DelayLine
    {
        float buffer[MAX_LEN] = {};
        int writeIdx = 0;
        int length = 0;

        void clear() { std::memset(buffer, 0, sizeof(buffer)); writeIdx = 0; }

        void setLength(int len) { length = std::min(len, MAX_LEN - 1); }

        void write(float sample)
        {
            buffer[writeIdx] = sample;
            if (++writeIdx >= MAX_LEN) writeIdx = 0;
        }

        float read(int delaySamples) const
        {
            int idx = writeIdx - delaySamples;
            if (idx < 0) idx += MAX_LEN;
            return buffer[idx];
        }

        float readAndWrite(float input)
        {
            float out = read(length);
            write(input);
            return out;
        }

        // Modulated read with linear interpolation
        float readInterp(float delaySamples) const
        {
            int intPart = static_cast<int>(delaySamples);
            float frac = delaySamples - static_cast<float>(intPart);
            int idx0 = writeIdx - intPart;
            int idx1 = idx0 - 1;
            if (idx0 < 0) idx0 += MAX_LEN;
            if (idx1 < 0) idx1 += MAX_LEN;
            return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
        }
    };

    // --- One-pole lowpass (damping filter) ---
    struct OnePoleLP
    {
        float state = 0.0f;
        float g = 0.5f;  // coefficient

        void setCutoff(float freqHz, float sr)
        {
            float w = 2.0f * PI_F * freqHz / sr;
            g = 1.0f - std::exp(-w);
        }
        void clear() { state = 0.0f; }

        float process(float input)
        {
            state += g * (input - state);
            return state;
        }
    };

    // Scale reference samples to current sample rate
    int scale(int refSamples) const
    {
        return std::max(1, static_cast<int>(refSamples * rateRatio + 0.5));
    }
    int msToSamples(float ms) const
    {
        return std::max(0, static_cast<int>(ms * 0.001f * static_cast<float>(sampleRate_) + 0.5f));
    }

    // =========================================================================
    // Crossover HPF (first-order)
    // =========================================================================
    void updateCrossover();
    float hpfCutoff = 220.0f;
    float lpCoeff = 0.0f;
    float lpState = 0.0f;

    // =========================================================================
    // ROOM — Schroeder-Moorer
    // =========================================================================
    struct RoomEngine
    {
        static constexpr int MAX_COMB = 3500;
        static constexpr int MAX_AP = 1300;

        struct DampedComb
        {
            DelayLine<3500> delay;
            OnePoleLP damp;
            float feedback = 0.84f;

            float process(float input)
            {
                float delayed = delay.read(delay.length);
                float damped = damp.process(delayed);
                delay.write(input + damped * feedback + DENORMAL_DC);
                return delayed;
            }
            void clear() { delay.clear(); damp.clear(); }
        };

        struct Allpass
        {
            DelayLine<1300> delay;
            static constexpr float coeff = 0.5f;

            float process(float input)
            {
                float delayed = delay.read(delay.length);
                float output = -input + delayed;
                delay.write(input + delayed * coeff + DENORMAL_DC);
                return output;
            }
            void clear() { delay.clear(); }
        };

        DampedComb combs[4];
        Allpass allpasses[2];
        DelayLine<4800> preDelay;  // max ~50ms at 96kHz

        void prepare(double sr, double ratio);
        void reset();
        float process(float input);
        void setFeedback(float fb);
        void setDamping(float freqHz, float sr);
    } room;

    // =========================================================================
    // PLATE — Dattorro tank
    // =========================================================================
    struct PlateEngine
    {
        // Input diffusion: 4 allpass filters
        struct InputAP
        {
            DelayLine<1000> delay;
            float coeff = 0.75f;

            float process(float input)
            {
                float delayed = delay.read(delay.length);
                float v = input - delayed * coeff;
                delay.write(v + DENORMAL_DC);
                return delayed + v * coeff;
            }
            void clear() { delay.clear(); }
        };

        InputAP inputAPs[4];

        // Tank: 2 interleaved allpass + delay loops with modulation
        struct TankSection
        {
            DelayLine<5000> apDelay;    // tank allpass
            DelayLine<5000> lineDelay;  // tank delay line
            OnePoleLP damp;
            float apCoeff = 0.5f;
            float feedback = 0.5f;      // Tank feedback

            float process(float input, float modOffset = 0.0f)
            {
                // Allpass
                float apRead = apDelay.readInterp(static_cast<float>(apDelay.length) + modOffset);
                float v = input - apRead * apCoeff;
                apDelay.write(v + DENORMAL_DC);
                float apOut = apRead + v * apCoeff;

                // Delay line with damping
                float lineRead = lineDelay.read(lineDelay.length);
                float damped = damp.process(lineRead);
                lineDelay.write(apOut + DENORMAL_DC);

                return damped * feedback;
            }
            void clear() { apDelay.clear(); lineDelay.clear(); damp.clear(); }
        };

        TankSection tanks[2];

        // LFO for tank modulation
        float lfoPhase = 0.0f;
        float lfoInc = 0.0f;
        float lfoDepth = 8.0f;

        void prepare(double sr, double ratio);
        void reset();
        float process(float input);
        void setFeedback(float fb);
        void setDamping(float freqHz, float sr);
    } plate;

    // =========================================================================
    // SPRING — Dispersive waveguide
    // =========================================================================
    struct SpringEngine
    {
        // Coherent path: recirculating delay
        DelayLine<12000> coherentDelay;
        OnePoleLP coherentDamp;
        float coherentFeedback = 0.6f;
        float coherentState = 0.0f;

        // Dispersive path: cascade of 8 first-order allpass filters
        struct DispersiveAP
        {
            float state = 0.0f;
            float pole = 0.5f;

            float process(float input)
            {
                float output = state + (input - state) * (1.0f - pole);
                state = input * pole + output * (1.0f - pole);
                // True allpass: y = -x + (1+a)*state
                float y = -input + (1.0f + pole) * state;
                state = input + pole * y;
                return y;
            }
            void clear() { state = 0.0f; }
        };

        DispersiveAP disperseAPs[8];

        // Wash modulation
        DelayLine<200> washDelay;
        float washLfoPhase = 0.0f;
        float washLfoInc = 0.0f;
        float washDepth = 12.0f;

        void prepare(double sr, double ratio);
        void reset();
        float process(float input);
        void setFeedback(float fb);
        void setDamping(float freqHz, float sr);
    } spring;

    // =========================================================================
    // HALL — 8-line FDN with Hadamard matrix
    // =========================================================================
    struct HallEngine
    {
        static constexpr int NUM_LINES = 8;
        static constexpr float INV_SQRT8 = 0.35355339059327f;  // 1/sqrt(8)

        // Main FDN delay lines
        DelayLine<7000> lines[NUM_LINES];
        OnePoleLP dampLow[NUM_LINES];   // Low-band attenuation
        OnePoleLP dampHigh[NUM_LINES];  // High-band attenuation
        float lineState[NUM_LINES] = {};

        // Per-band T60 feedback gains
        float fbLow = 0.0f;
        float fbHigh = 0.0f;

        // Early reflections: FIR tapped delay line
        static constexpr int ER_TAPS = 7;
        DelayLine<12000> erLine;
        int erTapSamples[ER_TAPS] = {};
        static constexpr float erGains[ER_TAPS] = { 0.9f, 0.7f, 0.6f, 0.5f, 0.4f, 0.35f, 0.3f };

        // Pre-delay
        DelayLine<5000> preDelay;

        // Crossover freq for per-band decay
        float crossoverFreq = 800.0f;

        void prepare(double sr, double ratio);
        void reset();
        float process(float input);
        void setFeedback(float decayMult);
        void setDamping(float toneFreqHz, float sr);

        // Hadamard multiply: out[i] = sum(H[i][j] * in[j]) / sqrt(8)
        void hadamardMix(const float in[NUM_LINES], float out[NUM_LINES]);
    } hall;

    // =========================================================================
    // State
    // =========================================================================
    double sampleRate_ = 44100.0;
    double rateRatio = 1.0;

    bool enabled = false;
    Mode currentMode = Mode::Room;
    float decayParam = 0.5f;
    float toneParam = 0.5f;

    Smooth mixSm;
    Smooth decaySm;
    Smooth toneSm;

    bool reverbWasActive = false;  // For flush-on-bypass

    // Per-mode HPF cutoffs
    static constexpr float modeHPF[4] = { 220.0f, 200.0f, 180.0f, 300.0f };

    void updateDampingForMode();
    void updateFeedbackForMode();
    void flushAllBuffers();
};
