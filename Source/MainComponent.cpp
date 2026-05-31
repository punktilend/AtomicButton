#include "MainComponent.h"
#include "EditClipComponent.h"

// ── Colour palette ────────────────────────────────────────
namespace C
{
    // ── Atomic Button — Teal anodized chassis + blue LCD (matches web IR3 design) ──
    static const juce::Colour ROOM        (0xff05100c);   // room behind the unit

    // Chassis gradient stops (top→bottom)
    static const juce::Colour CHASSIS_HI  (0xff159c8a);
    static const juce::Colour CHASSIS_C1  (0xff0d7568);
    static const juce::Colour CHASSIS_C2  (0xff0a594f);
    static const juce::Colour CHASSIS_C3  (0xff063b34);
    static const juce::Colour CHASSIS_EDGE(0xff03201c);
    static const juce::Colour CHASSIS     (0xff0d7568);   // base mid green
    static const juce::Colour CHASSIS_DRK (0xff063b34);

    // Darker recessed panel (repurposed names kept for paint() compatibility)
    static const juce::Colour PANEL_BLUE  (0xff0a594f);   // was steel blue — now dark green panel
    static const juce::Colour PANEL_BLUE_DRK (0xff063b34);

    // Brushed-metal top deck strip + thin bevel
    static const juce::Colour DECK_HI     (0xffeef1f2);
    static const juce::Colour DECK_LO     (0xffcfd5d8);
    static const juce::Colour RAIL        (0xffe8edee);   // brushed silver deck
    static const juce::Colour BEVEL       (0xff3fc9a6);
    static const juce::Colour SCREW       (0xff9aa2a6);

    // Swoosh accent + branding inks
    static const juce::Colour SWOOSH      (0xff2ec27e);
    static const juce::Colour SWOOSH_DK   (0xff1c7a4e);
    static const juce::Colour BRAND_BLUE  (0xff2b6db0);
    static const juce::Colour BRAND_DARK  (0xff0b2b22);

    // Status LEDs
    static const juce::Colour LED_GREEN   (0xff00ff44);
    static const juce::Colour LED_AMBER   (0xffffaa00);
    static const juce::Colour LED_RED     (0xffff2233);

    // Blue LCD
    static const juce::Colour LCD_BG      (0xff02101a);
    static const juce::Colour LCD_FRAME   (0xff0a3a4a);
    static const juce::Colour LCD_TEXT    (0xff62b6ff);
    static const juce::Colour LCD_DIM     (0xff1f6f9a);

    // Text on green chassis
    static const juce::Colour TEXT_MAIN   (0xffe8f1ec);
    static const juce::Colour TEXT_DIM    (0xffa9c6bd);

    // Pad buttons
    static const juce::Colour BTN_FACE    (0xff0e3326);
    static const juce::Colour BTN_TOP     (0xff134a35);
    static const juce::Colour BTN_LABEL   (0xffeaf6ef);

    static const juce::Colour METER_BG    (0xff06170f);
}

static juce::Font monoFont (float size, int style = juce::Font::plain)
{
    return juce::Font ("Courier New", size, style);
}

// ── Construction ──────────────────────────────────────────
MainComponent::MainComponent()
{
    setSize (1480, 1010);

    initSlots();
    engine.initialise();
    engine.onVoiceEnded = [this] (int bank, int key)
    {
        juce::MessageManager::callAsync ([this, bank, key]()
        {
            if (bank == currentBank)
            {
                keyboard.setKeyPlaying (key, false);
                pausedKeys.erase (key);
            }
            // Hot list follow-on
            if (hotListActive && followOn && hotListPos >= 0
                && hotListPos < (int)hotList.size()
                && hotList[hotListPos] == key)
            {
                if (hotListPos + 1 < (int)hotList.size())
                    playNextInHotList();
                else
                {
                    hotListActive = false;
                    hotListPos    = -1;
                    setStatus ("HOT LIST complete");
                }
            }
        });
    };

    initComponents();
    bindCallbacks();

    addKeyListener (this);
    startTimerHz (60);
}

MainComponent::~MainComponent()
{
    removeKeyListener (this);
    stopTimer();
    engine.shutdown();
}

// ── Slot initialisation ───────────────────────────────────
void MainComponent::initSlots()
{
    for (int b = 0; b < NUM_BANKS_TOTAL; ++b)
        for (int k = 0; k < NUM_KEYS; ++k)
        {
            auto& s     = banks[b][k];
            s.bankIndex = b;
            s.keyIndex  = k;
            s.key       = TRIGGER_KEYS[k];
            if (b < 4)
                s.name = PRESET_NAMES[b][k];
            else
                s.name = "SLOT " + juce::String (k + 1);
        }
}

// ── Component setup ───────────────────────────────────────
void MainComponent::initComponents()
{
    // ── Top rail ─────────────────────────────────────────
    // Brand wordmark ("360° BROADCAST" / "ATOMIC BUTTON") is painted directly
    // in paint() for the two-tone look; keep the labels present but blank.
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("", juce::dontSendNotification);

    addAndMakeVisible (subLabel);
    subLabel.setText ("", juce::dontSendNotification);

    addAndMakeVisible (clockLabel);
    clockLabel.setFont (monoFont (12.0f));
    clockLabel.setColour (juce::Label::textColourId, C::BRAND_DARK);
    clockLabel.setJustificationType (juce::Justification::right);

    addAndMakeVisible (timerLabel);
    timerLabel.setFont (monoFont (18.0f));
    timerLabel.setColour (juce::Label::textColourId, C::LCD_TEXT);
    timerLabel.setJustificationType (juce::Justification::right);
    timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93", juce::dontSendNotification);

    // ── LCD info labels ───────────────────────────────────
    for (auto* l : { &lcdBankLabel, &lcdClipLabel, &lcdDurLabel, &lcdPosLabel })
    {
        addAndMakeVisible (l);
        l->setFont (monoFont (11.0f));
        l->setColour (juce::Label::backgroundColourId, juce::Colour (0x00000000));
        l->setColour (juce::Label::textColourId, C::LCD_DIM);
    }
    lcdClipLabel.setColour (juce::Label::textColourId, C::LCD_TEXT);
    lcdClipLabel.setFont (monoFont (12.0f, juce::Font::bold));
    lcdBankLabel.setText ("BANK A  SLOT 1", juce::dontSendNotification);
    lcdClipLabel.setText ("-- NO CLIP --",   juce::dontSendNotification);
    lcdDurLabel.setText  ("0:00.0",           juce::dontSendNotification);
    lcdPosLabel.setText  ("> 0:00.0",         juce::dontSendNotification);

    // ── VU meters ────────────────────────────────────────
    addAndMakeVisible (vuLeft);
    addAndMakeVisible (vuRight);

    // ── Waveform ──────────────────────────────────────────
    addAndMakeVisible (waveform);

    // ── Keyboard ──────────────────────────────────────────
    addAndMakeVisible (keyboard);
    keyboard.setBank (currentBank, &banks[currentBank]);

    // ── IR3 right panel buttons ───────────────────────────
    auto setupBtn = [&] (juce::TextButton& btn, const juce::String& txt)
    {
        addAndMakeVisible (btn);
        btn.setButtonText (txt);
        btn.setLookAndFeel (&atomicLF);
        btn.setColour (juce::TextButton::buttonColourId,   C::BTN_FACE);
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff62b6ff)); // LCD ink accent
        btn.setColour (juce::TextButton::textColourOffId,  C::BTN_LABEL);
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff04140c));
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    };

    auto setupNavBtn = [&] (juce::TextButton& btn, const juce::String& txt)
    {
        setupBtn (btn, txt);
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff123f2e)); // nav green
    };

    // Soft keys below LCD (context-sensitive, labeled F1-F5)
    for (int i = 0; i < 5; ++i)
        setupBtn (btnSoft[i], "F" + juce::String (i + 1));

    // Navigation cluster
    setupNavBtn (btnNavBack,  "<<");
    setupNavBtn (btnNavDel,   "DEL");
    setupNavBtn (btnNavFwd,   ">>");
    setupNavBtn (btnNavLeft,  "<");
    setupNavBtn (btnNavUp,    "UP");
    setupNavBtn (btnNavRight, ">");
    setupNavBtn (btnNavDown,  "DN");

    // 3x3 right column — row 1
    setupBtn (btnCancel,     "CANCEL");
    setupBtn (btnMenu,       "MENU");
    setupBtn (btnBankSelect, "BANK\nSEL");

    // 3x3 right column — row 2
    setupBtn (btnFind,         "FIND");
    setupBtn (btnAssignHotKey, "ASSIGN HOT KEY");
    setupBtn (btnHotList,      "HOT\nLIST");

    // 3x3 right column — row 3
    setupBtn (btnLoop,    "LOOP");
    setupBtn (btnPreview, "PREVIEW");

    // CANCEL: normal styling — only lights red dynamically when there's something to cancel
    btnCancel.setToggleable (true);
    btnCancel.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6a1a1a));
    btnCancel.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xffff8888));
    btnLoop.setColour   (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a6a3a));

    // Toggle-able buttons
    btnBankSelect.setToggleable (true);
    btnAssignHotKey.setToggleable (true);
    btnHotList.setToggleable (true);
    btnLoop.setToggleable (true);

    // ENTER (wide, prominent)
    setupBtn (btnEnter, "ENTER");
    btnEnter.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff14503a));

    // Left panel bottom strip
    setupBtn (btnFollowOn, "FOLLOW ON");
    setupBtn (btnPause,    "PAUSE");
    btnFollowOn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1a6a3a));
    btnFollowOn.setToggleable (true);
    btnFollowOn.setToggleState (followOn, juce::dontSendNotification);

    // Transport (5 buttons)
    setupBtn (btnStop,   "STOP");
    setupBtn (btnPlay,   "PLAY");
    setupBtn (btnRecord, "REC");
    setupBtn (btnRew,    "REW");
    setupBtn (btnFF,     "FF");

    btnStop.setColour   (juce::TextButton::buttonColourId, juce::Colour (0xff2a2320));
    btnPlay.setColour   (juce::TextButton::buttonColourId, juce::Colour (0xff0f7a47));
    btnRecord.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff7a1410));
    btnRecord.setToggleable (true);
    btnRecord.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffd11a10));
    btnRecord.setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
    btnRew.setColour    (juce::TextButton::buttonColourId, juce::Colour (0xff123f2e));
    btnFF.setColour     (juce::TextButton::buttonColourId, juce::Colour (0xff123f2e));

    // ── Rotary knobs ──────────────────────────────────────
    auto setupKnob = [&] (juce::Slider& s)
    {
        addAndMakeVisible (s);
        s.setLookAndFeel (&atomicLF);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange (0.0, 1.0, 0.01);
        s.setValue (0.75, juce::dontSendNotification);
        s.onValueChange = [this] { repaint(); };   // refresh the -1.5/0/+1.5 readout
    };
    setupKnob (knobInputL);
    setupKnob (knobInputR);
    setupKnob (knobHeadphones);

    // ── Status bar ────────────────────────────────────────
    addAndMakeVisible (statusLabel);
    statusLabel.setFont (monoFont (10.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xff617081));
    statusLabel.setText ("READY -- Drop audio onto keys. Right-click for options. Ctrl+A/B/C/D switch banks.",
                          juce::dontSendNotification);

    addAndMakeVisible (statusModeLabel);
    statusModeLabel.setFont (monoFont (10.0f));
    statusModeLabel.setColour (juce::Label::textColourId, C::LCD_DIM);
    statusModeLabel.setText ("BANK A  |  0 PLAYING", juce::dontSendNotification);
    statusModeLabel.setJustificationType (juce::Justification::right);
}

// ── Callbacks ─────────────────────────────────────────────
void MainComponent::bindCallbacks()
{
    keyboard.onEditClip    = [this] (int ki) { openEditClip (ki); };
    keyboard.onKeyFired    = [this] (int ki) {
        switch (currentMode)
        {
            case UIMode::BankSelect:
                switchBank (ki);
                enterNormalMode();
                break;
            case UIMode::AssignHotKey:
                selectKey (ki);
                openFileChooserForKey (ki);
                enterNormalMode();
                break;
            case UIMode::HotList:
                addToHotList (ki);
                break;
            case UIMode::Normal:
            default:
                fireKey (ki);
                break;
        }
    };
    keyboard.onKeySelected = [this] (int ki) { selectKey (ki); };
    keyboard.onFileDrop    = [this] (int ki, const juce::File& f) { loadFileForKey (ki, f); };

    // ── Soft keys ─────────────────────────────────────────
    for (int i = 0; i < 5; ++i)
        btnSoft[i].onClick = [this, i] { softKeyPressed (i); };
    updateSoftKeys();

    // ── Navigation ─────────────────────────────────────────
    btnNavBack.onClick  = [this] {
        if (selectedKey >= 0) engine.seekVoice (currentBank, selectedKey, -5.0);
    };
    btnNavFwd.onClick   = [this] {
        if (selectedKey >= 0) engine.seekVoice (currentBank, selectedKey, +5.0);
    };
    btnNavDel.onClick   = [this] { clearKey (selectedKey); };
    btnNavLeft.onClick  = [this] {
        if (selectedKey > 0)  selectKey (selectedKey - 1);
    };
    btnNavRight.onClick = [this] {
        if (selectedKey < NUM_KEYS - 1) selectKey (selectedKey + 1);
    };
    btnNavUp.onClick    = [this] {
        if (selectedKey >= 10) selectKey (selectedKey - 10);
    };
    btnNavDown.onClick  = [this] {
        if (selectedKey + 10 < NUM_KEYS) selectKey (selectedKey + 10);
    };

    // ── Control grid ──────────────────────────────────────
    btnCancel.onClick = [this] {
        if (currentMode != UIMode::Normal)
            enterNormalMode();
        else if (selectedKey >= 0 && engine.isVoiceActive (currentBank, selectedKey))
            stopKey (selectedKey);
        else
            stopAll();
    };
    btnMenu.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem (1, "Audio Device Settings...");
        menu.addSeparator();
        menu.addItem (2, "Save Project...");
        menu.addItem (3, "Load Project...");
        menu.addSeparator();
        const bool hasClip = (selectedKey >= 0 && currentSlotForKey(selectedKey).isLoaded());
        menu.addItem (4, "Edit Clip...", hasClip);
        menu.addItem (5, "Open Clip in External Editor...", hasClip && currentSlotForKey(selectedKey).sourceFile.existsAsFile());
        menu.addSeparator();
        menu.addItem (6, "Clear All Banks");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btnMenu),
            [this] (int r)
            {
                if (r == 1)
                {
                    juce::AudioDeviceSelectorComponent selector (
                        engine.getDeviceManager(), 0, 0, 2, 2, false, false, true, false);
                    selector.setSize (400, 300);
                    juce::DialogWindow::LaunchOptions opts;
                    opts.content.setNonOwned (&selector);
                    opts.dialogTitle             = "Audio Device Settings";
                    opts.dialogBackgroundColour  = juce::Colour (0xff1a2a3a);
                    opts.useNativeTitleBar       = true;
                    opts.escapeKeyTriggersCloseButton = true;
                    opts.launchAsync();
                }
                else if (r == 2) saveProject();
                else if (r == 3) loadProject();
                else if (r == 4 && selectedKey >= 0) openEditClip (selectedKey);
                else if (r == 5 && selectedKey >= 0)
                {
                    auto& sl = currentSlotForKey (selectedKey);
                    if (sl.sourceFile.existsAsFile())
                        sl.sourceFile.startAsProcess();
                }
                else if (r == 6)
                {
                    for (auto& bank : banks)
                        for (auto& sl : bank)
                            sl.buffer.reset();
                    keyboard.refreshAll();
                    updateLCD();
                    setStatus ("All banks cleared");
                }
            });
    };
    btnBankSelect.onClick = [this] {
        if (currentMode == UIMode::BankSelect)
            enterNormalMode();
        else
            enterBankSelectMode();
    };
    btnFind.onClick = [this] { findSlot(); };
    btnAssignHotKey.onClick = [this] {
        if (currentMode == UIMode::AssignHotKey)
            enterNormalMode();
        else
            enterAssignHotKeyMode();
    };
    btnHotList.onClick = [this] {
        if (currentMode == UIMode::HotList)
            enterNormalMode();
        else
            enterHotListMode();
    };
    btnLoop.onClick = [this] {
        loopAll = !loopAll;
        btnLoop.setToggleState (loopAll, juce::dontSendNotification);
        highlightTransport();
        setStatus (juce::String ("LOOP: ") + (loopAll ? "ON" : "OFF"));
    };
    btnPreview.onClick = [this] {
        if (selectedKey >= 0) startKey (selectedKey, false);
    };

    // ── ENTER ─────────────────────────────────────────────
    btnEnter.onClick = [this] {
        switch (currentMode)
        {
            case UIMode::BankSelect:   enterNormalMode(); break;
            case UIMode::AssignHotKey: enterNormalMode(); break;
            case UIMode::HotList:
                // Start hot list playback from beginning
                hotListPos    = -1;
                hotListActive = true;
                playNextInHotList();
                break;
            case UIMode::Normal:
            default:
                if (selectedKey >= 0) fireKey (selectedKey);
                break;
        }
    };

    // ── Left bottom strip ─────────────────────────────────
    btnFollowOn.onClick = [this] {
        followOn = !followOn;
        btnFollowOn.setToggleState (followOn, juce::dontSendNotification);
        setStatus (juce::String ("FOLLOW ON: ") + (followOn ? "ON" : "OFF"));
    };
    btnPause.onClick = [this] {
        if (selectedKey < 0) return;
        if (engine.isVoiceActive (currentBank, selectedKey))
        {
            engine.togglePauseVoice (currentBank, selectedKey);
            const bool nowPaused = engine.isVoicePaused (currentBank, selectedKey);
            if (nowPaused)
            {
                pausedKeys.insert (selectedKey);
                btnPause.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff884422));
                setStatus ("PAUSED -- slot " + juce::String (selectedKey + 1));
            }
            else
            {
                pausedKeys.erase (selectedKey);
                btnPause.setColour (juce::TextButton::buttonColourId, C::BTN_FACE);
                setStatus ("RESUME -- slot " + juce::String (selectedKey + 1));
            }
        }
    };

    // ── Transport ─────────────────────────────────────────
    btnStop.onClick = [this] {
        if (engine.isRecording() || engine.isRecordArmed())
        {
            finishRecording();
            return;
        }
        if (hotListActive)
        {
            hotListActive = false;
            hotListPos    = -1;
            stopAll();
            setStatus ("HOT LIST stopped");
        }
        else
            stopAll();
    };
    btnPlay.onClick = [this] {
        if (engine.isRecordArmed())
        {
            engine.beginRecording();
            setStatus ("RECORDING -- slot " + juce::String (recArmedKey + 1)
                       + "  (STOP to finish)");
            return;
        }
        if (hotListActive)
        {
            // Manual step through hot list
            if (!followOn) playNextInHotList();
        }
        else if (selectedKey >= 0)
        {
            if (engine.isVoicePaused (currentBank, selectedKey))
            {
                engine.resumeVoice (currentBank, selectedKey);
                pausedKeys.erase (selectedKey);
                btnPause.setColour (juce::TextButton::buttonColourId, C::BTN_FACE);
                setStatus ("RESUME -- slot " + juce::String (selectedKey + 1));
            }
            else
                startKey (selectedKey, loopAll);
        }
    };
    btnRecord.onClick = [this] {
        if (engine.isRecording())
        {
            // Pressing REC while rolling stops and commits.
            finishRecording();
        }
        else if (engine.isRecordArmed())
        {
            // Disarm.
            engine.cancelRecord();
            recArmedKey = recArmedBank = -1;
            btnRecord.setToggleState (false, juce::dontSendNotification);
            setStatus ("REC -- disarmed");
        }
        else
        {
            if (selectedKey < 0)
            {
                setStatus ("REC -- select a key first");
                return;
            }
            recArmedKey  = selectedKey;
            recArmedBank = currentBank;
            engine.armRecord();
            btnRecord.setToggleState (true, juce::dontSendNotification);
            setStatus ("REC ARMED -- press PLAY to record to slot "
                       + juce::String (recArmedKey + 1) + ", STOP to finish");
        }
    };
    btnRew.onClick = [this] {
        if (selectedKey >= 0) engine.seekVoice (currentBank, selectedKey, -5.0);
        setStatus ("REW -5s");
    };
    btnFF.onClick = [this] {
        if (selectedKey >= 0) engine.seekVoice (currentBank, selectedKey, +5.0);
        setStatus ("FF +5s");
    };
}

// ── Mode / Hot List helpers ───────────────────────────────
juce::String MainComponent::bankName (int b) const
{
    if (b < 0 || b >= NUM_BANKS_TOTAL) return "??";
    // Banks 0-25 → A-Z, banks 26-49 → AA-AX
    if (b < 26) return juce::String::charToString ((juce::juce_wchar)('A' + b));
    return juce::String ("A") + juce::String::charToString ((juce::juce_wchar)('A' + (b - 26)));
}

void MainComponent::enterNormalMode()
{
    currentMode = UIMode::Normal;
    assignTargetKey = -1;
    btnBankSelect.setToggleState  (false, juce::dontSendNotification);
    btnAssignHotKey.setToggleState(false, juce::dontSendNotification);
    btnHotList.setToggleState     (false, juce::dontSendNotification);
    keyboard.setUIMode (0);
    setStatus ("READY");
    updateLCD();
    updateSoftKeys();
}

void MainComponent::enterBankSelectMode()
{
    currentMode = UIMode::BankSelect;
    btnBankSelect.setToggleState (true, juce::dontSendNotification);
    keyboard.setUIMode (1);
    setStatus ("BANK SEL -- press hot key 1-50 to select bank");
    updateLCD();
    updateSoftKeys();
}

void MainComponent::enterAssignHotKeyMode()
{
    currentMode = UIMode::AssignHotKey;
    btnAssignHotKey.setToggleState (true, juce::dontSendNotification);
    keyboard.setUIMode (2);
    setStatus ("ASSIGN HOT KEY -- press a hot key to assign a clip");
    updateLCD();
    updateSoftKeys();
}

void MainComponent::enterHotListMode()
{
    currentMode = UIMode::HotList;
    btnHotList.setToggleState (true, juce::dontSendNotification);
    keyboard.setUIMode (3);
    if (hotList.empty())
        setStatus ("HOT LIST -- press hot keys to add clips, PLAY to start");
    else
        setStatus ("HOT LIST -- " + juce::String ((int)hotList.size()) + " clips queued");
    updateLCD();
    updateSoftKeys();
}

void MainComponent::addToHotList (int keyIndex)
{
    auto it = std::find (hotList.begin(), hotList.end(), keyIndex);
    if (it != hotList.end())
    {
        hotList.erase (it);
        setStatus ("HOT LIST -- removed slot " + juce::String (keyIndex + 1));
    }
    else
    {
        if (currentSlotForKey (keyIndex).isLoaded())
        {
            hotList.push_back (keyIndex);
            setStatus ("HOT LIST -- added slot " + juce::String (keyIndex + 1)
                       + " (" + juce::String ((int)hotList.size()) + " total)");
        }
        else
            setStatus ("HOT LIST -- slot " + juce::String (keyIndex + 1) + " is empty");
    }
    keyboard.setHotList (hotList);
    updateLCD();
    updateSoftKeys();
}

void MainComponent::playNextInHotList()
{
    if (hotList.empty()) { hotListActive = false; return; }
    hotListPos = juce::jlimit (0, (int)hotList.size() - 1, hotListPos + 1);
    if (hotListPos >= (int)hotList.size()) { hotListActive = false; hotListPos = -1; return; }
    const int ki = hotList[hotListPos];
    startKey (ki, false);
    setStatus ("HOT LIST [" + juce::String (hotListPos + 1) + "/" + juce::String ((int)hotList.size()) + "]  "
               + currentSlotForKey (ki).name);
}

void MainComponent::openEditClip (int keyIndex)
{
    auto& sl = currentSlotForKey (keyIndex);
    if (!sl.isLoaded()) return;

    auto* editor = new EditClipComponent (sl, engine, [this, keyIndex]()
    {
        keyboard.refreshKey (keyIndex);
        updateLCD();
        setStatus ("EDIT APPLIED -- " + currentSlotForKey(keyIndex).name);
    });

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (editor);
    opts.dialogTitle             = "EDIT CLIP";
    opts.dialogBackgroundColour  = juce::Colour (0xff101820);
    opts.useNativeTitleBar       = false;
    opts.escapeKeyTriggersCloseButton = true;
    opts.launchAsync();
}

void MainComponent::saveProject()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Save Shortcut Pro Project",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.scp");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (!f.hasFileExtension ("scp"))
                f = f.withFileExtension ("scp");
            if (f.getFullPathName().isEmpty()) return;

            juce::XmlElement root ("ShortcutPro");
            root.setAttribute ("version", 1);

            for (int b = 0; b < NUM_BANKS_TOTAL; ++b)
            {
                bool bankHasContent = false;
                for (int k = 0; k < NUM_KEYS; ++k)
                    if (banks[b][k].isLoaded()) { bankHasContent = true; break; }
                if (!bankHasContent) continue;

                auto* bankEl = root.createNewChildElement ("Bank");
                bankEl->setAttribute ("index", b);
                bankEl->setAttribute ("name",  bankName (b));

                for (int k = 0; k < NUM_KEYS; ++k)
                {
                    const auto& sl = banks[b][k];
                    if (!sl.isLoaded()) continue;
                    auto* slotEl = bankEl->createNewChildElement ("Slot");
                    slotEl->setAttribute ("index",   k);
                    slotEl->setAttribute ("name",    sl.name);
                    slotEl->setAttribute ("file",    sl.sourceFile.getFullPathName());
                    slotEl->setAttribute ("trimIn",  (double)sl.trimIn);
                    slotEl->setAttribute ("trimOut", (double)sl.trimOut);
                    slotEl->setAttribute ("fadeIn",  (double)sl.fadeIn);
                    slotEl->setAttribute ("fadeOut", (double)sl.fadeOut);
                    slotEl->setAttribute ("gain",    (double)sl.gain);
                }
            }

            if (f.replaceWithText (root.toString()))
                setStatus ("PROJECT SAVED -- " + f.getFileName());
            else
                setStatus ("ERROR -- could not save project");
        });
}

void MainComponent::loadProject()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Load Shortcut Pro Project",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.scp");

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (!f.existsAsFile()) return;

            auto xml = juce::XmlDocument::parse (f);
            if (xml == nullptr || xml->getTagName() != "ShortcutPro")
            {
                setStatus ("ERROR -- not a valid Shortcut Pro project file");
                return;
            }

            engine.stopAll();
            // Clear banks
            for (auto& bank : banks)
                for (auto& sl : bank)
                    sl.buffer.reset();

            int loaded = 0;
            for (auto* bankEl : xml->getChildIterator())
            {
                if (bankEl->getTagName() != "Bank") continue;
                const int b = bankEl->getIntAttribute ("index", -1);
                if (b < 0 || b >= NUM_BANKS_TOTAL) continue;

                for (auto* slotEl : bankEl->getChildIterator())
                {
                    if (slotEl->getTagName() != "Slot") continue;
                    const int k = slotEl->getIntAttribute ("index", -1);
                    if (k < 0 || k >= NUM_KEYS) continue;

                    juce::File audioFile (slotEl->getStringAttribute ("file"));
                    if (!audioFile.existsAsFile()) continue;

                    auto& sl = banks[b][k];
                    if (engine.loadFile (sl, audioFile))
                    {
                        sl.name    = slotEl->getStringAttribute ("name", sl.name);
                        sl.trimIn  = (float)slotEl->getDoubleAttribute ("trimIn",  0.0);
                        sl.trimOut = (float)slotEl->getDoubleAttribute ("trimOut", 1.0);
                        sl.fadeIn  = (float)slotEl->getDoubleAttribute ("fadeIn",  0.0);
                        sl.fadeOut = (float)slotEl->getDoubleAttribute ("fadeOut", 0.0);
                        sl.gain    = (float)slotEl->getDoubleAttribute ("gain",    1.0);
                        ++loaded;
                    }
                }
            }

            switchBank (currentBank);
            updateLCD();
            setStatus ("PROJECT LOADED -- " + juce::String (loaded) + " clips restored from " + f.getFileName());
        });
}

void MainComponent::openFileChooserForKey (int keyIndex)
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Assign clip to hot key " + juce::String (keyIndex + 1),
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3");

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this, keyIndex, chooser] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile())
                loadFileForKey (keyIndex, f);
        });
}

void MainComponent::updateLCD()
{
    // Bank label
    lcdBankLabel.setText ("BANK " + bankName (currentBank)
                          + "  SLOT " + (selectedKey >= 0 ? juce::String (selectedKey + 1) : juce::String ("-")),
                          juce::dontSendNotification);

    // Mode indicator in bank label
    juce::String modeStr;
    switch (currentMode)
    {
        case UIMode::BankSelect:    modeStr = "  [BANK SEL]";   break;
        case UIMode::AssignHotKey:  modeStr = "  [ASSIGN]";     break;
        case UIMode::HotList:       modeStr = "  [HOT LIST]";   break;
        default: break;
    }
    if (modeStr.isNotEmpty())
        lcdBankLabel.setText (lcdBankLabel.getText() + modeStr, juce::dontSendNotification);

    // Clip info
    if (selectedKey >= 0 && selectedKey < NUM_KEYS)
    {
        const auto& slot = currentSlotForKey (selectedKey);
        lcdClipLabel.setText (slot.isLoaded() ? slot.name.toUpperCase() : "-- NO CLIP --",
                               juce::dontSendNotification);
        lcdDurLabel.setText  (slot.isLoaded() ? formatTime (slot.duration) : "0:00.0",
                               juce::dontSendNotification);
    }

    // Hot list info
    if (!hotList.empty() && currentMode == UIMode::HotList)
    {
        juce::String q = "QUEUE: ";
        for (int i = 0; i < juce::jmin ((int)hotList.size(), 8); ++i)
            q += juce::String (hotList[i] + 1) + " ";
        if ((int)hotList.size() > 8) q += "...";
        lcdClipLabel.setText (q, juce::dontSendNotification);
    }
}

// ── Layout ────────────────────────────────────────────────
void MainComponent::resized()
{
    // Deck fills the whole component (the window is sized to the deck's aspect),
    // with a small uniform "room" margin. Edge-to-edge avoids fragile centring.
    hardwareBounds = getLocalBounds().reduced (10);

    auto deck = hardwareBounds;
    topRailBounds  = deck.removeFromTop (42);
    statusBounds   = deck.removeFromBottom (26);
    mainBodyBounds = deck;

    // Split 62% left (hot keys) / 38% right (controls + meters)
    const int leftW  = (int)(mainBodyBounds.getWidth() * 0.62f);
    leftPanelBounds  = mainBodyBounds.removeFromLeft (leftW);
    rightPanelBounds = mainBodyBounds;

    // Right panel: far-right 80px = meter column; rest = control column
    const int meterColW = 80;
    meterColumnBounds   = rightPanelBounds.removeFromRight (meterColW);
    controlColumnBounds = rightPanelBounds;

    // ── Control column layout (IR3) — proportional fill ──
    auto ctrl = controlColumnBounds.reduced (8, 6);
    const int ctrlH = ctrl.getHeight();

    // Fixed-height sections
    const int softH      = 28;
    const int enterH     = 36;
    const int transportH = 48;
    const int gapsTotal  = 6 + 8 + 8 + 8 + 10;  // between each section

    // Distribute remaining height between LCD and mid-button block
    const int flexH  = ctrlH - softH - enterH - transportH - gapsTotal;
    const int lcdH   = (flexH * 44) / 100;   // LCD gets 44% of flex
    const int midH   = flexH - lcdH;          // mid buttons get the rest

    // 1) LCD touchscreen
    lcdBounds = ctrl.removeFromTop (lcdH);
    ctrl.removeFromTop (6);

    // 2) 4 soft keys below LCD
    softKeyRowBounds = ctrl.removeFromTop (softH);
    ctrl.removeFromTop (8);

    // 3) ENTER row
    enterRowBounds = ctrl.removeFromTop (enterH);
    ctrl.removeFromTop (8);

    // 4) Nav cluster (left ~42%) + right column, fills midH
    const int navColW = (ctrl.getWidth() * 42) / 100;
    auto midRow = ctrl.removeFromTop (midH);
    navClusterBounds = midRow.removeFromLeft (navColW);
    midRow.removeFromLeft (8);
    rightColBounds = midRow;
    ctrl.removeFromTop (10);

    // 5) Transport row
    transportRowBounds = ctrl.removeFromTop (transportH);

    // ── Left panel: keyboard + bottom strip ───────────────
    auto leftBody = leftPanelBounds.reduced (10, 8);
    leftBottomStripBounds = leftBody.removeFromBottom (40);
    keyboard.setBounds (leftBody);

    // Left bottom strip — FOLLOW ON + PAUSE
    {
        const int bH = leftBottomStripBounds.getHeight() - 4;
        const int bW = (leftBottomStripBounds.getWidth() - 8) / 2;
        const int y  = leftBottomStripBounds.getY() + 2;
        btnFollowOn.setBounds (leftBottomStripBounds.getX(),          y, bW, bH);
        btnPause.setBounds    (leftBottomStripBounds.getX() + bW + 8, y, bW, bH);
    }

    // ── LCD labels ────────────────────────────────────────
    {
        auto li = lcdBounds.reduced (6, 6);
        lcdBankLabel.setBounds (li.getX(), li.getY(),      li.getWidth(), 18);
        lcdClipLabel.setBounds (li.getX(), li.getY() + 20, li.getWidth(), 22);
        lcdDurLabel.setBounds  (li.getX(), li.getY() + 46, li.getWidth() / 2, 18);
        lcdPosLabel.setBounds  (li.getX() + li.getWidth() / 2,
                                 li.getY() + 46, li.getWidth() / 2, 18);
        waveform.setBounds (li.getX(), li.getY() + 68, li.getWidth(),
                            juce::jmax (20, lcdBounds.getHeight() - 68 - 8));
    }

    // ── Soft keys (5 buttons) ─────────────────────────────
    {
        const int sH = softKeyRowBounds.getHeight();
        const int sW = (softKeyRowBounds.getWidth() - 4 * 4) / 5;
        for (int i = 0; i < 5; ++i)
            btnSoft[i].setBounds (softKeyRowBounds.getX() + i * (sW + 4),
                                  softKeyRowBounds.getY(), sW, sH);
    }

    // ── ENTER row ─────────────────────────────────────────
    {
        const int eH = enterRowBounds.getHeight();
        const int eW = (enterRowBounds.getWidth() * 65) / 100;
        btnEnter.setBounds (enterRowBounds.getX(), enterRowBounds.getY(), eW, eH);
    }

    // ── Navigation cluster ────────────────────────────────
    {
        const int nG  = 5;
        const int nBH = (navClusterBounds.getHeight() - 2 * nG) / 3;
        const int nBW = (navClusterBounds.getWidth()  - 2 * nG) / 3;
        const int x0  = navClusterBounds.getX();
        const int y0  = navClusterBounds.getY();
        btnNavBack.setBounds  (x0,               y0,             nBW, nBH);
        btnNavDel.setBounds   (x0 + nBW + nG,    y0,             nBW, nBH);
        btnNavFwd.setBounds   (x0 + 2*(nBW+nG),  y0,             nBW, nBH);
        btnNavLeft.setBounds  (x0,               y0+nBH+nG,      nBW, nBH);
        btnNavUp.setBounds    (x0 + nBW + nG,    y0+nBH+nG,      nBW, nBH);
        btnNavRight.setBounds (x0 + 2*(nBW+nG),  y0+nBH+nG,      nBW, nBH);
        btnNavDown.setBounds  (x0 + nBW + nG,    y0+2*(nBH+nG),  nBW, nBH);
    }

    // ── 3x3 right column ──────────────────────────────────
    {
        const int rG  = 5;
        const int rBH = (rightColBounds.getHeight() - 2 * rG) / 3;
        const int rBW = (rightColBounds.getWidth()  - 2 * rG) / 3;
        const int x0  = rightColBounds.getX();
        const int y0  = rightColBounds.getY();
        btnCancel.setBounds     (x0,              y0,            rBW, rBH);
        btnMenu.setBounds       (x0 + rBW + rG,   y0,            rBW, rBH);
        btnBankSelect.setBounds (x0 + 2*(rBW+rG), y0,            rBW, rBH);
        btnFind.setBounds         (x0,              y0+rBH+rG,    rBW, rBH);
        btnAssignHotKey.setBounds (x0 + rBW + rG,   y0+rBH+rG,    rBW, rBH);
        btnHotList.setBounds      (x0 + 2*(rBW+rG), y0+rBH+rG,    rBW, rBH);
        btnLoop.setBounds    (x0,            y0+2*(rBH+rG),  rBW, rBH);
        btnPreview.setBounds (x0 + rBW + rG, y0+2*(rBH+rG),  rBW, rBH);
    }

    // ── Transport row (5 buttons) ─────────────────────────
    {
        const int tH = transportRowBounds.getHeight();
        const int tG = 5;
        const int tW = (transportRowBounds.getWidth() - 4 * tG) / 5;
        int x = transportRowBounds.getX();
        const int y = transportRowBounds.getY();
        btnStop.setBounds   (x, y, tW, tH); x += tW + tG;
        btnPlay.setBounds   (x, y, tW, tH); x += tW + tG;
        btnRecord.setBounds (x, y, tW, tH); x += tW + tG;
        btnRew.setBounds    (x, y, tW, tH); x += tW + tG;
        btnFF.setBounds     (x, y, tW, tH);
    }

    // ── Meter column ──────────────────────────────────────
    {
        auto meter = meterColumnBounds.reduced (6, 10);
        const int vuH    = 170;
        const int vuBarW = 14;
        const int vuGap  = 5;
        const int vuX    = meter.getCentreX() - vuBarW - vuGap / 2;
        vuLeft.setBounds  (vuX,              meter.getY() + 18, vuBarW, vuH);
        vuRight.setBounds (vuX+vuBarW+vuGap, meter.getY() + 18, vuBarW, vuH);
        const int knobSz     = 42;
        const int knobPitch  = 62;   // room for label above + readout below
        const int knobStartY = meter.getY() + 18 + vuH + 12;
        const int knobX      = meter.getCentreX() - knobSz / 2;
        knobInputL.setBounds    (knobX, knobStartY,                 knobSz, knobSz);
        knobInputR.setBounds    (knobX, knobStartY + knobPitch,     knobSz, knobSz);
        knobHeadphones.setBounds(knobX, knobStartY + knobPitch * 2, knobSz, knobSz);
    }

    // ── Top rail ─────────────────────────────────────────
    titleLabel.setBounds (topRailBounds.getX() + 20, topRailBounds.getY() + 6,  360, 16);
    subLabel.setBounds   (topRailBounds.getX() + 20, topRailBounds.getY() + 24, 470, 12);
    clockLabel.setBounds (topRailBounds.getRight() - 180, topRailBounds.getY() + 6,  164, 14);
    timerLabel.setBounds (topRailBounds.getRight() - 180, topRailBounds.getY() + 20, 164, 18);

    // ── Status bar ────────────────────────────────────────
    statusLabel.setBounds     (statusBounds.getX() + 10, statusBounds.getY() + 4,
                               statusBounds.getWidth() - 230, 16);
    statusModeLabel.setBounds (statusBounds.getRight() - 220, statusBounds.getY() + 4, 210, 16);
}

// ── Paint chassis ─────────────────────────────────────────
void MainComponent::paint (juce::Graphics& g)
{
    // Room background
    g.fillAll (C::ROOM);

    // Chassis body — Teal anodized vertical gradient
    {
        juce::ColourGradient cg (C::CHASSIS_HI, 0.0f, (float)hardwareBounds.getY(),
                                 C::CHASSIS_C3, 0.0f, (float)hardwareBounds.getBottom(), false);
        cg.addColour (0.08, C::CHASSIS_C1);
        cg.addColour (0.60, C::CHASSIS_C2);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (hardwareBounds.toFloat(), 10.0f);
    }

    // Right panel (steel blue) covers right portion
    {
        juce::Rectangle<int> rpFull (rightPanelBounds.getUnion (meterColumnBounds));
        // Clip to rounded hardware rect
        juce::Path rpPath;
        rpPath.addRoundedRectangle (
            (float)rpFull.getX(), (float)hardwareBounds.getY(),
            (float)rpFull.getWidth(), (float)hardwareBounds.getHeight(),
            10.0f, 10.0f, false, true, false, true);
        g.setColour (C::PANEL_BLUE);
        g.fillPath (rpPath);
    }

    // Chassis bevel
    g.setColour (C::BEVEL);
    g.drawRoundedRectangle (hardwareBounds.toFloat(), 10.0f, 1.0f);

    // Divider between left (gray) and right (blue) panels
    g.setColour (juce::Colour (0xff8a9aaa));
    g.fillRect (leftPanelBounds.getRight(), mainBodyBounds.getY(), 2, mainBodyBounds.getHeight());

    // Corner screws
    g.setColour (C::SCREW);
    for (const auto& p : { juce::Point<int> (hardwareBounds.getX() + 12, hardwareBounds.getY() + 12),
                            juce::Point<int> (hardwareBounds.getRight() - 12, hardwareBounds.getY() + 12),
                            juce::Point<int> (hardwareBounds.getX() + 12, hardwareBounds.getBottom() - 12),
                            juce::Point<int> (hardwareBounds.getRight() - 12, hardwareBounds.getBottom() - 12) })
        g.fillEllipse ((float)p.x - 3.0f, (float)p.y - 3.0f, 6.0f, 6.0f);

    // ── Top deck — brushed-metal strip with swoosh + brand ──
    {
        auto deck = topRailBounds.toFloat();
        juce::ColourGradient dg (C::DECK_HI, 0.0f, deck.getY(),
                                 C::DECK_LO, 0.0f, deck.getBottom(), false);
        g.setGradientFill (dg);
        g.fillRoundedRectangle (deck, 8.0f);

        // faint horizontal brushed lines
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        for (float yy = deck.getY() + 3.0f; yy < deck.getBottom() - 2.0f; yy += 3.0f)
            g.fillRect (deck.getX() + 4.0f, yy, deck.getWidth() - 8.0f, 0.6f);

        // swoosh — cubic ribbon across the lower third of the deck
        {
            const float L = deck.getX(), R = deck.getRight();
            const float W = deck.getWidth();
            const float base = deck.getBottom() - deck.getHeight() * 0.30f;
            const float amp  = deck.getHeight() * 0.16f;
            juce::Path sw;
            sw.startNewSubPath (L, base);
            sw.cubicTo (L + W * 0.22f, base - amp,  L + W * 0.38f, base + amp,   L + W * 0.56f, base - amp * 0.5f);
            sw.cubicTo (L + W * 0.72f, base - amp*1.4f, L + W * 0.86f, base + amp*0.4f, R, base - amp);
            sw.lineTo (R, deck.getBottom());
            sw.lineTo (L, deck.getBottom());
            sw.closeSubPath();
            g.setColour (C::SWOOSH.withAlpha (0.92f));
            g.fillPath (sw);
            g.setColour (C::BRAND_BLUE.withAlpha (0.8f));
            g.strokePath (sw, juce::PathStrokeType (2.0f));
        }

        // brand wordmark on the left of the deck
        const int bx = topRailBounds.getX() + 16;
        const int by = topRailBounds.getY() + 8;
        g.setColour (C::BRAND_BLUE);
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (juce::String::fromUTF8 ("360\xc2\xb0 BROADCAST"),
                    bx, by, 320, 14, juce::Justification::topLeft, false);

        juce::Font wordmark (30.0f, juce::Font::bold);
        wordmark.setItalic (true);
        g.setFont (wordmark);
        const int wy = by + 13;
        const juce::String a = "ATOMIC ";
        const int aw = wordmark.getStringWidth (a);
        g.setColour (C::BRAND_DARK);
        g.drawText (a, bx, wy, aw + 4, 34, juce::Justification::topLeft, false);
        g.setColour (C::SWOOSH);
        g.drawText ("BUTTON", bx + aw, wy, 260, 34, juce::Justification::topLeft, false);
    }

    // Status bar rail
    g.setColour (C::RAIL);
    g.fillRect  (statusBounds);
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (statusBounds.getX(), statusBounds.getY(), statusBounds.getWidth(), 1);

    // LCD surround (black with frame)
    {
        const auto lcdf = lcdBounds.expanded (5);
        g.setColour (C::LCD_BG);
        g.fillRoundedRectangle (lcdf.toFloat(), 4.0f);
        g.setColour (C::LCD_FRAME);
        g.drawRoundedRectangle (lcdf.toFloat(), 4.0f, 1.5f);
        // Cyan cursor line at top of LCD
        g.setColour (C::LCD_TEXT.withAlpha (0.5f));
        g.fillRect (lcdBounds.getX(), lcdBounds.getY() + 2, lcdBounds.getWidth(), 1);
    }

    // Section labels on right panel
    g.setColour (C::TEXT_DIM);
    g.setFont (monoFont (8.0f));
    g.drawText ("DISPLAY",
                lcdBounds.getX(), lcdBounds.getY() - 14,
                lcdBounds.getWidth(), 12, juce::Justification::centredLeft, false);
    g.drawText ("TRANSPORT",
                transportRowBounds.getX(), transportRowBounds.getY() - 10,
                transportRowBounds.getWidth(), 10, juce::Justification::centredLeft, false);

    // Power indicator (circle to right of ENTER)
    {
        const int eR = enterRowBounds.getRight();
        const int cy = enterRowBounds.getCentreY();
        const int r  = 8;
        g.setColour (juce::Colour (0xff1aaa44));
        g.fillEllipse ((float)(eR - r*3), (float)(cy - r), (float)(r*2), (float)(r*2));
        g.setColour (juce::Colour (0xff0a6622));
        g.drawEllipse ((float)(eR - r*3), (float)(cy - r), (float)(r*2), (float)(r*2), 1.0f);
    }

    // Meter column background (slightly darker blue)
    g.setColour (C::METER_BG);
    g.fillRect  (meterColumnBounds);

    // "PEAK LEVEL" label at top of meter column
    g.setColour (C::TEXT_DIM);
    g.setFont (monoFont (8.0f));
    g.drawText ("PEAK LEVEL",
                meterColumnBounds.getX(), meterColumnBounds.getY() + 6,
                meterColumnBounds.getWidth(), 12, juce::Justification::centred, false);

    // Knob labels + position scale (-1.5 / 0 / +1.5) + live readout
    {
        const int knobSz2    = 42;
        const int knobPitch2 = 62;
        const int knobX2     = knobInputL.getX();
        const int knobY0     = knobInputL.getY();
        const int colX       = meterColumnBounds.getX();
        const int colW       = meterColumnBounds.getWidth();

        struct KL { const char* name; juce::Slider* s; };
        const KL rows[3] = { { "INPUT L", &knobInputL },
                             { "INPUT R", &knobInputR },
                             { "PHONES",  &knobHeadphones } };

        for (int i = 0; i < 3; ++i)
        {
            const int ky = knobY0 + knobPitch2 * i;
            // name above the knob
            g.setColour (C::TEXT_DIM);
            g.setFont (monoFont (7.0f));
            g.drawText (rows[i].name, knobX2 - 6, ky - 10, knobSz2 + 12, 9,
                        juce::Justification::centred, false);
            // -1.5 / +1.5 ticks flanking the knob centre line
            g.setColour (C::LCD_DIM);
            g.setFont (monoFont (6.0f));
            g.drawText ("-1.5", colX + 1, ky + knobSz2 / 2 - 4, 16, 8, juce::Justification::left,  false);
            g.drawText ("+1.5", colX + colW - 17, ky + knobSz2 / 2 - 4, 16, 8, juce::Justification::right, false);
            // live readout below the knob: value 0..1 -> -1.5..+1.5
            const double pos = (rows[i].s->getValue() - 0.5) * 3.0;
            juce::String txt = (pos >= 0 ? "+" : "") + juce::String (pos, 1);
            g.setColour (C::LCD_TEXT);
            g.setFont (monoFont (9.0f, juce::Font::bold));
            g.drawText (txt, knobX2 - 6, ky + knobSz2 - 1, knobSz2 + 12, 11,
                        juce::Justification::centred, false);
        }
        // VU labels
        g.setColour (C::TEXT_DIM);
        g.setFont (monoFont (7.0f));
        g.drawText ("L", vuLeft.getX(),  meterColumnBounds.getY() + 8, vuLeft.getWidth(),  12, juce::Justification::centred, false);
        g.drawText ("R", vuRight.getX(), meterColumnBounds.getY() + 8, vuRight.getWidth(), 12, juce::Justification::centred, false);
    }

    // Active voice count in status
    g.setColour (C::LCD_TEXT);
    g.setFont (monoFont (11.0f, juce::Font::bold));
    g.drawText (juce::String (activeVoiceCount) + " PLAYING",
                statusBounds.getRight() - 120, statusBounds.getY() + 4,
                110, 16, juce::Justification::right, false);
}

// ── Timer (60fps) ─────────────────────────────────────────
void MainComponent::timerCallback()
{
    updateClockLabel();
    updateTimerLabel();

    vuLeft.setLevel  (engine.getVULevel (0));
    vuRight.setLevel (engine.getVULevel (1));

    // Update playhead for selected key
    if (selectedKey >= 0 && isPlaying (selectedKey))
    {
        const auto& slot = currentSlotForKey (selectedKey);
        if (slot.isLoaded())
        {
            const double pos    = engine.getVoicePosition (currentBank, selectedKey);
            const double segDur = (slot.trimOut - slot.trimIn) * slot.duration;
            const double frac   = segDur > 0.0 ? pos / segDur : 0.0;
            waveform.setPlayheadFraction (frac);
        }
    }

    // Count active voices
    int count = 0;
    for (int ki = 0; ki < NUM_KEYS; ++ki)
        if (engine.isVoiceActive (currentBank, ki)) ++count;
    activeVoiceCount = count;

    // Update LCD position every frame
    if (selectedKey >= 0 && engine.isVoiceActive (currentBank, selectedKey))
    {
        const double pos = engine.getVoicePosition (currentBank, selectedKey);
        lcdPosLabel.setText ("> " + formatTime (pos), juce::dontSendNotification);
    }
    // CANCEL lights up only when there's an active mode or something playing
    {
        const bool hasMode    = (currentMode != UIMode::Normal);
        const bool hasPlaying = (activeVoiceCount > 0);
        btnCancel.setToggleState (hasMode || hasPlaying, juce::dontSendNotification);
    }

    // Update bank/mode label
    statusModeLabel.setText ("BANK " + bankName (currentBank)
        + "  |  " + juce::String (activeVoiceCount) + " PLAYING",
        juce::dontSendNotification);

    repaint (meterColumnBounds);
    repaint (statusBounds);
}

void MainComponent::updateClockLabel()
{
    const auto t = juce::Time::getCurrentTime();
    clockLabel.setText (t.toString (false, true, true, false), juce::dontSendNotification);
}

void MainComponent::updateTimerLabel()
{
    if (selectedKey < 0 || !isPlaying (selectedKey)) return;
    const double pos  = engine.getVoicePosition (currentBank, selectedKey);
    const int    min  = (int)(pos / 60.0);
    const double sec  = pos - min * 60.0;
    timerLabel.setText (juce::String::formatted ("%d:%04.1f", min, sec),
                        juce::dontSendNotification);
}

// ── Playback logic ────────────────────────────────────────
void MainComponent::fireKey (int ki)
{
    auto& slot = currentSlotForKey (ki);
    if (!slot.isLoaded()) return;

    if (isPlaying (ki))
        stopKey (ki);
    else
        startKey (ki, loopAll);
}

void MainComponent::startKey (int ki, bool forceLoop)
{
    auto& slot = currentSlotForKey (ki);
    if (!slot.isLoaded()) return;
    engine.fireVoice (slot, forceLoop || loopAll);
    keyboard.setKeyPlaying (ki, true);
    selectKey (ki);
    setStatus ("> " + slot.name);
}

void MainComponent::stopKey (int ki)
{
    engine.stopVoice (currentBank, ki);
    keyboard.setKeyPlaying (ki, false);
    if (ki == selectedKey)
    {
        waveform.clearPlayhead();
        timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93",
                            juce::dontSendNotification);
    }
}

void MainComponent::stopAll()
{
    engine.stopAll();
    keyboard.refreshAll();
    waveform.clearPlayhead();
    timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93",
                        juce::dontSendNotification);
    setStatus ("STOP -- all voices killed");
}

void MainComponent::selectKey (int ki)
{
    selectedKey = ki;
    keyboard.setActiveKey (ki);
    const auto& slot = currentSlotForKey (ki);
    lcdClipLabel.setText (slot.isLoaded() ? slot.name : "-- NO CLIP --",
                           juce::dontSendNotification);
    lcdDurLabel.setText  (slot.isLoaded() ? formatTime (slot.duration) : "0:00.0",
                           juce::dontSendNotification);
    lcdPosLabel.setText  ("> 0:00.0", juce::dontSendNotification);
    waveform.setSlot (slot.isLoaded() ? &slot : nullptr);
    updateLCD();
    updateSoftKeys();   // EDIT/RENAME availability follows the selected slot
}

void MainComponent::switchBank (int b)
{
    currentBank = b;
    keyboard.setBank (b, &banks[b]);
    keyboard.setUIMode ((int)currentMode);   // preserve mode after bank change
    if (selectedKey >= 0) selectKey (selectedKey);
    setStatus ("Bank " + bankName (b)
               + " -- " + juce::String (countLoaded (b)) + " clips loaded");
    updateLCD();
}

int MainComponent::countLoaded (int b) const
{
    int n = 0;
    for (const auto& s : banks[b]) if (s.isLoaded()) ++n;
    return n;
}

void MainComponent::loadFileForKey (int ki, const juce::File& file)
{
    // Stop any voice already playing on this slot
    engine.stopVoice (currentBank, ki);

    auto& slot = currentSlotForKey (ki);

    // Reset all per-slot edit params so old trim/fade/gain don't bleed onto the new file
    slot.trimIn  = 0.0f;
    slot.trimOut = 1.0f;
    slot.fadeIn  = 0.0f;
    slot.fadeOut = 0.0f;
    slot.gain    = 1.0f;

    setStatus ("Loading: " + file.getFileName() + "...");
    bool ok = engine.loadFile (slot, file);
    if (ok)
    {
        slot.name = file.getFileNameWithoutExtension().toUpperCase().substring (0, 14);
        keyboard.refreshKey (ki);
        selectKey (ki);
        setStatus ("Loaded: " + slot.name + "  (" + formatTime (slot.duration) + ")");
        updateLCD();
    }
    else
    {
        const auto ext = file.getFileExtension().toUpperCase();
        juce::String reason;
        if (ext == ".MP3")
            reason = "MP3 support is disabled. Convert to WAV or FLAC and try again.";
        else if (!file.existsAsFile())
            reason = "File not found.";
        else
            reason = "The file could not be decoded.\n"
                     "Supported formats: WAV, AIFF, FLAC, OGG\n\n"
                     "If this is a FLAC, try re-encoding it at 16-bit or 24-bit PCM.";

        setStatus ("ERROR: " + file.getFileName() + " -- " + reason.upToFirstOccurrenceOf ("\n", false, false));

        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::WarningIcon,
            "Cannot Load File",
            file.getFileName() + "\n\n" + reason);
    }
}

void MainComponent::clearKey (int ki)
{
    engine.stopVoice (currentBank, ki);
    auto& slot = banks[currentBank][ki];
    slot = SoundSlot();
    slot.bankIndex = currentBank;
    slot.keyIndex  = ki;
    slot.key       = TRIGGER_KEYS[ki];
    slot.name      = (currentBank < 4) ? PRESET_NAMES[currentBank][ki] : ("SLOT " + juce::String (ki + 1));
    keyboard.refreshKey (ki);
    if (ki == selectedKey) selectKey (ki);
    const char k = TRIGGER_KEYS[ki];
    const juce::String kStr = (k != '\0')
        ? juce::String::charToString ((juce::juce_wchar)toupper(k))
        : juce::String (ki + 1);
    setStatus ("Slot " + juce::String (ki + 1) + " [" + kStr + "] cleared");
}

void MainComponent::finishRecording()
{
    if (recArmedBank < 0 || recArmedKey < 0)
    {
        engine.cancelRecord();
        btnRecord.setToggleState (false, juce::dontSendNotification);
        return;
    }

    const int bank = recArmedBank;
    const int key  = recArmedKey;
    const bool committed = engine.commitRecordingToSlot (banks[bank][key]);

    recArmedKey = recArmedBank = -1;
    btnRecord.setToggleState (false, juce::dontSendNotification);

    if (committed)
    {
        auto& slot = banks[bank][key];
        if (bank == currentBank)
        {
            keyboard.refreshKey (key);
            selectKey (key);
        }
        updateLCD();
        setStatus ("RECORDED -- " + juce::String (slot.duration, 1) + "s to bank "
                   + bankName (bank) + " slot " + juce::String (key + 1));
    }
    else
    {
        setStatus ("REC -- nothing captured (check input device in MENU)");
    }
}

void MainComponent::renameSlot (int ki)
{
    if (ki < 0) return;

    auto* w = new juce::AlertWindow ("Rename slot " + juce::String (ki + 1),
                                     "Enter new name:", juce::AlertWindow::NoIcon);
    w->addTextEditor ("name", currentSlotForKey (ki).name);
    w->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<juce::AlertWindow> safe (w);
    w->enterModalState (true,
        juce::ModalCallbackFunction::create ([this, ki, safe] (int r)
        {
            if (r == 1 && safe != nullptr)
            {
                const auto val = safe->getTextEditorContents ("name");
                if (val.isNotEmpty())
                    currentSlotForKey (ki).name = val.toUpperCase().substring (0, 14);
            }
            keyboard.refreshKey (ki);
            if (ki == selectedKey) selectKey (ki);
        }), true);
}

// ── Context soft keys (F1-F5) ─────────────────────────────
void MainComponent::updateSoftKeys()
{
    auto set = [this] (int i, const juce::String& label, bool enabled)
    {
        btnSoft[i].setButtonText (label);
        btnSoft[i].setEnabled (enabled);
    };

    switch (currentMode)
    {
        case UIMode::BankSelect:
            set (0, "PREV", currentBank > 0);
            set (1, "NEXT", currentBank < NUM_BANKS_TOTAL - 1);
            set (2, "",     false);
            set (3, "",     false);
            set (4, "EXIT", true);
            break;

        case UIMode::AssignHotKey:
            set (0, "",     false);
            set (1, "",     false);
            set (2, "",     false);
            set (3, "",     false);
            set (4, "EXIT", true);
            break;

        case UIMode::HotList:
            set (0, "CLEAR", ! hotList.empty());
            set (1, "",      false);
            set (2, "",      false);
            set (3, "",      false);
            set (4, "EXIT",  true);
            break;

        case UIMode::Normal:
        default:
        {
            const bool hasClip = (selectedKey >= 0 && currentSlotForKey (selectedKey).isLoaded());
            set (0, "EDIT",   hasClip);
            set (1, "RENAME", selectedKey >= 0);
            set (2, "LOAD",   selectedKey >= 0);
            set (3, "SAVE",   true);
            set (4, "OPEN",   true);
            break;
        }
    }
}

void MainComponent::softKeyPressed (int index)
{
    switch (currentMode)
    {
        case UIMode::BankSelect:
            if      (index == 0 && currentBank > 0)                    switchBank (currentBank - 1);
            else if (index == 1 && currentBank < NUM_BANKS_TOTAL - 1)  switchBank (currentBank + 1);
            else if (index == 4)                                       enterNormalMode();
            return;

        case UIMode::AssignHotKey:
            if (index == 4) enterNormalMode();
            return;

        case UIMode::HotList:
            if (index == 0)
            {
                hotList.clear();
                hotListActive = false;
                hotListPos    = -1;
                keyboard.setHotList (hotList);
                setStatus ("HOT LIST -- cleared");
                updateLCD();
                updateSoftKeys();
            }
            else if (index == 4)
            {
                enterNormalMode();
            }
            return;

        case UIMode::Normal:
        default:
            switch (index)
            {
                case 0:  if (selectedKey >= 0 && currentSlotForKey (selectedKey).isLoaded())
                             openEditClip (selectedKey);
                         break;
                case 1:  if (selectedKey >= 0) renameSlot (selectedKey);          break;
                case 2:  if (selectedKey >= 0) openFileChooserForKey (selectedKey); break;
                case 3:  saveProject();                                            break;
                case 4:  loadProject();                                            break;
                default: break;
            }
            return;
    }
}

void MainComponent::findSlot()
{
    auto* findWindow = new juce::AlertWindow (
        "Find Cut",
        "Type part of a clip name in the current bank:",
        juce::AlertWindow::NoIcon);

    findWindow->addTextEditor ("query", "");
    findWindow->addButton ("Find",   1, juce::KeyPress (juce::KeyPress::returnKey));
    findWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<juce::AlertWindow> safeWindow (findWindow);
    findWindow->enterModalState (
        true,
        juce::ModalCallbackFunction::create ([this, safeWindow] (int result)
        {
            if (result != 1 || safeWindow == nullptr)
                return;

            const auto query = safeWindow->getTextEditorContents ("query").trim().toUpperCase();
            if (query.isEmpty())
            {
                setStatus ("FIND -- no search text entered");
                return;
            }

            for (int i = 0; i < NUM_KEYS; ++i)
            {
                if (banks[currentBank][i].name.toUpperCase().contains (query))
                {
                    selectKey (i);
                    setStatus ("FIND -- slot " + juce::String (i + 1) + ": " + banks[currentBank][i].name);
                    return;
                }
            }

            setStatus ("FIND -- no cut matched \"" + query + "\" in bank "
                       + bankName (currentBank));
        }),
        true);
}

// ── Keyboard input ────────────────────────────────────────
bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const auto mods = key.getModifiers();

    // Ctrl combos
    if (mods.isCommandDown() || mods.isCtrlDown())
    {
        const int kc = key.getKeyCode();
        if (kc == 'a' || kc == 'A') { switchBank (0); return true; }
        if (kc == 'b' || kc == 'B') { switchBank (1); return true; }
        if (kc == 'c' || kc == 'C') { switchBank (2); return true; }
        if (kc == 'd' || kc == 'D') { switchBank (3); return true; }
        if (kc == 'l' || kc == 'L') { btnLoop.triggerClick(); return true; }
        if (kc == '.')               { stopAll(); return true; }
        if (key.getKeyCode() == juce::KeyPress::deleteKey ||
            key.getKeyCode() == juce::KeyPress::backspaceKey)
        {
            if (selectedKey >= 0) clearKey (selectedKey);
            return true;
        }
        if (key.getKeyCode() == juce::KeyPress::returnKey)
        {
            if (selectedKey >= 0) startKey (selectedKey, false);
            return true;
        }
        return false;
    }

    // No modifier — map to trigger keys
    char ch = (char)key.getTextCharacter();
    if (ch >= 'A' && ch <= 'Z') ch = (char)tolower (ch);

    // Space bar — also mode-aware
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        for (int i = 0; i < NUM_KEYS; ++i)
        {
            if (TRIGGER_KEYS[i] == ' ')
            {
                switch (currentMode)
                {
                    case UIMode::BankSelect:   switchBank (i); enterNormalMode(); break;
                    case UIMode::AssignHotKey: selectKey (i); openFileChooserForKey (i); enterNormalMode(); break;
                    case UIMode::HotList:      addToHotList (i); break;
                    default:                   fireKey (i); break;
                }
                return true;
            }
        }
        return false;
    }

    // All other trigger keys — route through the same mode-aware dispatch as on-screen keys
    for (int i = 0; i < NUM_KEYS; ++i)
    {
        if (TRIGGER_KEYS[i] != '\0' && TRIGGER_KEYS[i] == ch)
        {
            switch (currentMode)
            {
                case UIMode::BankSelect:
                    switchBank (i);
                    enterNormalMode();
                    break;
                case UIMode::AssignHotKey:
                    selectKey (i);
                    openFileChooserForKey (i);
                    enterNormalMode();
                    break;
                case UIMode::HotList:
                    addToHotList (i);
                    break;
                case UIMode::Normal:
                default:
                    fireKey (i);
                    break;
            }
            return true;
        }
    }

    return false;
}

bool MainComponent::keyStateChanged (bool /*isDown*/, juce::Component*)
{
    return false;
}

// ── UI updates ────────────────────────────────────────────
void MainComponent::highlightTransport()
{
    btnLoop.setColour (juce::TextButton::buttonColourId,
                       loopAll ? juce::Colour (0xff2a5a7a) : C::BTN_FACE);
}

void MainComponent::setStatus (const juce::String& msg)
{
    statusLabel.setText (msg, juce::dontSendNotification);
}

juce::String MainComponent::formatTime (double sec) const
{
    if (sec < 0.0) return "0:00.0";
    const int    m = (int)(sec / 60.0);
    const double s = sec - m * 60.0;
    return juce::String::formatted ("%d:%04.1f", m, s);
}
