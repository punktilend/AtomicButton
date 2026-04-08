#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"

class WaveformDisplay : public juce::Component
{
public:
    WaveformDisplay();

    void setSlot (const SoundSlot* slot);
    void setPlayheadFraction (double frac);   // 0-1, called on timer
    void clearPlayhead();

    void paint (juce::Graphics& g) override;

private:
    const SoundSlot* currentSlot = nullptr;
    double playheadFrac = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
