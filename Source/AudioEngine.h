#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"
#include <array>
#include <atomic>
#include <functional>

class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    AudioEngine();
    ~AudioEngine() override;

    // ── Lifecycle ─────────────────────────────────────────
    void initialise();
    void shutdown();

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // ── Loading ───────────────────────────────────────────
    // Decodes file and stores in slot. Runs on message thread.
    bool loadFile (SoundSlot& slot, const juce::File& file);

    // ── Playback control (call from any thread) ───────────
    void fireVoice  (const SoundSlot& slot, bool loop = false);
    void stopVoice  (int bankIndex, int keyIndex);
    void stopAll    ();

    // ── Query ─────────────────────────────────────────────
    bool isVoiceActive (int bankIndex, int keyIndex) const;
    // Returns elapsed seconds for the (first) voice on this slot
    double getVoicePosition (int bankIndex, int keyIndex) const;

    // ── VU metering (lock-free reads) ────────────────────
    float getVULevel (int channel) const;  // 0=L, 1=R, returns 0-1

    // ── Callbacks ────────────────────────────────────────
    std::function<void(int bankIndex, int keyIndex)> onVoiceEnded;

    // ── AudioIODeviceCallback ─────────────────────────────
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                          int                 numInputChannels,
                                          float* const*       outputChannelData,
                                          int                 numOutputChannels,
                                          int                 numSamples,
                                          const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped () override;

private:
    juce::AudioDeviceManager   deviceManager;
    juce::AudioFormatManager   formatManager;

    // Voice pool — accessed from audio thread, protected by voiceLock
    std::array<Voice, MAX_VOICES> voices;
    mutable juce::CriticalSection voiceLock;

    // VU — atomic, written on audio thread, read on message thread
    std::atomic<float> vuLevel[2];

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Voices to end — queued from audio thread, fired on message thread
    struct EndedVoice { int bank; int key; };
    juce::AbstractFifo           endedFifo  { 128 };
    std::array<EndedVoice, 128>  endedQueue {};
    // Inner timer to dispatch onVoiceEnded on message thread
    struct EndDispatcher : juce::Timer {
        AudioEngine& engine;
        EndDispatcher (AudioEngine& e) : engine(e) { startTimerHz(60); }
        void timerCallback() override { engine.dispatchEnded(); }
    };
    std::unique_ptr<EndDispatcher> endDispatcher;

    void dispatchEnded();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEngine)
};
