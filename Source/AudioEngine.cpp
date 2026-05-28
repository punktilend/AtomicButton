#include "AudioEngine.h"

AudioEngine::AudioEngine()
{
    vuLevel[0] = 0.0f;
    vuLevel[1] = 0.0f;

    formatManager.registerBasicFormats();   // WAV, AIFF, OGG, FLAC

    for (auto& v : voices)
        v.active = false;
}

AudioEngine::~AudioEngine()
{
    endDispatcher.reset();
    shutdown();
}

void AudioEngine::initialise()
{
    // Let JUCE pick the best default device (WASAPI on Windows, CoreAudio on Mac)
    auto err = deviceManager.initialiseWithDefaultDevices (0, 2);
    jassert (err.isEmpty());
    deviceManager.addAudioCallback (this);

    endDispatcher = std::make_unique<EndDispatcher> (*this);
}

void AudioEngine::shutdown()
{
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
}

// ── File loading (message thread) ─────────────────────────
bool AudioEngine::loadFile (SoundSlot& slot, const juce::File& file)
{
    auto* reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatReader> rdr (reader);

    const int64_t numSamples  = rdr->lengthInSamples;
    const int     numChannels = juce::jmin (static_cast<int> (rdr->numChannels), 2);

    if (numSamples <= 0 || numChannels <= 0)
        return false;

    // Guard against files that would allocate more than ~2 GB
    const int64_t maxSamples = (int64_t)2e9 / juce::jmax (1, numChannels) / 4;
    if (numSamples > maxSamples)
        return false;

    auto buffer = std::make_shared<juce::AudioBuffer<float>> (numChannels, (int)numSamples);
    rdr->read (buffer.get(), 0, (int)numSamples, 0, true, true);

    // If mono, duplicate to stereo
    if (buffer->getNumChannels() == 1)
    {
        buffer->setSize (2, (int)numSamples, true, false, true);
        buffer->copyFrom (1, 0, *buffer, 0, 0, (int)numSamples);
    }

    // Build waveform peaks (200 buckets for display)
    const int buckets = 200;
    std::vector<float> peaks (buckets);
    const int blockSize = juce::jmax (1, (int)(numSamples / buckets));
    const float* ch0 = buffer->getReadPointer (0);
    for (int i = 0; i < buckets; ++i)
    {
        float maxVal = 0.0f;
        const int start = i * blockSize;
        const int end   = juce::jmin (start + blockSize, (int)numSamples);
        for (int s = start; s < end; ++s)
            maxVal = juce::jmax (maxVal, std::abs (ch0[s]));
        peaks[i] = maxVal;
    }

    // Commit atomically
    {
        juce::ScopedLock sl (voiceLock);
        slot.buffer        = std::move (buffer);
        slot.sampleRate    = (int)rdr->sampleRate;
        slot.duration      = (double)numSamples / rdr->sampleRate;
        slot.sourceFile    = file;
        slot.waveformPeaks = std::move (peaks);
    }

    return true;
}

// ── Pause / seek ──────────────────────────────────────────
void AudioEngine::pauseVoice (int bank, int key)
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        if (v.active && v.bankIndex == bank && v.keyIndex == key)
            v.paused = true;
}

void AudioEngine::resumeVoice (int bank, int key)
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        if (v.active && v.bankIndex == bank && v.keyIndex == key)
            v.paused = false;
}

void AudioEngine::togglePauseVoice (int bank, int key)
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        if (v.active && v.bankIndex == bank && v.keyIndex == key)
            v.paused = !v.paused;
}

bool AudioEngine::isVoicePaused (int bank, int key) const
{
    juce::ScopedLock sl (voiceLock);
    for (const auto& v : voices)
        if (v.active && v.bankIndex == bank && v.keyIndex == key)
            return v.paused;
    return false;
}

void AudioEngine::seekVoice (int bank, int key, double deltaSeconds)
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        if (v.active && v.bankIndex == bank && v.keyIndex == key)
        {
            const int64_t delta = (int64_t)(deltaSeconds * currentSampleRate);
            v.readPos = juce::jlimit (v.startSamp, v.endSamp - 1LL, v.readPos + delta);
        }
}

// ── Voice management ──────────────────────────────────────
void AudioEngine::fireVoice (const SoundSlot& slot, bool loop)
{
    if (!slot.isLoaded()) return;

    juce::ScopedLock sl (voiceLock);

    // Stop any existing voice for this slot (one-voice-per-key policy)
    for (auto& v : voices)
        if (v.active && v.bankIndex == slot.bankIndex && v.keyIndex == slot.keyIndex)
            v.active = false;

    // Find a free voice
    Voice* target = nullptr;
    for (auto& v : voices)
        if (!v.active) { target = &v; break; }

    if (target == nullptr)
    {
        // Steal oldest voice (first active one in array)
        for (auto& v : voices)
            if (v.active) { target = &v; break; }
    }

    if (target == nullptr) return;

    target->buffer     = slot.buffer;
    target->startSamp  = slot.trimInSample();
    target->endSamp    = slot.trimOutSample();
    target->readPos    = target->startSamp;
    target->gain       = slot.gain;
    target->loop         = loop;
    target->fadeInSamps  = (int64_t)(slot.fadeIn  * currentSampleRate);
    target->fadeOutSamps = (int64_t)(slot.fadeOut * currentSampleRate);
    target->paused     = false;
    target->bankIndex  = slot.bankIndex;
    target->keyIndex   = slot.keyIndex;
    target->active     = true;
}

void AudioEngine::stopVoice (int bankIndex, int keyIndex)
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        if (v.active && v.bankIndex == bankIndex && v.keyIndex == keyIndex)
            v.active = false;
}

void AudioEngine::stopAll()
{
    juce::ScopedLock sl (voiceLock);
    for (auto& v : voices)
        v.active = false;
}

bool AudioEngine::isVoiceActive (int bankIndex, int keyIndex) const
{
    juce::ScopedLock sl (voiceLock);
    for (const auto& v : voices)
        if (v.active && v.bankIndex == bankIndex && v.keyIndex == keyIndex)
            return true;
    return false;
}

double AudioEngine::getVoicePosition (int bankIndex, int keyIndex) const
{
    juce::ScopedLock sl (voiceLock);
    for (const auto& v : voices)
    {
        if (v.active && v.bankIndex == bankIndex && v.keyIndex == keyIndex)
        {
            if (currentSampleRate > 0.0)
                return (double)(v.readPos - v.startSamp) / currentSampleRate;
        }
    }
    return 0.0;
}

float AudioEngine::getVULevel (int channel) const
{
    return vuLevel[juce::jlimit (0, 1, channel)].load (std::memory_order_relaxed);
}

// ── Audio callback (real-time thread) ─────────────────────
void AudioEngine::audioDeviceIOCallbackWithContext (
    const float* const*, int,
    float* const*       outputChannelData,
    int                 numOutputChannels,
    int                 numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    // Zero the output
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (outputChannelData[ch] != nullptr)
            juce::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

    const int outCh = juce::jmin (numOutputChannels, 2);

    float peakL = 0.0f, peakR = 0.0f;

    {
        juce::ScopedTryLock stl (voiceLock);
        if (!stl.isLocked())
            return;   // skip block rather than block real-time thread

        for (auto& v : voices)
        {
            if (!v.active || v.paused || v.buffer == nullptr) continue;

            const int bufChannels = v.buffer->getNumChannels();
            int samplesLeft = numSamples;
            int outOffset   = 0;

            while (samplesLeft > 0 && v.active)
            {
                const int64_t available = v.endSamp - v.readPos;
                if (available <= 0)
                {
                    if (v.loop)
                    {
                        v.readPos = v.startSamp;
                        continue;
                    }
                    else
                    {
                        v.active = false;
                        // Queue end notification (lock-free)
                        int idx1, size1, idx2, size2;
                        endedFifo.prepareToWrite (1, idx1, size1, idx2, size2);
                        if (size1 > 0 && idx1 >= 0 && idx1 < (int)endedQueue.size())
                        {
                            endedQueue[idx1] = { v.bankIndex, v.keyIndex };
                            endedFifo.finishedWrite (1);
                        }
                        break;
                    }
                }

                const int toCopy = (int)juce::jlimit ((int64_t)0, available, (int64_t)samplesLeft);

                for (int s = 0; s < toCopy; ++s)
                {
                    const int64_t elapsed   = (v.readPos + s) - v.startSamp;
                    const int64_t remaining = v.endSamp - (v.readPos + s);
                    float fadeGain = v.gain;
                    if (v.fadeInSamps  > 0 && elapsed   < v.fadeInSamps)
                        fadeGain *= (float)elapsed   / (float)v.fadeInSamps;
                    if (v.fadeOutSamps > 0 && remaining < v.fadeOutSamps)
                        fadeGain *= (float)remaining / (float)v.fadeOutSamps;

                    for (int ch = 0; ch < outCh; ++ch)
                    {
                        if (outputChannelData[ch] == nullptr) continue;
                        const int srcCh = juce::jmin (ch, bufChannels - 1);
                        outputChannelData[ch][outOffset + s] +=
                            v.buffer->getSample (srcCh, (int)(v.readPos + s)) * fadeGain;
                    }
                }

                v.readPos   += toCopy;
                outOffset   += toCopy;
                samplesLeft -= toCopy;
            }
        }

        // VU measurement
        if (numOutputChannels > 0 && outputChannelData[0])
            for (int i = 0; i < numSamples; ++i)
                peakL = juce::jmax (peakL, std::abs (outputChannelData[0][i]));
        if (numOutputChannels > 1 && outputChannelData[1])
            for (int i = 0; i < numSamples; ++i)
                peakR = juce::jmax (peakR, std::abs (outputChannelData[1][i]));
    }

    // Smooth VU decay (approx 20ms decay at 512 samples/44.1kHz)
    const float decay = 0.92f;
    vuLevel[0].store (juce::jmax (peakL, vuLevel[0].load() * decay), std::memory_order_relaxed);
    vuLevel[1].store (juce::jmax (peakR, vuLevel[1].load() * decay), std::memory_order_relaxed);
}

void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    currentSampleRate = device->getCurrentSampleRate();
}

void AudioEngine::audioDeviceStopped()
{
    stopAll();
}

void AudioEngine::dispatchEnded()
{
    int idx1, size1, idx2, size2;
    endedFifo.prepareToRead (endedFifo.getNumReady(), idx1, size1, idx2, size2);

    auto dispatch = [&] (int start, int count) {
        for (int i = start; i < start + count; ++i)
        {
            if (onVoiceEnded)
                onVoiceEnded (endedQueue[i].bank, endedQueue[i].key);
        }
    };
    dispatch (idx1, size1);
    dispatch (idx2, size2);

    endedFifo.finishedRead (size1 + size2);
}
