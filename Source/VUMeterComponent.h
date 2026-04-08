#pragma once
#include <JuceHeader.h>

// 16-segment LED VU meter (single channel, vertical)
class VUMeterComponent : public juce::Component
{
public:
    explicit VUMeterComponent (juce::String label);

    void setLevel (float level);    // 0.0 – 1.0 linear
    void paint (juce::Graphics& g) override;

private:
    static constexpr int SEG_COUNT = 16;
    juce::String channelLabel;
    float currentLevel = 0.0f;
    float peakHold     = 0.0f;
    int   peakHoldTick = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeterComponent)
};
