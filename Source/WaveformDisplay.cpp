#include "WaveformDisplay.h"

namespace WFPalette
{
    // Blue LCD theme (matches Atomic Button / IR3 design)
    static const juce::Colour LCD_BG    (0xff02101a);
    static const juce::Colour GRID      (0xff0c3346);
    static const juce::Colour WAVE_LIVE (0xff62b6ff);
    static const juce::Colour WAVE_TRIM (0xff1f4f7a);
    static const juce::Colour TRIM_LINE (0xff8fd0ff);
    static const juce::Colour EMPTY_TXT (0xff1f5f8a);
    static const juce::Colour PLAYHEAD  (0xffaee0ff);
}

WaveformDisplay::WaveformDisplay()
{
    setOpaque (true);
}

void WaveformDisplay::setSlot (const SoundSlot* slot)
{
    currentSlot = slot;
    repaint();
}

void WaveformDisplay::setPlayheadFraction (double frac)
{
    playheadFrac = juce::jlimit (0.0, 1.0, frac);
    repaint();
}

void WaveformDisplay::clearPlayhead()
{
    playheadFrac = 0.0;
    repaint();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    g.fillAll (WFPalette::LCD_BG);

    // Grid lines
    g.setColour (WFPalette::GRID);
    for (int x = 0; x < W; x += 50)
        g.drawVerticalLine (x, 0.0f, (float)H);
    g.drawHorizontalLine (H / 2, 0.0f, (float)W);

    if (currentSlot == nullptr || !currentSlot->isLoaded() || currentSlot->waveformPeaks.empty())
    {
        g.setColour (WFPalette::EMPTY_TXT);
        g.setFont (juce::Font ("Courier New", 11.0f, juce::Font::plain));
        g.drawText ("NO AUDIO — DROP FILE ONTO KEY OR RIGHT-CLICK TO ASSIGN",
                    0, 0, W, H, juce::Justification::centred);
        return;
    }

    const auto& peaks  = currentSlot->waveformPeaks;
    const int   inX    = (int)(currentSlot->trimIn  * W);
    const int   outX   = (int)(currentSlot->trimOut * W);
    const int   midY   = H / 2;

    // Dim regions outside trim
    g.setColour (juce::Colour (0x88000000));
    g.fillRect (0, 0, inX,     H);
    g.fillRect (outX, 0, W - outX, H);

    // Waveform bars
    const int count = (int)peaks.size();
    for (int x = 0; x < W; ++x)
    {
        const int   idx  = juce::jlimit (0, count - 1, (int)((float)x / W * count));
        const float peak = peaks[idx];
        const int   h    = juce::jmax (1, (int)(peak * H * 0.88f));
        g.setColour ((x >= inX && x <= outX) ? WFPalette::WAVE_LIVE : WFPalette::WAVE_TRIM);
        g.fillRect (x, midY - h / 2, 1, h);
    }

    // Trim handles
    g.setColour (WFPalette::TRIM_LINE);
    if (inX > 0)  { g.drawVerticalLine (inX,  0.0f, (float)H); }
    if (outX < W) { g.drawVerticalLine (outX, 0.0f, (float)H); }

    // Playhead
    if (playheadFrac > 0.0)
    {
        const int px = (int)(playheadFrac * W);
        g.setColour (WFPalette::WAVE_LIVE.withAlpha (0.25f));
        g.fillRect (px - 2, 0, 5, H);                       // glow
        g.setColour (WFPalette::PLAYHEAD.withAlpha (0.95f));
        g.drawVerticalLine (px, 0.0f, (float)H);
        // Small triangle
        juce::Path tri;
        tri.addTriangle ((float)px - 4.0f, 0.0f, (float)px + 4.0f, 0.0f, (float)px, 8.0f);
        g.fillPath (tri);
    }
}
