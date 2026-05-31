#pragma once
#include <JuceHeader.h>

// ============================================================
//  AtomicLookAndFeel — recessed "rubber pad" buttons for the
//  Atomic Button broadcast deck. Header-only.
//  Renders the face colour the button already carries
//  (buttonColourId / buttonOnColourId), so PLAY/REC/etc. keep
//  their distinct colours while sharing the hardware look.
// ============================================================
class AtomicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AtomicLookAndFeel()
    {
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffeaf6ef));
        setColour (juce::TextButton::textColourOnId,  juce::Colour (0xff04140c));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                               const juce::Colour& bg,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const float rad = 7.0f;
        const bool  on  = b.getToggleState();

        juce::Colour face = bg;
        if (highlighted && ! down) face = face.brighter (0.08f);
        if (down)                  face = face.darker  (0.12f);

        // recessed body — vertical gradient
        juce::ColourGradient cg (face.brighter (0.18f), 0.0f, r.getY(),
                                 face.darker   (0.30f), 0.0f, r.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (r, rad);

        // inner top highlight
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawLine (r.getX() + 4.0f, r.getY() + 1.5f, r.getRight() - 4.0f, r.getY() + 1.5f, 1.2f);

        // bottom inner shadow
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (r.getX() + 4.0f, r.getBottom() - 1.5f, r.getRight() - 4.0f, r.getBottom() - 1.5f, 1.5f);

        // outline (accent glow when toggled on)
        if (on)
        {
            g.setColour (juce::Colour (0xff62b6ff).withAlpha (0.55f));
            g.drawRoundedRectangle (r, rad, 2.0f);
        }
        else
        {
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawRoundedRectangle (r, rad, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b,
                         bool /*highlighted*/, bool /*down*/) override
    {
        const bool on = b.getToggleState();
        g.setColour (b.findColour (on ? juce::TextButton::textColourOnId
                                      : juce::TextButton::textColourOffId));
        g.setFont (juce::Font (juce::jmin (15.0f, b.getHeight() * 0.42f), juce::Font::bold));
        g.drawFittedText (b.getButtonText(),
                          b.getLocalBounds().reduced (4, 2),
                          juce::Justification::centred, 2, 0.9f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float pos, float startAngle, float endAngle,
                           juce::Slider&) override
    {
        const juce::Colour ink (0xff62b6ff);
        auto bounds = juce::Rectangle<float> ((float)x, (float)y, (float)width, (float)height).reduced (3.0f);
        const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
        const float rad = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const float ang = startAngle + pos * (endAngle - startAngle);

        // body — radial dark-green
        juce::ColourGradient body (juce::Colour (0xff2b5e49), cx - rad * 0.4f, cy - rad * 0.5f,
                                   juce::Colour (0xff06231a), cx, cy + rad, true);
        g.setGradientFill (body);
        g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f, 1.0f);

        // tick marks at min / centre / max
        for (float t : { 0.0f, 0.5f, 1.0f })
        {
            const float a = startAngle + t * (endAngle - startAngle);
            const float r0 = rad + 1.5f, r1 = rad + 4.0f;
            g.setColour (ink.withAlpha (0.55f));
            g.drawLine (cx + std::sin (a) * r0, cy - std::cos (a) * r0,
                        cx + std::sin (a) * r1, cy - std::cos (a) * r1, 1.2f);
        }

        // pointer
        const float pr = rad * 0.78f;
        g.setColour (ink.withAlpha (0.35f));
        g.drawLine (cx, cy, cx + std::sin (ang) * pr, cy - std::cos (ang) * pr, 4.0f); // glow
        g.setColour (ink);
        g.drawLine (cx, cy, cx + std::sin (ang) * pr, cy - std::cos (ang) * pr, 2.0f);
    }
};
