#pragma once
#include <JuceHeader.h>
#include "SoundSlot.h"
#include "AudioEngine.h"
#include "KeyboardComponent.h"
#include "VUMeterComponent.h"
#include "WaveformDisplay.h"
#include "AtomicLookAndFeel.h"
#include <array>
#include <optional>
#include <set>
#include <vector>
#include <algorithm>

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
    // ── Look and feel ────────────────────────────────────
    AtomicLookAndFeel atomicLF;

    // ── Audio ────────────────────────────────────────────
    AudioEngine engine;

    // ── State ────────────────────────────────────────────
    static constexpr int NUM_BANKS_TOTAL = 50;
    std::array<std::array<SoundSlot, NUM_KEYS>, NUM_BANKS_TOTAL> banks;
    int  currentBank      = 0;
    int  selectedKey      = -1;
    int  activeVoiceCount = 0;

    enum class PlayMode { FireStop, Loop };
    PlayMode playMode = PlayMode::FireStop;
    bool     loopAll  = false;

    // ── Recording (arm + PLAY to roll) ────────────────────
    int  recArmedKey  = -1;   // slot armed for recording
    int  recArmedBank = -1;

    // ── UI Mode ───────────────────────────────────────────────
    enum class UIMode { Normal, BankSelect, AssignHotKey, HotList };
    UIMode currentMode  = UIMode::Normal;
    int    assignTargetKey = -1;   // key being assigned in AssignHotKey mode

    // ── Hot List ──────────────────────────────────────────────
    std::vector<int> hotList;       // key indices queued for playback
    int  hotListPos    = -1;        // current play position in hot list (-1=idle)
    bool hotListActive = false;     // true while playing from hot list

    // ── Paused keys tracking ──────────────────────────────────
    std::set<int> pausedKeys;       // key indices currently paused

    // ── Sub-components ───────────────────────────────────
    VUMeterComponent vuLeft  { "L" };
    VUMeterComponent vuRight { "R" };
    WaveformDisplay  waveform;
    KeyboardComponent keyboard;

    // Top rail labels
    juce::Label titleLabel, subLabel, clockLabel, timerLabel;

    // LCD display labels
    juce::Label lcdBankLabel, lcdClipLabel, lcdDurLabel, lcdPosLabel;

    // IR3 right panel — context soft keys (below LCD) — 5 buttons like IR3
    juce::TextButton btnSoft[5];

    // IR3 right panel — navigation cluster
    juce::TextButton btnNavBack, btnNavDel, btnNavFwd;      // ◄◄ DEL ►►
    juce::TextButton btnNavLeft, btnNavUp, btnNavRight, btnNavDown;

    // IR3 right panel — 3×3 control grid
    juce::TextButton btnCancel, btnMenu, btnBankSelect;     // row 1
    juce::TextButton btnFind, btnAssignHotKey, btnHotList;  // row 2
    juce::TextButton btnLoop, btnPreview;                   // row 3

    // IR3 right panel — ENTER (wide) + power indicator
    juce::TextButton btnEnter;

    // IR3 left panel bottom strip
    juce::TextButton btnFollowOn, btnPause;

    // Transport row (5 buttons)
    juce::TextButton btnStop, btnPlay, btnRecord, btnRew, btnFF;

    // Rotary knobs (meter column)
    juce::Slider knobInputL, knobInputR, knobHeadphones;

    // Status bar
    juce::Label statusLabel, statusModeLabel;

    // IR3 state
    bool followOn = true;

    // ── Layout rects ─────────────────────────────────────
    juce::Rectangle<int> hardwareBounds, topRailBounds, mainBodyBounds,
                          leftPanelBounds, rightPanelBounds,
                          controlColumnBounds, meterColumnBounds,
                          lcdBounds, softKeyRowBounds, enterRowBounds,
                          navClusterBounds, rightColBounds,
                          transportRowBounds, leftBottomStripBounds,
                          statusBounds;

    // ── Logic ────────────────────────────────────────────
    void fireKey    (int keyIndex);
    void startKey   (int keyIndex, bool forceLoop);
    void stopKey    (int keyIndex);
    void stopAll    ();

    void selectKey  (int keyIndex);
    void switchBank (int bankIndex);

    void loadFileForKey (int keyIndex, const juce::File& file);
    void clearKey       (int keyIndex);
    void findSlot       ();
    void finishRecording();   // commit captured audio into the armed slot

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
    void highlightTransport();
    juce::String formatTime  (double seconds) const;
    int          countLoaded (int bankIndex) const;

    void enterNormalMode ();
    void enterBankSelectMode ();
    void enterAssignHotKeyMode ();
    void enterHotListMode ();
    void addToHotList (int keyIndex);
    void playNextInHotList ();
    void openFileChooserForKey (int keyIndex);
    void openEditClip  (int keyIndex);
    void saveProject   ();
    void loadProject   ();
    void updateLCD ();
    juce::String bankName (int b) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
