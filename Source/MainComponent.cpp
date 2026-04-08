#include "MainComponent.h"

// ── Colour palette ────────────────────────────────────────
namespace C
{
    static const juce::Colour CHASSIS     (0xff1a1c1e);
    static const juce::Colour CHASSIS_DRK (0xff111315);
    static const juce::Colour PANEL       (0xff1e2022);
    static const juce::Colour RAIL        (0xff141618);
    static const juce::Colour LED_GREEN   (0xff00ff44);
    static const juce::Colour LED_AMBER   (0xffffaa00);
    static const juce::Colour LED_RED     (0xffff2233);
    static const juce::Colour LCD_BG      (0xff080f08);
    static const juce::Colour LCD_TEXT    (0xff00dd44);
    static const juce::Colour LCD_DIM     (0xff006622);
    static const juce::Colour TEXT_MAIN   (0xffb8bec4);
    static const juce::Colour TEXT_DIM    (0xff555e66);
    static const juce::Colour TEXT_LABEL  (0xff778899);
    static const juce::Colour BTN_FACE    (0xff2e3235);
    static const juce::Colour BTN_TOP     (0xff3a3e42);
}

static juce::Font monoFont (float size, int style = juce::Font::plain)
{
    return juce::Font ("Courier New", size, style);
}

// ── Construction ──────────────────────────────────────────
MainComponent::MainComponent()
{
    setSize (960, 780);

    initSlots();
    engine.initialise();
    engine.onVoiceEnded = [this] (int bank, int key)
    {
        juce::MessageManager::callAsync ([this, bank, key]()
        {
            if (bank == currentBank)
                keyboard.setKeyPlaying (key, false);
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
            auto& s    = banks[b][k];
            s.bankIndex = b;
            s.keyIndex  = k;
            s.key       = TRIGGER_KEYS[k];
            s.name      = PRESET_NAMES[b][k];
        }
}

// ── Component setup ───────────────────────────────────────
void MainComponent::initComponents()
{
    // Title
    addAndMakeVisible (titleLabel);
    titleLabel.setFont (monoFont (13.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, C::LED_GREEN);
    titleLabel.setText ("\xe2\x96\xa0 SHORTCUT PRO", juce::dontSendNotification);

    addAndMakeVisible (subLabel);
    subLabel.setFont (monoFont (8.0f));
    subLabel.setColour (juce::Label::textColourId, C::TEXT_DIM);
    subLabel.setText ("WEIRD AL UNIVERSE EDITION  \xc2\xb7  188-SLOT BROADCAST KEYBOARD", juce::dontSendNotification);

    addAndMakeVisible (clockLabel);
    clockLabel.setFont (monoFont (12.0f));
    clockLabel.setColour (juce::Label::textColourId, C::TEXT_DIM);
    clockLabel.setJustificationType (juce::Justification::right);

    addAndMakeVisible (timerLabel);
    timerLabel.setFont (monoFont (18.0f));
    timerLabel.setColour (juce::Label::textColourId, C::LCD_TEXT);
    timerLabel.setJustificationType (juce::Justification::right);
    timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93", juce::dontSendNotification);

    // LCD info
    for (auto* l : { &lcdBankLabel, &lcdClipLabel, &lcdDurLabel, &lcdPosLabel })
    {
        addAndMakeVisible (l);
        l->setFont (monoFont (11.0f));
        l->setColour (juce::Label::textColourId, C::LCD_DIM);
    }
    lcdClipLabel.setColour (juce::Label::textColourId, C::LCD_TEXT);
    lcdBankLabel.setText ("BANK A", juce::dontSendNotification);
    lcdClipLabel.setText ("-- NO CLIP LOADED --", juce::dontSendNotification);
    lcdDurLabel.setText  ("0:00.0", juce::dontSendNotification);
    lcdPosLabel.setText  ("> 0:00.0", juce::dontSendNotification);

    // VU meters
    addAndMakeVisible (vuLeft);
    addAndMakeVisible (vuRight);

    // Waveform
    addAndMakeVisible (waveform);

    // Keyboard
    addAndMakeVisible (keyboard);
    keyboard.setBank (currentBank, &banks[currentBank]);

    // Transport buttons
    auto setupTransport = [&] (juce::TextButton& btn, const juce::String& txt, const juce::String& tooltip)
    {
        addAndMakeVisible (btn);
        btn.setButtonText (txt);
        btn.setTooltip (tooltip);
        btn.setColour (juce::TextButton::buttonColourId,   C::BTN_FACE);
        btn.setColour (juce::TextButton::textColourOffId,  C::TEXT_MAIN);
    };
    setupTransport (btnRew,     juce::CharPointer_UTF8("\xe2\x8f\xae"), "Rewind");
    setupTransport (btnPlay,    juce::CharPointer_UTF8("\xe2\x96\xb6"), "Play selected [Enter]");
    setupTransport (btnPause,   juce::CharPointer_UTF8("\xe2\x8f\xb8"), "Pause/Resume [Ctrl+P]");
    setupTransport (btnStop,    juce::CharPointer_UTF8("\xe2\x96\xa0"), "Stop selected");
    setupTransport (btnLoop,    juce::CharPointer_UTF8("\xe2\x9f\xb3"), "Loop toggle [Ctrl+L]");
    setupTransport (btnClear,   "CLR", "Clear selected slot [Ctrl+Del]");
    setupTransport (btnKillAll, "KILL", "Stop all voices [Ctrl+.]");

    btnClear.setColour   (juce::TextButton::textColourOffId, C::LED_RED);
    btnKillAll.setColour (juce::TextButton::textColourOffId, C::LED_RED);

    // Mode buttons
    auto setupMode = [&] (juce::TextButton& btn, const juce::String& txt)
    {
        addAndMakeVisible (btn);
        btn.setButtonText (txt);
        btn.setColour (juce::TextButton::buttonColourId,  C::BTN_FACE);
        btn.setColour (juce::TextButton::textColourOffId, C::TEXT_DIM);
    };
    setupMode (modeFireStop,  "FIRE/STOP");
    setupMode (modePlayEnd,   "PLAY-END");
    setupMode (modeMomentary, "MOMENTARY");
    setupMode (modeLoopFire,  "LOOP FIRE");
    setupMode (modeRestart,   "RESTART");
    modeFireStop.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff002f1e));
    modeFireStop.setColour (juce::TextButton::textColourOffId, C::LED_GREEN);

    // Bank buttons
    for (int b = 0; b < 4; ++b)
    {
        addAndMakeVisible (bankBtns[b]);
        bankBtns[b].setButtonText (juce::String::charToString ('A' + b));
        bankBtns[b].setColour (juce::TextButton::buttonColourId,  C::BTN_FACE);
        bankBtns[b].setColour (juce::TextButton::textColourOffId, C::TEXT_DIM);
    }
    bankBtns[0].setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff005522));
    bankBtns[0].setColour (juce::TextButton::textColourOffId, C::LED_GREEN);

    // Sliders
    auto setupSlider = [&] (juce::Slider& s, juce::Label& lbl, const juce::String& lblTxt,
                            double lo, double hi, double val)
    {
        addAndMakeVisible (s);
        addAndMakeVisible (lbl);
        s.setRange (lo, hi, 0.01);
        s.setValue (val, juce::dontSendNotification);
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::trackColourId,    C::LED_GREEN);
        s.setColour (juce::Slider::backgroundColourId, C::CHASSIS_DRK);
        lbl.setFont (monoFont (8.0f));
        lbl.setColour (juce::Label::textColourId, C::TEXT_LABEL);
        lbl.setText (lblTxt, juce::dontSendNotification);
    };
    setupSlider (gainSlider,    gainLabel,    "GAIN",     0.0, 2.0, 1.0);
    setupSlider (trimInSlider,  trimInLabel,  "TRIM IN",  0.0, 1.0, 0.0);
    setupSlider (trimOutSlider, trimOutLabel, "TRIM OUT", 0.0, 1.0, 1.0);

    addAndMakeVisible (masterVolSlider);
    masterVolSlider.setRange (0.0, 1.0, 0.01);
    masterVolSlider.setValue (0.85, juce::dontSendNotification);
    masterVolSlider.setSliderStyle (juce::Slider::LinearVertical);
    masterVolSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterVolSlider.setColour (juce::Slider::trackColourId,     C::LED_GREEN);
    masterVolSlider.setColour (juce::Slider::backgroundColourId, C::CHASSIS_DRK);

    addAndMakeVisible (masterVolLabel);
    masterVolLabel.setFont (monoFont (8.0f));
    masterVolLabel.setColour (juce::Label::textColourId, C::TEXT_LABEL);
    masterVolLabel.setText ("MASTER", juce::dontSendNotification);
    masterVolLabel.setJustificationType (juce::Justification::centred);

    // Status bar
    addAndMakeVisible (statusLabel);
    statusLabel.setFont (monoFont (10.0f));
    statusLabel.setColour (juce::Label::textColourId, C::TEXT_DIM);
    statusLabel.setText ("READY -- Drop audio onto keys. Right-click for options. Ctrl+A/B/C/D switch banks.",
                          juce::dontSendNotification);

    addAndMakeVisible (statusModeLabel);
    statusModeLabel.setFont (monoFont (10.0f));
    statusModeLabel.setColour (juce::Label::textColourId, C::LCD_DIM);
    statusModeLabel.setText ("MODE: FIRE/STOP | 0 PLAYING", juce::dontSendNotification);
    statusModeLabel.setJustificationType (juce::Justification::right);
}

// ── Callbacks ─────────────────────────────────────────────
void MainComponent::bindCallbacks()
{
    keyboard.onKeyFired    = [this] (int ki) { fireKey (ki);   };
    keyboard.onKeySelected = [this] (int ki) { selectKey (ki); };
    keyboard.onFileDrop    = [this] (int ki, const juce::File& f) { loadFileForKey (ki, f); };

    btnRew.onClick  = [this] { engine.stopAll(); keyboard.refreshAll(); waveform.clearPlayhead(); };
    btnPlay.onClick = [this] { if (selectedKey >= 0) startKey (selectedKey, false); };
    btnPause.onClick= [this] {
        auto& dm = engine.getDeviceManager();
        if (auto* d = dm.getCurrentAudioDevice())
        {
            if (d->isPlaying()) { d->stop(); btnPause.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff005522)); }
            else                { dm.restartLastAudioDevice(); btnPause.setColour(juce::TextButton::buttonColourId, C::BTN_FACE); }
        }
    };
    btnStop.onClick    = [this] { if (selectedKey >= 0) stopKey (selectedKey); };
    btnKillAll.onClick = [this] { stopAll(); };
    btnLoop.onClick    = [this] {
        loopAll = !loopAll;
        highlightTransport();
        setStatus (juce::String ("LOOP ALL: ") + (loopAll ? "ON" : "OFF"));
    };
    btnClear.onClick   = [this] { if (selectedKey >= 0) clearKey (selectedKey); };

    // Bank buttons
    for (int b = 0; b < 4; ++b)
        bankBtns[b].onClick = [this, b] { switchBank (b); };

    // Mode buttons
    modeFireStop.onClick  = [this] { setMode (PlayMode::FireStop);  };
    modePlayEnd.onClick   = [this] { setMode (PlayMode::PlayEnd);   };
    modeMomentary.onClick = [this] { setMode (PlayMode::Momentary); };
    modeLoopFire.onClick  = [this] { setMode (PlayMode::LoopFire);  };
    modeRestart.onClick   = [this] { setMode (PlayMode::Restart);   };

    // Gain slider
    gainSlider.onValueChange = [this] {
        if (selectedKey < 0) return;
        currentSlotForKey(selectedKey).gain = (float)gainSlider.getValue();
    };

    // Trim in
    trimInSlider.onValueChange = [this] {
        if (selectedKey < 0) return;
        auto& s = currentSlotForKey(selectedKey);
        s.trimIn = (float)trimInSlider.getValue();
        waveform.setSlot (&s);
    };

    // Trim out
    trimOutSlider.onValueChange = [this] {
        if (selectedKey < 0) return;
        auto& s = currentSlotForKey(selectedKey);
        s.trimOut = (float)trimOutSlider.getValue();
        waveform.setSlot (&s);
    };

    // Master vol
    masterVolSlider.onValueChange = [this] {
        // Set on the audio device manager via gain node (handled in engine)
        // For now: scale all voices via master gain concept
        // The engine itself doesn't have a master gain; apply to device output
        // A simple approach: store and scale in engine. We'll use a rough workaround
        // by adjusting the output volume directly (platform-specific).
        // For now, note that per-slot gains handle individual levels.
    };

    // Audio device settings button
    // (Can be triggered from menu: Options > Audio Device Settings)
}

// ── Layout ────────────────────────────────────────────────
void MainComponent::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    const int TOP_H    = 44;
    const int STATUS_H = 26;
    const int KB_H     = 380;
    const int BODY_H   = H - TOP_H - STATUS_H - KB_H;
    const int LEFT_W   = 80;
    const int RIGHT_W  = 130;

    // Top rail
    topRailBounds = { 0, 0, W, TOP_H };
    titleLabel.setBounds (12, 6, 320, 16);
    subLabel.setBounds   (12, 22, 420, 12);
    clockLabel.setBounds (W - 180, 6, 170, 16);
    timerLabel.setBounds (W - 180, 20, 170, 18);

    // Main body
    mainBodyBounds  = { 0, TOP_H, W, BODY_H };
    leftPanelBounds = { 0, TOP_H, LEFT_W, BODY_H };
    rightPanelBounds= { W - RIGHT_W, TOP_H, RIGHT_W, BODY_H };
    centerPanelBounds={ LEFT_W, TOP_H, W - LEFT_W - RIGHT_W, BODY_H };

    // Left panel — VU meters + master vol
    const int vuY = TOP_H + 20;
    const int vuH = BODY_H - 60;
    vuLeft.setBounds  (8,  vuY, 22, vuH);
    vuRight.setBounds (34, vuY, 22, vuH);
    masterVolSlider.setBounds (62, vuY, 14, vuH - 16);
    masterVolLabel.setBounds  (56, vuY + vuH - 16, 26, 12);

    // Center panel
    const int cx = LEFT_W + 10;
    const int cy = TOP_H  + 8;
    const int cw = W - LEFT_W - RIGHT_W - 20;

    // LCD / waveform
    const int lcdH = BODY_H - 90;
    waveform.setBounds (cx, cy, cw, lcdH);
    // LCD labels below waveform
    const int liY = cy + lcdH + 2;
    lcdBankLabel.setBounds (cx,          liY, 60,       14);
    lcdClipLabel.setBounds (cx + 60,     liY, cw - 160, 14);
    lcdDurLabel.setBounds  (cx + cw - 90,liY, 45,       14);
    lcdPosLabel.setBounds  (cx + cw - 45,liY, 45,       14);

    // Transport
    const int tY  = liY + 18;
    const int tW  = 44;
    const int tH  = 32;
    const int tGap= 5;
    int tx = cx;
    for (auto* btn : { &btnRew, &btnPlay, &btnPause, &btnStop, &btnLoop, &btnClear, &btnKillAll })
    {
        btn->setBounds (tx, tY, tW, tH);
        tx += tW + tGap;
    }

    // Mode buttons
    const int mY  = tY + tH + 5;
    const int mW  = 74;
    const int mH  = 22;
    int mx = cx;
    for (auto* btn : { &modeFireStop, &modePlayEnd, &modeMomentary, &modeLoopFire, &modeRestart })
    {
        btn->setBounds (mx, mY, mW, mH);
        mx += mW + 4;
    }

    // Right panel
    const int rx = W - RIGHT_W + 8;
    int ry = TOP_H + 12;
    for (int b = 0; b < 4; ++b)
    {
        const int col = b % 2, row = b / 2;
        bankBtns[b].setBounds (rx + col * 54, ry + row * 32, 50, 28);
    }
    ry += 72;
    gainLabel.setBounds    (rx, ry,      114, 12); ry += 12;
    gainSlider.setBounds   (rx, ry,      114, 18); ry += 20;
    trimInLabel.setBounds  (rx, ry,      114, 12); ry += 12;
    trimInSlider.setBounds (rx, ry,      114, 18); ry += 20;
    trimOutLabel.setBounds (rx, ry,      114, 12); ry += 12;
    trimOutSlider.setBounds(rx, ry,      114, 18);

    // Keyboard
    keyboardBounds = { 0, TOP_H + BODY_H, W, KB_H };
    keyboard.setBounds (keyboardBounds);

    // Status bar
    statusBounds = { 0, H - STATUS_H, W, STATUS_H };
    statusLabel.setBounds     (8,         H - STATUS_H + 5, W - 250, 16);
    statusModeLabel.setBounds (W - 248,   H - STATUS_H + 5, 240,    16);
}

// ── Paint chassis ─────────────────────────────────────────
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (C::CHASSIS);

    // Top rail
    g.setColour (C::RAIL);
    g.fillRect  (topRailBounds);
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (topRailBounds.removeFromBottom (2));

    // Panels border
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (leftPanelBounds.getRight() - 1, mainBodyBounds.getY(), 1, mainBodyBounds.getHeight());
    g.fillRect  (rightPanelBounds.getX(),         mainBodyBounds.getY(), 1, mainBodyBounds.getHeight());

    // LCD surround
    const int lcdX = leftPanelBounds.getRight() + 10;
    const int lcdY = mainBodyBounds.getY() + 8;
    const int lcdW = rightPanelBounds.getX() - lcdX - 10;
    const int lcdH = mainBodyBounds.getHeight() - 90;
    g.setColour (C::LCD_BG);
    g.fillRect  (lcdX - 4, lcdY - 4, lcdW + 8, lcdH + 8);
    g.setColour (juce::Colour(0xff004400));
    g.drawRect  (lcdX - 5, lcdY - 5, lcdW + 10, lcdH + 10, 1);

    // Keyboard section
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (keyboardBounds);
    g.setColour (juce::Colour(0xff0a0b0c));
    g.fillRect  (keyboardBounds.removeFromTop(1));

    // Status bar
    g.setColour (C::RAIL);
    g.fillRect  (statusBounds);
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (statusBounds.removeFromTop (1));
}

// ── Timer (60fps) ─────────────────────────────────────────
void MainComponent::timerCallback()
{
    updateClockLabel();
    updateTimerLabel();

    vuLeft.setLevel  (engine.getVULevel (0));
    vuRight.setLevel (engine.getVULevel (1));

    // Update playhead
    if (selectedKey >= 0 && isPlaying (selectedKey))
    {
        const auto& slot = currentSlotForKey (selectedKey);
        if (slot.isLoaded())
        {
            const double pos     = engine.getVoicePosition (currentBank, selectedKey);
            const double segDur  = (slot.trimOut - slot.trimIn) * slot.duration;
            const double frac    = segDur > 0.0 ? pos / segDur : 0.0;
            waveform.setPlayheadFraction (frac);
        }
    }

    // Update poly count in status
    int count = 0;
    for (int ki = 0; ki < NUM_KEYS; ++ki)
        if (engine.isVoiceActive(currentBank, ki)) ++count;
    statusModeLabel.setText (
        "MODE: " + modeToString() + "  |  " + juce::String(count) + " PLAYING",
        juce::dontSendNotification);
}

juce::String MainComponent::modeToString() const
{
    switch (playMode)
    {
        case PlayMode::FireStop:  return "FIRE/STOP";
        case PlayMode::PlayEnd:   return "PLAY-END";
        case PlayMode::Momentary: return "MOMENTARY";
        case PlayMode::LoopFire:  return "LOOP FIRE";
        case PlayMode::Restart:   return "RESTART";
    }
    return {};
}

void MainComponent::updateClockLabel()
{
    const auto t = juce::Time::getCurrentTime();
    clockLabel.setText (t.toString (false, true, true, false), juce::dontSendNotification);
}

void MainComponent::updateTimerLabel()
{
    if (selectedKey < 0 || !isPlaying(selectedKey)) return;
    const double pos = engine.getVoicePosition (currentBank, selectedKey);
    const auto& slot = currentSlotForKey(selectedKey);
    const double elapsed = pos;
    const int min  = (int)(elapsed / 60.0);
    const double s = elapsed - min * 60.0;
    timerLabel.setText (juce::String::formatted ("%d:%04.1f", min, s), juce::dontSendNotification);
}

// ── Playback logic ────────────────────────────────────────
void MainComponent::fireKey (int ki)
{
    auto& slot = currentSlotForKey(ki);
    if (!slot.isLoaded()) return;

    switch (playMode)
    {
        case PlayMode::FireStop:
            if (isPlaying(ki)) stopKey(ki);
            else               startKey(ki, false);
            break;
        case PlayMode::PlayEnd:
            startKey(ki, false);
            break;
        case PlayMode::LoopFire:
            if (isPlaying(ki)) stopKey(ki);
            else               startKey(ki, true);
            break;
        case PlayMode::Restart:
            startKey(ki, false);   // always restarts from trimIn
            break;
        case PlayMode::Momentary:
            startKey(ki, false);   // held down
            break;
    }
}

void MainComponent::startKey (int ki, bool forceLoop)
{
    auto& slot = currentSlotForKey(ki);
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
        timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93", juce::dontSendNotification);
    }
}

void MainComponent::stopAll()
{
    engine.stopAll();
    keyboard.refreshAll();
    waveform.clearPlayhead();
    timerLabel.setText ("\xe2\x80\x93:\xe2\x80\x93\xe2\x80\x93.\xe2\x80\x93", juce::dontSendNotification);
    setStatus ("STOP -- all voices killed");
}

void MainComponent::selectKey (int ki)
{
    selectedKey = ki;
    keyboard.setActiveKey (ki);
    const auto& slot = currentSlotForKey(ki);
    lcdClipLabel.setText (slot.isLoaded() ? slot.name : "-- NO CLIP LOADED --",
                           juce::dontSendNotification);
    lcdDurLabel.setText  (slot.isLoaded() ? formatTime(slot.duration) : "0:00.0",
                           juce::dontSendNotification);
    waveform.setSlot (slot.isLoaded() ? &slot : nullptr);
    updateSlotControls();
}

void MainComponent::switchBank (int b)
{
    currentBank = b;
    updateBankButtons();
    keyboard.setBank (b, &banks[b]);
    lcdBankLabel.setText ("BANK " + juce::String::charToString('A' + b),
                           juce::dontSendNotification);
    if (selectedKey >= 0) selectKey (selectedKey);
    setStatus ("Bank " + juce::String::charToString('A' + b)
               + " -- " + juce::String(countLoaded(b)) + " clips loaded");
}

int MainComponent::countLoaded (int b) const
{
    int n = 0;
    for (const auto& s : banks[b]) if (s.isLoaded()) ++n;
    return n;
}

void MainComponent::setMode (PlayMode mode)
{
    playMode = mode;
    updateModeButtons();
}

void MainComponent::loadFileForKey (int ki, const juce::File& file)
{
    auto& slot = currentSlotForKey(ki);
    setStatus ("Loading: " + file.getFileName() + "...");
    bool ok = engine.loadFile (slot, file);
    if (ok)
    {
        slot.name = file.getFileNameWithoutExtension().toUpperCase().substring(0,14);
        keyboard.refreshKey (ki);
        selectKey (ki);
        setStatus ("Loaded: " + slot.name + "  (" + formatTime(slot.duration) + ")");
    }
    else
    {
        setStatus ("ERROR: Could not decode " + file.getFileName());
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
    slot.name      = PRESET_NAMES[currentBank][ki];
    keyboard.refreshKey (ki);
    if (ki == selectedKey) selectKey (ki);
    setStatus ("Key [" + juce::String::charToString((juce::juce_wchar)toupper(TRIGGER_KEYS[ki])) + "] cleared");
}

// ── Keyboard input ────────────────────────────────────────
bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const auto mods = key.getModifiers();

    // Ctrl combos
    if (mods.isCommandDown() || mods.isCtrlDown())
    {
        const int kc = key.getKeyCode();
        if (kc == 'a' || kc == 'A') { switchBank(0); return true; }
        if (kc == 'b' || kc == 'B') { switchBank(1); return true; }
        if (kc == 'c' || kc == 'C') { switchBank(2); return true; }
        if (kc == 'd' || kc == 'D') { switchBank(3); return true; }
        if (kc == 'l' || kc == 'L') { btnLoop.triggerClick(); return true; }
        if (kc == '.')               { stopAll(); return true; }
        if (key.getKeyCode() == juce::KeyPress::deleteKey ||
            key.getKeyCode() == juce::KeyPress::backspaceKey)
        { if (selectedKey >= 0) clearKey(selectedKey); return true; }
        if (key.getKeyCode() == juce::KeyPress::returnKey)
        { if (selectedKey >= 0) startKey(selectedKey,false); return true; }
        return false;
    }

    // No modifier — map to trigger keys
    if (playMode == PlayMode::Momentary) return true; // handled in keyState

    char ch = (char)key.getTextCharacter();
    // JUCE returns uppercase for alpha keys without shift, so lowercase
    if (ch >= 'A' && ch <= 'Z') ch = (char)tolower(ch);

    for (int i = 0; i < TRIGGER_KEY_COUNT; ++i)
    {
        if (TRIGGER_KEYS[i] == ch)
        {
            fireKey (i);
            return true;
        }
    }

    // Space bar
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        fireKey (TRIGGER_KEY_COUNT - 1);  // space is last
        return true;
    }

    return false;
}

bool MainComponent::keyStateChanged (bool isDown, juce::Component*)
{
    if (playMode != PlayMode::Momentary) return false;

    for (int i = 0; i < TRIGGER_KEY_COUNT; ++i)
    {
        char ch = TRIGGER_KEYS[i];
        juce::KeyPress kp (ch, juce::ModifierKeys::noModifiers, (juce::juce_wchar)ch);
        const bool held = juce::KeyPress::isKeyCurrentlyDown (ch);
        if (held && isDown  && !isPlaying(i)) startKey(i, false);
        if (!held && !isDown && isPlaying(i)) stopKey(i);
    }
    return false;
}

// ── UI updates ────────────────────────────────────────────
void MainComponent::updateBankButtons()
{
    for (int b = 0; b < 4; ++b)
    {
        const bool active = (b == currentBank);
        bankBtns[b].setColour (juce::TextButton::buttonColourId,
                               active ? juce::Colour(0xff005522) : C::BTN_FACE);
        bankBtns[b].setColour (juce::TextButton::textColourOffId,
                               active ? C::LED_GREEN : C::TEXT_DIM);
    }
}

void MainComponent::updateModeButtons()
{
    struct Pair { PlayMode m; juce::TextButton* b; };
    Pair pairs[] = {
        {PlayMode::FireStop,  &modeFireStop},
        {PlayMode::PlayEnd,   &modePlayEnd},
        {PlayMode::Momentary, &modeMomentary},
        {PlayMode::LoopFire,  &modeLoopFire},
        {PlayMode::Restart,   &modeRestart},
    };
    for (auto& p : pairs)
    {
        const bool active = (p.m == playMode);
        p.b->setColour (juce::TextButton::buttonColourId,
                        active ? juce::Colour(0xff002f1e) : C::BTN_FACE);
        p.b->setColour (juce::TextButton::textColourOffId,
                        active ? C::LED_GREEN : C::TEXT_DIM);
    }
}

void MainComponent::updateSlotControls()
{
    if (selectedKey < 0) return;
    const auto& slot = currentSlotForKey(selectedKey);
    gainSlider.setValue    (slot.gain,    juce::dontSendNotification);
    trimInSlider.setValue  (slot.trimIn,  juce::dontSendNotification);
    trimOutSlider.setValue (slot.trimOut, juce::dontSendNotification);
}

void MainComponent::highlightTransport()
{
    btnLoop.setColour (juce::TextButton::buttonColourId,
                       loopAll ? juce::Colour(0xff3a2a00) : C::BTN_FACE);
    btnLoop.setColour (juce::TextButton::textColourOffId,
                       loopAll ? C::LED_AMBER : C::TEXT_MAIN);
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
