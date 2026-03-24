#include "NoteManager.h"
#include <cmath>
#include <algorithm>

NoteManager::NoteManager() = default;

void NoteManager::prepare(double /*sampleRate*/, int /*blockSize*/)
{
    reset();
}

void NoteManager::reset()
{
    for (int i = 0; i < MAX_NOTES; ++i)
        noteStack[i] = NoteEntry{};

    stackSize = 0;
    currentNote = -1;
    previousNote = -1;
    currentVelocity = 0.0f;
    legato = false;
    noteTriggeredThisBlock = false;
    noteReleasedThisBlock = false;
    pitchBendNormalized = 0.0f;
    modWheel = 0.0f;
}

void NoteManager::prepareForBlock()
{
    noteTriggeredThisBlock = false;
    noteReleasedThisBlock = false;
}

void NoteManager::noteOn(int noteNumber, float velocity)
{
    if (noteNumber < 0 || noteNumber > 127)
        return;

    // Determine if this is a legato transition
    bool wasNoteHeld = (currentNote >= 0);

    previousNote = currentNote;

    // Push the new note onto the stack
    pushNote(noteNumber, velocity);

    // Set as current note
    currentNote = noteNumber;
    currentVelocity = velocity;
    legato = wasNoteHeld;
    noteTriggeredThisBlock = true;
}

void NoteManager::noteOff(int noteNumber)
{
    if (noteNumber < 0 || noteNumber > 127)
        return;

    // Remove from stack
    removeNote(noteNumber);

    // If this was the current note, fall back to the next note in the stack
    if (noteNumber == currentNote)
    {
        int nextNote = topNote();
        if (nextNote >= 0)
        {
            // Fall back to previously held note (legato return)
            previousNote = currentNote;
            currentNote = nextNote;
            currentVelocity = topVelocity();
            legato = true;
            noteTriggeredThisBlock = true;
        }
        else
        {
            // No more notes held — release
            previousNote = currentNote;
            currentNote = -1;
            currentVelocity = 0.0f;
            legato = false;
            noteReleasedThisBlock = true;
        }
    }
    // If it wasn't the current note, just removing from the stack is sufficient
}

void NoteManager::pitchBend(int value)
{
    // 14-bit pitch bend: 0 to 16383, center at 8192
    pitchBendNormalized = static_cast<float>(value - 8192) / 8192.0f;
}

void NoteManager::controlChange(int ccNumber, int ccValue)
{
    switch (ccNumber)
    {
        case 1:  // Mod wheel
            modWheel = static_cast<float>(ccValue) / 127.0f;
            break;

        case 123: // All notes off
        case 120: // All sound off
            reset();
            break;

        default:
            // Other CCs can be handled here in the future
            break;
    }
}

void NoteManager::setPitchBendRange(float semitones)
{
    pitchBendRange = std::max(0.0f, std::min(24.0f, semitones));
}

double NoteManager::noteToFrequency(int midiNote)
{
    // A4 (MIDI note 69) = 440 Hz
    // f = 440 * 2^((note - 69) / 12)
    return 440.0 * std::pow(2.0, (static_cast<double>(midiNote) - 69.0) / 12.0);
}

// --- Note stack management ---

void NoteManager::pushNote(int noteNumber, float velocity)
{
    // First check if this note is already in the stack (re-trigger)
    for (int i = 0; i < stackSize; ++i)
    {
        if (noteStack[i].note == noteNumber)
        {
            // Update velocity and move to top
            noteStack[i].velocity = velocity;
            // Shift it to the top by moving all above it down
            NoteEntry entry = noteStack[i];
            for (int j = i; j < stackSize - 1; ++j)
                noteStack[j] = noteStack[j + 1];
            noteStack[stackSize - 1] = entry;
            return;
        }
    }

    // Add new note to top of stack
    if (stackSize < MAX_NOTES)
    {
        noteStack[stackSize] = NoteEntry{ noteNumber, velocity, true };
        stackSize++;
    }
    else
    {
        // Stack full — drop the oldest note (bottom of stack)
        for (int i = 0; i < MAX_NOTES - 1; ++i)
            noteStack[i] = noteStack[i + 1];
        noteStack[MAX_NOTES - 1] = NoteEntry{ noteNumber, velocity, true };
    }
}

void NoteManager::removeNote(int noteNumber)
{
    for (int i = 0; i < stackSize; ++i)
    {
        if (noteStack[i].note == noteNumber)
        {
            // Shift everything above down
            for (int j = i; j < stackSize - 1; ++j)
                noteStack[j] = noteStack[j + 1];
            stackSize--;
            noteStack[stackSize] = NoteEntry{};
            return;
        }
    }
}

int NoteManager::topNote() const
{
    if (stackSize <= 0)
        return -1;
    return noteStack[stackSize - 1].note;
}

float NoteManager::topVelocity() const
{
    if (stackSize <= 0)
        return 0.0f;
    return noteStack[stackSize - 1].velocity;
}
