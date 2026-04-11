#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"
#include "AudioEngine.h"
#include "KeyboardComponent.h"
#include "VUMeterComponent.h"
#include "WaveformDisplay.h"
#include <array>
#include <optional>

class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

    // KeyListener
    bool keyPressed (const juce::KeyPress& key, juce::Component*) override;
    bool keyStateChanged (bool isDown, juce::Component*) override;

    // Timer (60fps UI update)
    void timerCallback() override;

private:
    // ── Audio ────────────────────────────────────────────
    AudioEngine engine;

    // ── State ────────────────────────────────────────────
    static constexpr int NUM_BANKS_TOTAL = 4;
    std::array<std::array<SoundSlot, NUM_KEYS>, NUM_BANKS_TOTAL> banks;
    int  currentBank    = 0;
    int  selectedKey    = -1;
    int  activeVoiceCount = 0;

    enum class PlayMode { FireStop, PlayEnd, Momentary, LoopFire, Restart };
    PlayMode playMode   = PlayMode::FireStop;
    bool     loopAll    = false;

    // ── Sub-components ───────────────────────────────────
    VUMeterComponent vuLeft  { "L" };
    VUMeterComponent vuRight { "R" };
    WaveformDisplay  waveform;
    KeyboardComponent keyboard;

    // Top rail
    juce::Label     titleLabel, subLabel, clockLabel, timerLabel;

    // LCD info bar
    juce::Label     lcdBankLabel, lcdClipLabel, lcdDurLabel, lcdPosLabel;

    // Transport buttons
    juce::TextButton btnRew, btnPlay, btnPause, btnStop, btnLoop, btnClear, btnKillAll;

    // Edit buttons
    juce::TextButton btnMark, btnZero, btnGoTo, btnFind;
    juce::TextButton btnCut, btnCopy, btnInsert, btnErase, btnUndo;
    juce::TextButton btnZoomIn, btnZoomOut;

    // Mode buttons
    juce::TextButton modeFireStop, modePlayEnd, modeMomentary, modeLoopFire, modeRestart;

    // Bank buttons
    juce::TextButton bankBtns[4];

    // Slot controls
    juce::Slider  gainSlider, trimInSlider, trimOutSlider;
    juce::Label   gainLabel, trimInLabel, trimOutLabel;

    // Master volume
    juce::Slider  masterVolSlider;
    juce::Label   masterVolLabel;

    // Status bar
    juce::Label   statusLabel, statusModeLabel;

    // ── Layout helpers ───────────────────────────────────
    juce::Rectangle<int> hardwareBounds, topRailBounds, mainBodyBounds, leftPanelBounds,
                          centerPanelBounds, rightPanelBounds, lcdBounds, lowerDeckBounds,
                          editClusterBounds, transportClusterBounds, keyboardHeaderBounds,
                          transportBounds, modeBounds, keyboardBounds, statusBounds,
                          scrubWheelBounds, voicesBounds, bankBounds, slotBounds, monitorBounds;

    // ── Logic ────────────────────────────────────────────
    void fireKey    (int keyIndex);
    void startKey   (int keyIndex, bool forceLoop);
    void stopKey    (int keyIndex);
    void stopAll    ();

    void selectKey  (int keyIndex);
    void switchBank (int bankIndex);
    void setMode    (PlayMode mode);

    void loadFileForKey (int keyIndex, const juce::File& file);
    void clearKey       (int keyIndex);

    void setStatus (const juce::String& msg);

    SoundSlot& currentSlotForKey (int ki) { return banks[currentBank][ki]; }
    bool       isPlaying         (int ki) const { return engine.isVoiceActive(currentBank, ki); }
    bool       hasSelection      () const { return selectedKey >= 0 && selectedKey < NUM_KEYS; }

    void initSlots();
    void initComponents();
    void bindCallbacks();
    void paintChassis   (juce::Graphics& g);
    void updateClockLabel();
    void updateTimerLabel();
    void updateBankButtons();
    void updateModeButtons();
    void updateSlotControls();
    void highlightTransport();
    juce::String gainToString (float g) const;
    juce::String formatTime   (double seconds) const;
    juce::String modeToString () const;
    int          countLoaded  (int bankIndex) const;

    void storeUndoFromKey (int keyIndex);
    void copySelectedSlot ();
    void cutSelectedSlot  ();
    void insertClipboardToSelected ();
    void eraseSelectedSlot ();
    void undoLastEdit ();
    void markSelectedKey ();
    void goToMarkedKey ();
    void findSlot ();
    void setPendingStatus (const juce::String& featureName);

    std::optional<SoundSlot> clipboardSlot;
    std::optional<SoundSlot> undoSlot;
    std::optional<int>       markedKey;
    int                      undoKey = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
