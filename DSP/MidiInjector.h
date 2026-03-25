#pragma once
#include <cstdint>
#include <atomic>

/**
 * MidiInjector — Lock-free SPSC FIFO for injecting MIDI events
 * from the message thread into the audio thread's MidiBuffer.
 *
 * Single Producer (message thread) / Single Consumer (audio thread).
 * Fixed-size ring buffer — no heap allocation after construction.
 *
 * Each event is stored as: [noteNumber, velocity, deltaMs]
 * velocity=0 means note-off. deltaMs is relative to the first event
 * in the sequence (the audio thread converts to sample offset).
 *
 * Pure C++ — no JUCE dependencies.
 */
class MidiInjector
{
public:
    struct MidiEvent
    {
        uint8_t note = 0;        // MIDI note number (0-127)
        uint8_t velocity = 0;    // 0 = note-off, 1-127 = note-on
        uint16_t deltaMs = 0;    // Milliseconds from sequence start
    };

    MidiInjector() = default;

    /** Push an event from the message thread. Returns false if queue is full. */
    bool push(MidiEvent event)
    {
        auto currentWrite = writePos.load(std::memory_order_relaxed);
        auto nextWrite = (currentWrite + 1) % CAPACITY;

        if (nextWrite == readPos.load(std::memory_order_acquire))
            return false;  // Full

        buffer[currentWrite] = event;
        writePos.store(nextWrite, std::memory_order_release);
        return true;
    }

    /** Pop an event from the audio thread. Returns false if queue is empty. */
    bool pop(MidiEvent& event)
    {
        auto currentRead = readPos.load(std::memory_order_relaxed);

        if (currentRead == writePos.load(std::memory_order_acquire))
            return false;  // Empty

        event = buffer[currentRead];
        readPos.store((currentRead + 1) % CAPACITY, std::memory_order_release);
        return true;
    }

    /** Check if there are events available (audio thread). */
    bool hasEvents() const
    {
        return readPos.load(std::memory_order_acquire) !=
               writePos.load(std::memory_order_acquire);
    }

    /** Clear all pending events (call from message thread only when safe). */
    void clear()
    {
        readPos.store(0, std::memory_order_relaxed);
        writePos.store(0, std::memory_order_relaxed);
    }

    /** Signal that a new sequence has started (resets timing). */
    void beginSequence() { sequenceActive.store(true, std::memory_order_release); }

    /** Signal that the sequence is complete. */
    void endSequence() { sequenceActive.store(false, std::memory_order_release); }

    /** Check if a sequence is currently being played back. */
    bool isPlaying() const { return sequenceActive.load(std::memory_order_acquire); }

private:
    static constexpr int CAPACITY = 256;  // Max events in flight
    MidiEvent buffer[CAPACITY] = {};
    std::atomic<int> readPos { 0 };
    std::atomic<int> writePos { 0 };
    std::atomic<bool> sequenceActive { false };
};
