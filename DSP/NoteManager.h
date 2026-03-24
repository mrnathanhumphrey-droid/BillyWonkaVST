#pragma once
#include <cstdint>

/**
 * Monophonic Voice & MIDI Manager.
 *
 * Handles all MIDI input parsing and monophonic voice management for
 * the synthesizer. As a monophonic synth (like the Minimoog), only one
 * note sounds at a time. When multiple notes are held, the most recent
 * note takes priority (last-note priority), and releasing it returns to
 * the previously held note if still active.
 *
 * Features:
 * - Last-note priority voice stealing
 * - Legato detection (for portamento triggering)
 * - Note stack (up to 16 held notes) for recall on release
 * - Pitch bend parsing (14-bit, ±2 semitone default range)
 * - Mod wheel (CC 1) parsing
 * - All CC parsing available for future extension
 */
class NoteManager
{
public:
    NoteManager();

    void prepare(double sampleRate, int blockSize);
    void reset();

    // --- MIDI event handling (called from processBlock MIDI parsing) ---

    /** Handle a MIDI note-on event. */
    void noteOn(int noteNumber, float velocity);

    /** Handle a MIDI note-off event. */
    void noteOff(int noteNumber);

    /** Handle a pitch bend message (14-bit value: 0 to 16383, center = 8192). */
    void pitchBend(int value);

    /** Handle a CC message. */
    void controlChange(int ccNumber, int ccValue);

    /** Call once per block to clear per-block flags. */
    void prepareForBlock();

    // --- State queries ---

    /** Is a note currently active (gate open)? */
    bool isNoteActive() const { return currentNote >= 0; }

    /** Get the current MIDI note number (-1 if none). */
    int getCurrentNote() const { return currentNote; }

    /** Get the velocity of the current note (0.0 to 1.0). */
    float getCurrentVelocity() const { return currentVelocity; }

    /** Was the current note triggered as a legato transition?
     *  (i.e., previous note was still held when this note started) */
    bool isLegato() const { return legato; }

    /** Was a new note triggered this block? */
    bool wasNoteTriggered() const { return noteTriggeredThisBlock; }

    /** Was note released this block? */
    bool wasNoteReleased() const { return noteReleasedThisBlock; }

    /** Get normalized pitch bend value (-1.0 to +1.0). */
    float getPitchBend() const { return pitchBendNormalized; }

    /** Get pitch bend in semitones (using current bend range). */
    float getPitchBendSemitones() const { return pitchBendNormalized * pitchBendRange; }

    /** Get mod wheel value (0.0 to 1.0, CC 1). */
    float getModWheel() const { return modWheel; }

    /** Set pitch bend range in semitones (default ±2). */
    void setPitchBendRange(float semitones);

    /** Convert a MIDI note number to frequency in Hz (A4 = 440 Hz). */
    static double noteToFrequency(int midiNote);

private:
    // Note stack for last-note priority
    static constexpr int MAX_NOTES = 16;

    struct NoteEntry
    {
        int note = -1;
        float velocity = 0.0f;
        bool active = false;
    };

    NoteEntry noteStack[MAX_NOTES];
    int stackSize = 0;

    int currentNote = -1;
    int previousNote = -1;
    float currentVelocity = 0.0f;
    bool legato = false;

    // Per-block event flags
    bool noteTriggeredThisBlock = false;
    bool noteReleasedThisBlock = false;

    // Controllers
    float pitchBendNormalized = 0.0f;   // -1 to +1
    float pitchBendRange = 2.0f;        // ±2 semitones default
    float modWheel = 0.0f;              // 0 to 1

    /** Push a note onto the stack. */
    void pushNote(int noteNumber, float velocity);

    /** Remove a note from the stack. */
    void removeNote(int noteNumber);

    /** Find the top (most recent) note in the stack. Returns -1 if empty. */
    int topNote() const;

    /** Get the velocity of the top note. */
    float topVelocity() const;
};
