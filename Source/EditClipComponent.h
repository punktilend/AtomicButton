#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"
#include "AudioEngine.h"
#include <functional>

class EditClipComponent : public juce::Component,
                          private juce::Slider::Listener,
                          private juce::TextEditor::Listener
{
public:
    EditClipComponent (SoundSlot& slot, AudioEngine& engine,
                       std::function<void()> onApplied);
    ~EditClipComponent() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    void sliderValueChanged (juce::Slider*) override;
    void textEditorReturnKeyPressed (juce::TextEditor&) override {}

    void drawWaveform (juce::Graphics&);
    juce::String formatSecs (double s) const;
    float xToFraction (int x) const;
    int   fractionToX (float f) const;

    SoundSlot&   slot;
    AudioEngine& engine;
    std::function<void()> onApplied;

    // Working edit values (applied on OK)
    float wTrimIn  = 0.0f;
    float wTrimOut = 1.0f;
    float wFadeIn  = 0.0f;
    float wFadeOut = 0.0f;
    float wGain    = 1.0f;

    juce::Rectangle<int> waveArea;
    int draggingHandle = -1;   // 0=trimIn 1=trimOut

    juce::TextEditor nameEdit;

    // Sliders
    juce::Slider slTrimIn, slTrimOut, slFadeIn, slFadeOut, slGain;

    // Buttons
    juce::TextButton btnPreview {"PREVIEW"},
                     btnExtEdit {"OPEN EXTERNALLY"},
                     btnApply   {"APPLY"},
                     btnCancel  {"CANCEL"};

    // Labels
    juce::Label lblTrimIn  {"","TRIM IN"},
                lblTrimOut {"","TRIM OUT"},
                lblFadeIn  {"","FADE IN"},
                lblFadeOut {"","FADE OUT"},
                lblGain    {"","GAIN"};

    static juce::Font monoFont (float sz, int style = juce::Font::plain)
    { return juce::Font ("Courier New", sz, style); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditClipComponent)
};
