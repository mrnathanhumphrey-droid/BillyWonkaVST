#include "BassReverb.h"

constexpr float BassReverb::modeHPF[4];
constexpr float BassReverb::HallEngine::erGains[7];

BassReverb::BassReverb() = default;

// =============================================================================
// Prepare / Reset
// =============================================================================

void BassReverb::prepare(double sampleRate, int /*blockSize*/)
{
    sampleRate_ = sampleRate;
    rateRatio = sampleRate_ / static_cast<double>(REF_RATE);

    room.prepare(sampleRate_, rateRatio);
    plate.prepare(sampleRate_, rateRatio);
    spring.prepare(sampleRate_, rateRatio);
    hall.prepare(sampleRate_, rateRatio);

    mixSm.prepare(sampleRate_, 30.0f);
    mixSm.snap(0.0f);
    decaySm.prepare(sampleRate_, 30.0f);
    decaySm.snap(decayParam);
    toneSm.prepare(sampleRate_, 30.0f);
    toneSm.snap(toneParam);

    updateCrossover();
    updateFeedbackForMode();
    updateDampingForMode();
}

void BassReverb::reset()
{
    flushAllBuffers();
    lpState = 0.0f;
    reverbWasActive = false;
}

void BassReverb::flushAllBuffers()
{
    room.reset();
    plate.reset();
    spring.reset();
    hall.reset();
}

// =============================================================================
// Parameter Setters
// =============================================================================

void BassReverb::setEnabled(bool on) { enabled = on; }

void BassReverb::setMode(int modeIndex)
{
    Mode newMode = static_cast<Mode>(std::max(0, std::min(3, modeIndex)));
    if (newMode != currentMode)
    {
        currentMode = newMode;
        hpfCutoff = modeHPF[modeIndex];
        updateCrossover();
        updateFeedbackForMode();
        updateDampingForMode();
    }
}

void BassReverb::setMix(float mix)
{
    mixSm.set(std::max(0.0f, std::min(1.0f, mix)));
}

void BassReverb::setDecay(float decay)
{
    decayParam = std::max(0.1f, std::min(1.0f, decay));
    decaySm.set(decayParam);
    updateFeedbackForMode();
}

void BassReverb::setTone(float tone)
{
    toneParam = std::max(0.0f, std::min(1.0f, tone));
    toneSm.set(toneParam);
    updateDampingForMode();
}

// =============================================================================
// Crossover
// =============================================================================

void BassReverb::updateCrossover()
{
    float wc = 2.0f * PI_F * hpfCutoff / static_cast<float>(sampleRate_);
    lpCoeff = std::exp(-wc);
}

// =============================================================================
// Per-mode feedback and damping
// =============================================================================

void BassReverb::updateFeedbackForMode()
{
    switch (currentMode)
    {
        case Mode::Room:
            // Base feedback 0.84 * decay multiplier
            room.setFeedback(0.84f * decayParam);
            break;
        case Mode::Plate:
            plate.setFeedback(0.5f + decayParam * 0.45f);
            break;
        case Mode::Spring:
            spring.setFeedback(0.3f + decayParam * 0.45f);
            break;
        case Mode::Hall:
            hall.setFeedback(decayParam);
            break;
    }
}

void BassReverb::updateDampingForMode()
{
    // Tone 0.0 = 800Hz (dark), 1.0 = 10000Hz (bright)
    float dampFreq = 800.0f + toneParam * 9200.0f;
    float sr = static_cast<float>(sampleRate_);

    switch (currentMode)
    {
        case Mode::Room:   room.setDamping(dampFreq, sr);   break;
        case Mode::Plate:  plate.setDamping(dampFreq, sr);  break;
        case Mode::Spring: spring.setDamping(dampFreq, sr); break;
        case Mode::Hall:   hall.setDamping(dampFreq, sr);   break;
    }
}

// =============================================================================
// Main Processing
// =============================================================================

float BassReverb::processSample(float input)
{
    // --- Bypass checks (toggle first, then mix) ---
    float mix = mixSm.tick();

    if (!enabled || (mix == 0.0f && mixSm.target == 0.0f))
    {
        // Flush on transition to bypass
        if (reverbWasActive)
        {
            flushAllBuffers();
            reverbWasActive = false;
        }
        return input;
    }

    reverbWasActive = true;

    // --- Crossover HPF: separate sub from harmonics ---
    lpState = (1.0f - lpCoeff) * input + lpCoeff * lpState;
    float lowPath = lpState;           // Sub/fundamental (always dry)
    float highPath = input - lowPath;  // Harmonics (reverbed)

    // --- Process active reverb mode ---
    float reverbOut = 0.0f;
    switch (currentMode)
    {
        case Mode::Room:   reverbOut = room.process(highPath);   break;
        case Mode::Plate:  reverbOut = plate.process(highPath);  break;
        case Mode::Spring: reverbOut = spring.process(highPath); break;
        case Mode::Hall:   reverbOut = hall.process(highPath);   break;
    }

    // --- Wet/dry blend on harmonic path only ---
    float highOut = highPath * (1.0f - mix) + reverbOut * mix;

    // --- Recombine: dry sub + blended harmonics ---
    return lowPath + highOut;
}

// =============================================================================
// ROOM ENGINE — Schroeder-Moorer
// =============================================================================

void BassReverb::RoomEngine::prepare(double sr, double ratio)
{
    static constexpr int combRef[4] = { 1557, 1617, 1491, 1422 };
    static constexpr int apRef[2] = { 225, 556 };

    for (int i = 0; i < 4; ++i)
    {
        int len = std::max(1, static_cast<int>(combRef[i] * ratio + 0.5));
        combs[i].delay.setLength(len);
        combs[i].delay.length = len;
    }
    for (int i = 0; i < 2; ++i)
    {
        int len = std::max(1, static_cast<int>(apRef[i] * ratio + 0.5));
        allpasses[i].delay.setLength(len);
        allpasses[i].delay.length = len;
    }

    // Pre-delay: 8ms
    int pdLen = static_cast<int>(0.008 * sr + 0.5);
    preDelay.setLength(pdLen);
    preDelay.length = pdLen;
}

void BassReverb::RoomEngine::reset()
{
    for (int i = 0; i < 4; ++i) combs[i].clear();
    for (int i = 0; i < 2; ++i) allpasses[i].clear();
    preDelay.clear();
}

float BassReverb::RoomEngine::process(float input)
{
    float pd = preDelay.readAndWrite(input);

    float combSum = 0.0f;
    for (int i = 0; i < 4; ++i)
        combSum += combs[i].process(pd);
    combSum *= 0.25f;

    float out = combSum;
    for (int i = 0; i < 2; ++i)
        out = allpasses[i].process(out);

    return out;
}

void BassReverb::RoomEngine::setFeedback(float fb)
{
    for (int i = 0; i < 4; ++i)
        combs[i].feedback = fb;
}

void BassReverb::RoomEngine::setDamping(float freqHz, float sr)
{
    for (int i = 0; i < 4; ++i)
        combs[i].damp.setCutoff(freqHz, sr);
}

// =============================================================================
// PLATE ENGINE — Dattorro
// =============================================================================

void BassReverb::PlateEngine::prepare(double sr, double ratio)
{
    static constexpr int inAPRef[4] = { 142, 107, 379, 277 };
    static constexpr int tankAPRef[2] = { 672, 908 };
    static constexpr int tankDelRef[2] = { 3163, 3720 };

    for (int i = 0; i < 4; ++i)
    {
        int len = std::max(1, static_cast<int>(inAPRef[i] * ratio + 0.5));
        inputAPs[i].delay.setLength(len);
        inputAPs[i].delay.length = len;
        inputAPs[i].coeff = 0.75f;
    }

    for (int i = 0; i < 2; ++i)
    {
        int apLen = std::max(1, static_cast<int>(tankAPRef[i] * ratio + 0.5));
        tanks[i].apDelay.setLength(apLen);
        tanks[i].apDelay.length = apLen;

        int delLen = std::max(1, static_cast<int>(tankDelRef[i] * ratio + 0.5));
        tanks[i].lineDelay.setLength(delLen);
        tanks[i].lineDelay.length = delLen;

        tanks[i].apCoeff = 0.5f;
    }

    // LFO: ~0.1Hz
    lfoInc = static_cast<float>(0.1 * 2.0 * 3.14159265358979 / sr);
    lfoPhase = 0.0f;
    lfoDepth = 8.0f;
}

void BassReverb::PlateEngine::reset()
{
    for (int i = 0; i < 4; ++i) inputAPs[i].clear();
    for (int i = 0; i < 2; ++i) tanks[i].clear();
    lfoPhase = 0.0f;
}

float BassReverb::PlateEngine::process(float input)
{
    // Input diffusion
    float diff = input;
    for (int i = 0; i < 4; ++i)
        diff = inputAPs[i].process(diff);

    // LFO for tank modulation
    float mod0 = std::sin(lfoPhase) * lfoDepth;
    float mod1 = std::cos(lfoPhase) * lfoDepth;
    lfoPhase += lfoInc;
    if (lfoPhase > 6.283185307f) lfoPhase -= 6.283185307f;

    // Tank processing (cross-feedback)
    float t0out = tanks[0].process(diff + tanks[1].feedback * tanks[1].lineDelay.read(tanks[1].lineDelay.length), mod0);
    float t1out = tanks[1].process(diff + t0out, mod1);

    // Update tank feedback for next iteration
    tanks[0].feedback = t0out / (std::abs(t0out) + 1.0f);  // Soft limit
    tanks[0].feedback = 0.0f;  // Reset — actual feedback comes from setFeedback

    return (t0out + t1out) * 0.5f;
}

void BassReverb::PlateEngine::setFeedback(float fb)
{
    for (int i = 0; i < 2; ++i)
        tanks[i].feedback = fb;
}

void BassReverb::PlateEngine::setDamping(float freqHz, float sr)
{
    for (int i = 0; i < 2; ++i)
        tanks[i].damp.setCutoff(freqHz, sr);
}

// =============================================================================
// SPRING ENGINE — Dispersive waveguide
// =============================================================================

void BassReverb::SpringEngine::prepare(double sr, double ratio)
{
    int cohLen = std::max(1, static_cast<int>(4800 * ratio + 0.5));
    coherentDelay.setLength(cohLen);
    coherentDelay.length = cohLen;

    // Dispersive allpass poles distributed between 0.1 and 0.9
    float poleStep = 0.8f / 7.0f;
    for (int i = 0; i < 8; ++i)
        disperseAPs[i].pole = 0.1f + static_cast<float>(i) * poleStep;

    // Wash LFO: ~0.3Hz
    washLfoInc = static_cast<float>(0.3 * 2.0 * 3.14159265358979 / sr);
    washLfoPhase = 0.0f;
    washDepth = 12.0f;

    int washLen = std::max(1, static_cast<int>(100 * ratio + 0.5));
    washDelay.setLength(washLen);
    washDelay.length = washLen;
}

void BassReverb::SpringEngine::reset()
{
    coherentDelay.clear();
    coherentDamp.clear();
    coherentState = 0.0f;
    for (int i = 0; i < 8; ++i) disperseAPs[i].clear();
    washDelay.clear();
    washLfoPhase = 0.0f;
}

float BassReverb::SpringEngine::process(float input)
{
    // Coherent path: recirculating delay with damping
    float cohRead = coherentDelay.read(coherentDelay.length);
    float cohDamped = coherentDamp.process(cohRead);
    coherentDelay.write(input + cohDamped * coherentFeedback + DENORMAL_DC);

    // Dispersive path: cascade of 8 allpass filters (creates chirp)
    float dispersed = input;
    for (int i = 0; i < 8; ++i)
        dispersed = disperseAPs[i].process(dispersed);

    // Wash modulation
    float washMod = std::sin(washLfoPhase) * washDepth;
    washLfoPhase += washLfoInc;
    if (washLfoPhase > 6.283185307f) washLfoPhase -= 6.283185307f;

    float washRead = washDelay.readInterp(static_cast<float>(washDelay.length / 2) + washMod);
    washDelay.write(dispersed + DENORMAL_DC);
    dispersed = washRead;

    // Mix coherent + dispersive 60/40
    return cohRead * 0.6f + dispersed * 0.4f;
}

void BassReverb::SpringEngine::setFeedback(float fb)
{
    coherentFeedback = fb;
}

void BassReverb::SpringEngine::setDamping(float freqHz, float sr)
{
    coherentDamp.setCutoff(freqHz, sr);
}

// =============================================================================
// HALL ENGINE — 8-line FDN with Hadamard matrix
// =============================================================================

void BassReverb::HallEngine::prepare(double sr, double ratio)
{
    static constexpr int lineRef[NUM_LINES] = { 1021, 1327, 1559, 1811, 2089, 2351, 2617, 2843 };

    for (int i = 0; i < NUM_LINES; ++i)
    {
        int len = std::max(1, static_cast<int>(lineRef[i] * ratio + 0.5));
        lines[i].setLength(len);
        lines[i].length = len;
        lineState[i] = 0.0f;
    }

    // Early reflections taps: 18, 29, 41, 57, 73, 89, 107 ms
    static constexpr float erMs[ER_TAPS] = { 18.0f, 29.0f, 41.0f, 57.0f, 73.0f, 89.0f, 107.0f };
    int maxErSamples = 0;
    for (int i = 0; i < ER_TAPS; ++i)
    {
        erTapSamples[i] = static_cast<int>(erMs[i] * 0.001f * sr + 0.5f);
        if (erTapSamples[i] > maxErSamples) maxErSamples = erTapSamples[i];
    }
    erLine.setLength(maxErSamples + 1);
    erLine.length = maxErSamples + 1;

    // Pre-delay: 42ms
    int pdLen = static_cast<int>(0.042 * sr + 0.5);
    preDelay.setLength(pdLen);
    preDelay.length = pdLen;

    crossoverFreq = 800.0f;
}

void BassReverb::HallEngine::reset()
{
    for (int i = 0; i < NUM_LINES; ++i)
    {
        lines[i].clear();
        dampLow[i].clear();
        dampHigh[i].clear();
        lineState[i] = 0.0f;
    }
    erLine.clear();
    preDelay.clear();
}

void BassReverb::HallEngine::hadamardMix(const float in[NUM_LINES], float out[NUM_LINES])
{
    // 8x8 Hadamard (Walsh-ordered) scaled by 1/sqrt(8)
    // H8 is recursive: H2 ⊗ H4, but unrolled for speed
    // Stage 1: butterfly pairs
    float a[8];
    a[0] = in[0] + in[1]; a[1] = in[0] - in[1];
    a[2] = in[2] + in[3]; a[3] = in[2] - in[3];
    a[4] = in[4] + in[5]; a[5] = in[4] - in[5];
    a[6] = in[6] + in[7]; a[7] = in[6] - in[7];

    // Stage 2
    float b[8];
    b[0] = a[0] + a[2]; b[1] = a[1] + a[3];
    b[2] = a[0] - a[2]; b[3] = a[1] - a[3];
    b[4] = a[4] + a[6]; b[5] = a[5] + a[7];
    b[6] = a[4] - a[6]; b[7] = a[5] - a[7];

    // Stage 3
    out[0] = (b[0] + b[4]) * INV_SQRT8;
    out[1] = (b[1] + b[5]) * INV_SQRT8;
    out[2] = (b[2] + b[6]) * INV_SQRT8;
    out[3] = (b[3] + b[7]) * INV_SQRT8;
    out[4] = (b[0] - b[4]) * INV_SQRT8;
    out[5] = (b[1] - b[5]) * INV_SQRT8;
    out[6] = (b[2] - b[6]) * INV_SQRT8;
    out[7] = (b[3] - b[7]) * INV_SQRT8;
}

float BassReverb::HallEngine::process(float input)
{
    // Pre-delay
    float pd = preDelay.readAndWrite(input);

    // Early reflections
    erLine.write(pd);
    float er = 0.0f;
    for (int i = 0; i < ER_TAPS; ++i)
        er += erLine.read(erTapSamples[i]) * erGains[i];
    er *= 0.25f;  // Normalize

    // FDN: read delay lines
    float delayOuts[NUM_LINES];
    for (int i = 0; i < NUM_LINES; ++i)
        delayOuts[i] = lines[i].read(lines[i].length);

    // Hadamard mixing
    float mixed[NUM_LINES];
    hadamardMix(delayOuts, mixed);

    // Per-band attenuation + write back
    for (int i = 0; i < NUM_LINES; ++i)
    {
        // Split into low/high bands via one-pole crossover
        float low = dampLow[i].process(mixed[i]);
        float high = mixed[i] - low;

        // Apply per-band feedback gains
        float fb = low * fbLow + high * fbHigh;

        // Write input + feedback into delay line
        lines[i].write(pd + fb + DENORMAL_DC);
    }

    // Output: sum first 4 lines (mono output)
    float out = 0.0f;
    for (int i = 0; i < NUM_LINES; ++i)
        out += delayOuts[i];
    out *= 0.125f;  // 1/8

    return er + out;
}

void BassReverb::HallEngine::setFeedback(float decayMult)
{
    // Low band T60 = 2.8s * decayMult, High band T60 = 1.4s * decayMult
    // Feedback gain = 10^(-3 * delayTime / T60)
    // Use average delay time (~1950 samples at 44100 = ~44ms)
    float avgDelaySec = 0.044f;

    float t60Low = 2.8f * decayMult;
    float t60High = 1.4f * decayMult;

    fbLow = (t60Low > 0.01f) ? std::pow(10.0f, -3.0f * avgDelaySec / t60Low) : 0.0f;
    fbHigh = (t60High > 0.01f) ? std::pow(10.0f, -3.0f * avgDelaySec / t60High) : 0.0f;

    fbLow = std::min(fbLow, 0.999f);
    fbHigh = std::min(fbHigh, 0.999f);
}

void BassReverb::HallEngine::setDamping(float toneFreqHz, float sr)
{
    for (int i = 0; i < NUM_LINES; ++i)
    {
        dampLow[i].setCutoff(crossoverFreq, sr);
        dampHigh[i].setCutoff(toneFreqHz, sr);
    }
}
