#include "EditClipComponent.h"

static const juce::Colour BG      (0xff101820);
static const juce::Colour PANEL   (0xff1a2a3a);
static const juce::Colour ACCENT  (0xff35c6ff);
static const juce::Colour BTNFACE (0xff2e4a62);
static const juce::Colour TXTCOL  (0xffe8eef4);
static const juce::Colour DIMCOL  (0xff6a8aaa);

EditClipComponent::EditClipComponent (SoundSlot& s, AudioEngine& eng,
                                       std::function<void()> cb)
    : slot(s), engine(eng), onApplied(std::move(cb))
{
    wTrimIn  = s.trimIn;
    wTrimOut = s.trimOut;
    wFadeIn  = s.fadeIn;
    wFadeOut = s.fadeOut;
    wGain    = s.gain;

    // Name editor
    addAndMakeVisible (nameEdit);
    nameEdit.setText (s.name);
    nameEdit.setFont (monoFont (13.0f));
    nameEdit.setColour (juce::TextEditor::backgroundColourId, PANEL);
    nameEdit.setColour (juce::TextEditor::textColourId, ACCENT);
    nameEdit.setColour (juce::TextEditor::outlineColourId, BTNFACE);
    nameEdit.addListener (this);

    // Sliders
    auto setupSlider = [&] (juce::Slider& sl, double lo, double hi,
                             double val, const juce::String& suffix)
    {
        addAndMakeVisible (sl);
        sl.setSliderStyle (juce::Slider::LinearHorizontal);
        sl.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
        sl.setRange (lo, hi, 0.001);
        sl.setValue (val, juce::dontSendNotification);
        sl.setTextValueSuffix (suffix);
        sl.setColour (juce::Slider::backgroundColourId,   PANEL);
        sl.setColour (juce::Slider::trackColourId,         ACCENT.withAlpha(0.4f));
        sl.setColour (juce::Slider::thumbColourId,         ACCENT);
        sl.setColour (juce::Slider::textBoxTextColourId,   TXTCOL);
        sl.setColour (juce::Slider::textBoxBackgroundColourId, PANEL);
        sl.setColour (juce::Slider::textBoxOutlineColourId,    BTNFACE);
        sl.addListener (this);
    };

    // Trim In/Out displayed as seconds
    const double dur = s.duration > 0.0 ? s.duration : 1.0;
    setupSlider (slTrimIn,  0.0, dur,  wTrimIn  * dur, "s");
    setupSlider (slTrimOut, 0.0, dur,  wTrimOut * dur, "s");
    setupSlider (slFadeIn,  0.0, 10.0, wFadeIn,        "s");
    setupSlider (slFadeOut, 0.0, 10.0, wFadeOut,       "s");
    setupSlider (slGain,   -12.0, 12.0,
                 20.0 * std::log10 (juce::jmax (0.001f, wGain)), "dB");

    // Labels
    for (auto* l : { &lblTrimIn, &lblTrimOut, &lblFadeIn, &lblFadeOut, &lblGain })
    {
        addAndMakeVisible (l);
        l->setFont (monoFont (9.5f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, DIMCOL);
    }

    // Buttons
    auto setupBtn = [&] (juce::TextButton& b, juce::Colour col)
    {
        addAndMakeVisible (b);
        b.setColour (juce::TextButton::buttonColourId,  col);
        b.setColour (juce::TextButton::textColourOffId, TXTCOL);
        b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    };
    setupBtn (btnPreview, BTNFACE);
    setupBtn (btnExtEdit, BTNFACE);
    setupBtn (btnApply,   juce::Colour (0xff1a5a1a));
    setupBtn (btnCancel,  juce::Colour (0xff5a1a1a));

    btnPreview.onClick = [this] {
        // Play with current edit settings applied temporarily
        SoundSlot tmp = slot;
        tmp.trimIn  = wTrimIn;
        tmp.trimOut = wTrimOut;
        tmp.fadeIn  = wFadeIn;
        tmp.fadeOut = wFadeOut;
        tmp.gain    = wGain;
        engine.fireVoice (tmp, false);
    };

    btnExtEdit.onClick = [this] {
        if (slot.sourceFile.existsAsFile())
        {
            // Open with system default audio editor
            if (! slot.sourceFile.startAsProcess())
            {
                // Fallback: reveal in Explorer
                slot.sourceFile.revealToUser();
            }
        }
    };

    btnApply.onClick = [this] {
        // Write edits back to slot
        slot.name    = nameEdit.getText().toUpperCase().substring (0, 16);
        slot.trimIn  = wTrimIn;
        slot.trimOut = wTrimOut;
        slot.fadeIn  = wFadeIn;
        slot.fadeOut = wFadeOut;
        slot.gain    = wGain;
        if (onApplied) onApplied();
        // Close the parent dialog window
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (1);
    };

    btnCancel.onClick = [this] {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->exitModalState (0);
    };

    setSize (660, 480);
}

EditClipComponent::~EditClipComponent()
{
    slTrimIn.removeListener (this);
    slTrimOut.removeListener (this);
    slFadeIn.removeListener (this);
    slFadeOut.removeListener (this);
    slGain.removeListener (this);
}

void EditClipComponent::sliderValueChanged (juce::Slider* sl)
{
    const double dur = slot.duration > 0.0 ? slot.duration : 1.0;
    if (sl == &slTrimIn)  { wTrimIn  = (float)(slTrimIn.getValue()  / dur); repaint (waveArea); }
    if (sl == &slTrimOut) { wTrimOut = (float)(slTrimOut.getValue() / dur); repaint (waveArea); }
    if (sl == &slFadeIn)  { wFadeIn  = (float)slFadeIn.getValue();  }
    if (sl == &slFadeOut) { wFadeOut = (float)slFadeOut.getValue();  }
    if (sl == &slGain)    { wGain    = (float)std::pow (10.0, slGain.getValue() / 20.0); }
}

void EditClipComponent::paint (juce::Graphics& g)
{
    g.fillAll (BG);

    // Title bar
    g.setColour (PANEL);
    g.fillRect (0, 0, getWidth(), 36);
    g.setColour (ACCENT);
    g.setFont (monoFont (13.0f, juce::Font::bold));
    g.drawText ("EDIT CLIP  \xe2\x80\x94  " + slot.name.toUpperCase(),
                10, 0, getWidth() - 20, 36, juce::Justification::centredLeft, true);

    // Slot info
    g.setColour (DIMCOL);
    g.setFont (monoFont (9.0f));
    g.drawText ("DUR: " + formatSecs (slot.duration)
                + "   SR: " + juce::String ((int)(slot.sampleRate / 1000)) + "kHz"
                + "   FILE: " + slot.sourceFile.getFileName(),
                10, 36, getWidth() - 20, 14, juce::Justification::centredLeft, true);

    drawWaveform (g);

    // Panel behind controls
    g.setColour (PANEL.withAlpha (0.5f));
    g.fillRect (0, waveArea.getBottom() + 4, getWidth(),
                nameEdit.getBottom() + 4 - waveArea.getBottom());
}

void EditClipComponent::drawWaveform (juce::Graphics& g)
{
    const auto& r = waveArea;
    g.setColour (juce::Colour (0xff0d1820));
    g.fillRect (r);
    g.setColour (ACCENT.withAlpha (0.15f));
    g.drawRect (r);

    if (slot.waveformPeaks.empty()) return;

    const int count = (int)slot.waveformPeaks.size();

    // Shaded out-of-trim regions
    const int inX  = fractionToX (wTrimIn);
    const int outX = fractionToX (wTrimOut);
    g.setColour (juce::Colour (0x80000000));
    g.fillRect (r.getX(), r.getY(), inX  - r.getX(), r.getHeight());
    g.fillRect (outX, r.getY(), r.getRight() - outX, r.getHeight());

    // Waveform
    g.setColour (ACCENT.withAlpha (0.6f));
    for (int x = 0; x < r.getWidth(); ++x)
    {
        const int idx  = juce::jlimit (0, count-1, (int)((float)x / r.getWidth() * count));
        const float pk = slot.waveformPeaks[idx];
        const int ph   = juce::jmax (1, (int)(pk * r.getHeight() * 0.85f));
        g.fillRect (r.getX() + x, r.getCentreY() - ph/2, 1, ph);
    }

    // Trim markers
    g.setColour (juce::Colour (0xff44ff88));
    g.fillRect (inX - 1, r.getY(), 2, r.getHeight());
    g.setColour (juce::Colour (0xffff4444));
    g.fillRect (outX - 1, r.getY(), 2, r.getHeight());

    // Fade in overlay
    if (wFadeIn > 0.0f && slot.duration > 0.0)
    {
        const float fadeInFrac = (float)(wFadeIn / slot.duration);
        const int   fadeInEndX = fractionToX (wTrimIn + fadeInFrac);
        juce::Path p;
        p.addTriangle ((float)inX, (float)r.getY(),
                       (float)fadeInEndX, (float)r.getY(),
                       (float)inX, (float)r.getBottom());
        g.setColour (juce::Colour (0x3044ff88));
        g.fillPath (p);
    }
    // Fade out overlay
    if (wFadeOut > 0.0f && slot.duration > 0.0)
    {
        const float fadeOutFrac = (float)(wFadeOut / slot.duration);
        const int   fadeOutStX  = fractionToX (wTrimOut - fadeOutFrac);
        juce::Path p;
        p.addTriangle ((float)fadeOutStX, (float)r.getY(),
                       (float)outX, (float)r.getY(),
                       (float)outX, (float)r.getBottom());
        g.setColour (juce::Colour (0x30ff4444));
        g.fillPath (p);
    }

    // Handle labels
    g.setFont (monoFont (9.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff44ff88));
    g.drawText ("IN",  inX + 3,  r.getY() + 2, 20, 12, juce::Justification::left,  false);
    g.setColour (juce::Colour (0xffff4444));
    g.drawText ("OUT", outX - 28, r.getY() + 2, 26, 12, juce::Justification::right, false);
}

void EditClipComponent::resized()
{
    const int pad = 10;
    int y = 54;

    // Waveform area
    waveArea = { pad, y, getWidth() - 2*pad, 130 };
    y += 130 + 8;

    // Name editor (full width row)
    nameEdit.setBounds (pad + 60, y, getWidth() - pad - 60 - pad, 22);
    // "NAME" label — reuse lblTrimIn temporarily before it gets its real placement below
    lblTrimIn.setBounds (pad, y, 55, 22);
    lblTrimIn.setText ("NAME", juce::dontSendNotification);
    y += 26;

    // Sliders (label 60px wide, slider fills rest)
    lblTrimIn.setText  ("TRIM IN",  juce::dontSendNotification);
    lblTrimIn.setBounds (pad, y, 75, 22);
    slTrimIn.setBounds  (pad+76, y, getWidth()-pad-76-pad, 22);
    y += 26;

    lblTrimOut.setBounds (pad, y, 75, 22);
    slTrimOut.setBounds  (pad+76, y, getWidth()-pad-76-pad, 22);
    y += 26;

    lblFadeIn.setBounds (pad, y, 75, 22);
    slFadeIn.setBounds  (pad+76, y, getWidth()-pad-76-pad, 22);
    y += 26;

    lblFadeOut.setBounds (pad, y, 75, 22);
    slFadeOut.setBounds  (pad+76, y, getWidth()-pad-76-pad, 22);
    y += 26;

    lblGain.setBounds (pad, y, 75, 22);
    slGain.setBounds  (pad+76, y, getWidth()-pad-76-pad, 22);
    y += 34;

    // Buttons row
    const int bH  = 34;
    const int bW  = 130;
    const int bG  = 8;
    const int bY  = getHeight() - bH - pad;
    btnPreview.setBounds (pad,                          bY, bW, bH);
    btnExtEdit.setBounds (pad + bW + bG,                bY, bW + 20, bH);
    btnApply.setBounds   (getWidth() - 2*(bW+bG) - pad, bY, bW, bH);
    btnCancel.setBounds  (getWidth() - bW - pad,        bY, bW, bH);
}

// ── Waveform drag interaction ─────────────────────────────
int EditClipComponent::fractionToX (float f) const
{
    return waveArea.getX() + (int)(f * waveArea.getWidth());
}

float EditClipComponent::xToFraction (int x) const
{
    return juce::jlimit (0.0f, 1.0f,
        (float)(x - waveArea.getX()) / (float)waveArea.getWidth());
}

void EditClipComponent::mouseDown (const juce::MouseEvent& e)
{
    if (!waveArea.contains (e.x, e.y)) return;
    const int inX  = fractionToX (wTrimIn);
    const int outX = fractionToX (wTrimOut);
    if (std::abs (e.x - inX)  < 8) { draggingHandle = 0; return; }
    if (std::abs (e.x - outX) < 8) { draggingHandle = 1; return; }
    draggingHandle = -1;
}

void EditClipComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingHandle < 0) return;
    const float f = xToFraction (e.x);
    const double dur = slot.duration > 0.0 ? slot.duration : 1.0;
    if (draggingHandle == 0)
    {
        wTrimIn = juce::jmin (f, wTrimOut - 0.02f);
        slTrimIn.setValue (wTrimIn * dur, juce::dontSendNotification);
    }
    else
    {
        wTrimOut = juce::jmax (f, wTrimIn + 0.02f);
        slTrimOut.setValue (wTrimOut * dur, juce::dontSendNotification);
    }
    repaint (waveArea);
}

void EditClipComponent::mouseUp (const juce::MouseEvent&)
{
    draggingHandle = -1;
}

juce::String EditClipComponent::formatSecs (double s) const
{
    const int m  = (int)s / 60;
    const double sec = std::fmod (s, 60.0);
    return juce::String (m) + ":" + juce::String (sec, 1).paddedLeft ('0', 4);
}
