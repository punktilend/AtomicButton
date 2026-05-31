#include "VUMeterComponent.h"

VUMeterComponent::VUMeterComponent (juce::String label)
    : channelLabel (std::move (label))
{
}

void VUMeterComponent::setLevel (float level)
{
    currentLevel = juce::jlimit (0.0f, 1.0f, level * 2.5f);  // boost for visual response

    // Peak hold
    if (currentLevel > peakHold)
    {
        peakHold     = currentLevel;
        peakHoldTick = 0;
    }
    else
    {
        ++peakHoldTick;
        if (peakHoldTick > 40)  // ~40 frames hold
            peakHold = juce::jmax (0.0f, peakHold - 0.025f);
    }

    repaint();
}

void VUMeterComponent::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    g.fillAll (juce::Colour (0xff06170f));

    // Label
    g.setFont (juce::Font ("Courier New", 9.0f, juce::Font::plain));
    g.setColour (juce::Colour (0xff8fb3a6));
    g.drawText (channelLabel, 0, H - 12, W, 12, juce::Justification::centred);

    const int meterH   = H - 16;
    const int segCount = SEG_COUNT;
    const int segH     = (meterH - segCount + 1) / segCount;
    const int segW     = W - 4;
    const int segX     = 2;

    const int litSegs      = (int)(currentLevel * segCount);
    const int peakSegIndex = (int)(peakHold     * segCount);

    for (int i = 0; i < segCount; ++i)
    {
        const int y = meterH - (i + 1) * (segH + 1) + 2;
        const juce::Rectangle<int> segRect (segX, y, segW, segH);

        juce::Colour onColour;
        if      (i < 10) onColour = juce::Colour (0xff00ff44);
        else if (i < 13) onColour = juce::Colour (0xffffaa00);
        else             onColour = juce::Colour (0xffff2233);

        const bool lit  = (i < litSegs);
        const bool peak = (i == peakSegIndex && peakHold > 0.05f);

        if (lit || peak)
        {
            g.setColour (lit ? onColour : onColour.withAlpha (0.7f));
            g.fillRoundedRectangle (segRect.toFloat(), 1.0f);
            if (lit)
            {
                // LED glow
                g.setColour (onColour.withAlpha (0.15f));
                g.fillRoundedRectangle (segRect.expanded (1).toFloat(), 2.0f);
            }
        }
        else
        {
            g.setColour (juce::Colour (0xff0c1f17));
            g.fillRoundedRectangle (segRect.toFloat(), 1.0f);
        }
    }
}
