#include "MainComponent.h"

// ── Colour palette ────────────────────────────────────────
namespace C
{
    static const juce::Colour ROOM        (0xff0a0a0b);
    static const juce::Colour CHASSIS     (0xffd8dde2);
    static const juce::Colour CHASSIS_DRK (0xffb8c0ca);
    static const juce::Colour PANEL       (0xffdfe4e8);
    static const juce::Colour PANEL_BLUE  (0xff3b5b86);
    static const juce::Colour RAIL        (0xffcfd5da);
    static const juce::Colour BEVEL       (0xfff3f5f7);
    static const juce::Colour SCREW       (0xff7e8794);
    static const juce::Colour PANEL_HI    (0xff8e99a6);
    static const juce::Colour LCD_FRAME   (0xff687585);
    static const juce::Colour LED_GREEN   (0xff00ff44);
    static const juce::Colour LED_AMBER   (0xffffaa00);
    static const juce::Colour LED_RED     (0xffff2233);
    static const juce::Colour LCD_BG      (0xff020406);
    static const juce::Colour LCD_TEXT    (0xff35c6ff);
    static const juce::Colour LCD_DIM     (0xff6ca7c2);
    static const juce::Colour TEXT_MAIN   (0xff25364f);
    static const juce::Colour TEXT_DIM    (0xff617081);
    static const juce::Colour TEXT_LABEL  (0xff7a8795);
    static const juce::Colour BTN_FACE    (0xffe8edf2);
    static const juce::Colour BTN_TOP     (0xfff8fafc);
}

static juce::Font monoFont (float size, int style = juce::Font::plain)
{
    return juce::Font ("Courier New", size, style);
}

static juce::String hotKeyLabelForIndex (int idx)
{
    if (idx < 0 || idx >= TRIGGER_KEY_COUNT)
        return {};

    const char key = TRIGGER_KEYS[idx];
    if (key == ' ')
        return "SPACE";

    return juce::String::charToString ((juce::juce_wchar) std::toupper (key));
}

static void drawPanelFrame (juce::Graphics& g, juce::Rectangle<int> r, float corner = 6.0f)
{
    g.setColour (juce::Colour (0xffd8dfe6));
    g.fillRoundedRectangle (r.toFloat(), corner);
    g.setColour (C::PANEL_HI);
    g.drawRoundedRectangle (r.toFloat(), corner, 1.0f);
    g.setColour (juce::Colour (0x55ffffff));
    g.drawRoundedRectangle (r.reduced (2).toFloat(), corner - 1.5f, 1.0f);
}

// ── Construction ──────────────────────────────────────────
MainComponent::MainComponent()
{
    setSize (1280, 900);

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
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff264d87));
    titleLabel.setText ("\xe2\x96\xa0 SHORTCUT PRO", juce::dontSendNotification);

    addAndMakeVisible (subLabel);
    subLabel.setFont (monoFont (8.0f));
    subLabel.setColour (juce::Label::textColourId, C::TEXT_DIM);
    subLabel.setText ("BROADCAST AUDIO EDITOR  \xc2\xb7  188-SLOT HOT KEY DECK", juce::dontSendNotification);

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
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff51667c));
    };
    setupTransport (btnRew,     juce::CharPointer_UTF8("\xe2\x8f\xae"), "Rewind");
    setupTransport (btnPlay,    juce::CharPointer_UTF8("\xe2\x96\xb6"), "Play selected [Enter]");
    setupTransport (btnPause,   juce::CharPointer_UTF8("\xe2\x8f\xb8"), "Pause/Resume [Ctrl+P]");
    setupTransport (btnStop,    juce::CharPointer_UTF8("\xe2\x96\xa0"), "Stop selected");
    setupTransport (btnLoop,    juce::CharPointer_UTF8("\xe2\x9f\xb3"), "Loop toggle [Ctrl+L]");
    setupTransport (btnClear,   "CLR", "Clear selected slot [Ctrl+Del]");
    setupTransport (btnKillAll, "KILL", "Stop all voices [Ctrl+.]");

    btnClear.setColour   (juce::TextButton::textColourOffId, juce::Colour (0xffe34a62));
    btnKillAll.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe34a62));

    auto setupEdit = [&] (juce::TextButton& btn, const juce::String& txt, const juce::String& tooltip, bool accent = false)
    {
        addAndMakeVisible (btn);
        btn.setButtonText (txt);
        btn.setTooltip (tooltip);
        btn.setColour (juce::TextButton::buttonColourId, accent ? juce::Colour (0xffeef3f9) : C::BTN_FACE);
        btn.setColour (juce::TextButton::textColourOffId, accent ? juce::Colour (0xff4e78b6) : C::TEXT_MAIN);
    };
    setupEdit (btnMark,   "MARK",   "Mark the currently selected hot key");
    setupEdit (btnZero,   "ZERO",   "Return to the first hot key in the current bank");
    setupEdit (btnGoTo,   "GO TO",  "Jump back to the marked hot key");
    setupEdit (btnFind,   "FIND",   "Find a cut in the current bank by name");
    setupEdit (btnCut,    "CUT",    "Copy the selected cut and clear its key");
    setupEdit (btnCopy,   "COPY",   "Copy the selected cut");
    setupEdit (btnInsert, "INSERT", "Paste the copied cut to the selected key");
    setupEdit (btnErase,  "ERASE",  "Clear the selected key", true);
    setupEdit (btnUndo,   "UNDO",   "Undo the last slot edit", true);
    setupEdit (btnZoomIn, "ZOOM+",  "Waveform zoom in");
    setupEdit (btnZoomOut,"ZOOM-",  "Waveform zoom out");

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
    modeFireStop.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xffeff6ec));
    modeFireStop.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff238a53));

    // Bank buttons
    for (int b = 0; b < 4; ++b)
    {
        addAndMakeVisible (bankBtns[b]);
        bankBtns[b].setButtonText (juce::String::charToString ('A' + b));
        bankBtns[b].setColour (juce::TextButton::buttonColourId,  C::BTN_FACE);
        bankBtns[b].setColour (juce::TextButton::textColourOffId, C::TEXT_DIM);
    }
    bankBtns[0].setColour (juce::TextButton::buttonColourId,  juce::Colour(0xffeff6ec));
    bankBtns[0].setColour (juce::TextButton::textColourOffId, juce::Colour (0xff238a53));

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
        s.setColour (juce::Slider::trackColourId,    juce::Colour (0xff43b7ff));
        s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2a2f36));
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
    masterVolSlider.setColour (juce::Slider::trackColourId,     juce::Colour (0xff43b7ff));
    masterVolSlider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2a2f36));

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
    btnMark.onClick    = [this] { markSelectedKey(); };
    btnZero.onClick    = [this] { selectKey (0); setStatus ("ZERO -- returned to first hot key in current bank"); };
    btnGoTo.onClick    = [this] { goToMarkedKey(); };
    btnFind.onClick    = [this] { findSlot(); };
    btnCut.onClick     = [this] { cutSelectedSlot(); };
    btnCopy.onClick    = [this] { copySelectedSlot(); };
    btnInsert.onClick  = [this] { insertClipboardToSelected(); };
    btnErase.onClick   = [this] { eraseSelectedSlot(); };
    btnUndo.onClick    = [this] { undoLastEdit(); };
    btnZoomIn.onClick  = [this] { setPendingStatus ("Waveform zoom in"); };
    btnZoomOut.onClick = [this] { setPendingStatus ("Waveform zoom out"); };

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
    auto available = getLocalBounds().reduced (18, 16);
    const int deckW = juce::jmin (available.getWidth(), 1280);
    const int deckH = juce::jmin (available.getHeight(), 930);
    hardwareBounds = available.withSizeKeepingCentre (deckW, deckH);

    auto deck = hardwareBounds;
    topRailBounds = deck.removeFromTop (46);
    statusBounds = deck.removeFromBottom (28);
    mainBodyBounds = deck;

    leftPanelBounds = mainBodyBounds.removeFromLeft (772);
    auto monitorZone = mainBodyBounds.removeFromLeft (70);
    auto controlZone = mainBodyBounds;
    rightPanelBounds = controlZone.removeFromRight (168);
    centerPanelBounds = controlZone;
    monitorBounds = monitorZone.reduced (6, 10);
    keyboardHeaderBounds = {};
    keyboardBounds = leftPanelBounds.reduced (14, 16);
    bankBounds = rightPanelBounds.reduced (10, 12).removeFromTop (96);
    slotBounds = rightPanelBounds.reduced (10, 12);
    slotBounds.removeFromTop (110);

    titleLabel.setBounds (topRailBounds.getX() + 20, topRailBounds.getY() + 7, 360, 16);
    subLabel.setBounds   (topRailBounds.getX() + 20, topRailBounds.getY() + 24, 470, 12);
    clockLabel.setBounds (topRailBounds.getRight() - 186, topRailBounds.getY() + 6, 170, 14);
    timerLabel.setBounds (topRailBounds.getRight() - 186, topRailBounds.getY() + 20, 170, 18);

    const auto meterInner = monitorZone.reduced (8, 20);
    const int vuY = meterInner.getY() + 40;
    const int vuH = 240;
    vuLeft.setBounds  (meterInner.getX() + 4, vuY, 12, vuH);
    vuRight.setBounds (meterInner.getX() + 22, vuY, 12, vuH);
    masterVolSlider.setBounds (meterInner.getX() + 42, vuY + 32, 16, 180);
    masterVolLabel.setBounds  (meterInner.getX() + 24, vuY + 220, 34, 12);

    auto centerInner = centerPanelBounds.reduced (12, 12);
    lcdBounds = centerInner.removeFromTop (juce::jlimit (220, 270, centerInner.getHeight() / 3));
    auto lcdInner = lcdBounds.reduced (12, 12);
    auto lcdInfoBounds = lcdInner.removeFromBottom (18);
    waveform.setBounds (lcdInner);
    lcdBankLabel.setBounds (lcdInfoBounds.getX(), lcdInfoBounds.getY(), 80, lcdInfoBounds.getHeight());
    lcdDurLabel.setBounds  (lcdInfoBounds.getRight() - 92, lcdInfoBounds.getY(), 44, lcdInfoBounds.getHeight());
    lcdPosLabel.setBounds  (lcdInfoBounds.getRight() - 44, lcdInfoBounds.getY(), 44, lcdInfoBounds.getHeight());
    lcdClipLabel.setBounds (lcdInfoBounds.getX() + 80, lcdInfoBounds.getY(), lcdInfoBounds.getWidth() - 176, lcdInfoBounds.getHeight());

    centerInner.removeFromTop (12);
    lowerDeckBounds = centerInner.removeFromTop (344);
    auto towerInner = lowerDeckBounds.reduced (10, 8);
    editClusterBounds = towerInner.removeFromTop (108);
    towerInner.removeFromTop (8);
    juce::Rectangle<int> scrubBand = towerInner.removeFromTop (86);
    towerInner.removeFromTop (8);
    transportClusterBounds = towerInner.removeFromTop (96);
    towerInner.removeFromTop (8);
    modeBounds = towerInner.removeFromTop (28);
    scrubWheelBounds = { scrubBand.getCentreX() - 34, scrubBand.getCentreY() - 34, 68, 68 };

    auto editInner = editClusterBounds.reduced (12, 26);
    const int editW = 76;
    const int editGap = 6;
    int row1X = editInner.getCentreX() - ((editW * 4 + editGap * 3) / 2);
    int row2X = row1X;
    btnMark.setBounds   (row1X, editInner.getY(), editW, 24); row1X += editW + editGap;
    btnZero.setBounds   (row1X, editInner.getY(), editW, 24); row1X += editW + editGap;
    btnGoTo.setBounds   (row1X, editInner.getY(), editW, 24); row1X += editW + editGap;
    btnFind.setBounds   (row1X, editInner.getY(), editW, 24);
    btnCut.setBounds    (row2X, editInner.getY() + 30, editW, 24); row2X += editW + editGap;
    btnCopy.setBounds   (row2X, editInner.getY() + 30, editW, 24); row2X += editW + editGap;
    btnInsert.setBounds (row2X, editInner.getY() + 30, editW, 24); row2X += editW + editGap;
    btnErase.setBounds  (row2X, editInner.getY() + 30, editW, 24);
    int row3X = editInner.getCentreX() - ((editW * 3 + editGap * 2) / 2);
    btnUndo.setBounds    (row3X, editInner.getY() + 60, editW, 24); row3X += editW + editGap;
    btnZoomIn.setBounds  (row3X, editInner.getY() + 60, editW, 24); row3X += editW + editGap;
    btnZoomOut.setBounds (row3X, editInner.getY() + 60, editW, 24);

    transportBounds = transportClusterBounds.withTrimmedTop (26).removeFromTop (76);
    const int tW = 64;
    const int tH = 32;
    const int tGap = 8;
    const int topRowY = transportBounds.getY();
    const int botRowY = transportBounds.getY() + tH + 10;
    const int topWidth = 4 * tW + 3 * tGap;
    const int botWidth = 3 * tW + 2 * tGap;
    int topX = transportBounds.getCentreX() - topWidth / 2;
    int botX = transportBounds.getCentreX() - botWidth / 2;
    btnRew.setBounds     (topX, topRowY, tW, tH); topX += tW + tGap;
    btnPlay.setBounds    (topX, topRowY, tW, tH); topX += tW + tGap;
    btnPause.setBounds   (topX, topRowY, tW, tH); topX += tW + tGap;
    btnStop.setBounds    (topX, topRowY, tW, tH);
    btnLoop.setBounds    (botX, botRowY, tW, tH); botX += tW + tGap;
    btnClear.setBounds   (botX, botRowY, tW, tH); botX += tW + tGap;
    btnKillAll.setBounds (botX, botRowY, tW, tH);

    const int mW = 76;
    const int mH = 24;
    const int mGap = 6;
    const int modeTotal = mW * 5 + mGap * 4;
    int modeX = modeBounds.getCentreX() - modeTotal / 2;
    const int modeY = modeBounds.getY();
    modeFireStop.setBounds  (modeX, modeY, mW, mH); modeX += mW + mGap;
    modePlayEnd.setBounds   (modeX, modeY, mW, mH); modeX += mW + mGap;
    modeMomentary.setBounds (modeX, modeY, mW, mH); modeX += mW + mGap;
    modeLoopFire.setBounds  (modeX, modeY, mW, mH); modeX += mW + mGap;
    modeRestart.setBounds   (modeX, modeY, mW, mH);

    auto rightInner = rightPanelBounds.reduced (10, 12);
    int ry = rightInner.getY() + 12;
    for (int b = 0; b < 4; ++b)
    {
        const int col = b % 2;
        const int row = b / 2;
        bankBtns[b].setBounds (rightInner.getX() + col * 62, ry + row * 40, 56, 34);
    }
    ry += 94;
    gainLabel.setBounds    (rightInner.getX(), ry, rightInner.getWidth(), 12); ry += 13;
    gainSlider.setBounds   (rightInner.getX(), ry, rightInner.getWidth(), 18); ry += 24;
    trimInLabel.setBounds  (rightInner.getX(), ry, rightInner.getWidth(), 12); ry += 13;
    trimInSlider.setBounds (rightInner.getX(), ry, rightInner.getWidth(), 18); ry += 24;
    trimOutLabel.setBounds (rightInner.getX(), ry, rightInner.getWidth(), 12); ry += 13;
    trimOutSlider.setBounds(rightInner.getX(), ry, rightInner.getWidth(), 18); ry += 28;
    voicesBounds = { rightInner.getX(), ry + 18, rightInner.getWidth(), 36 };

    keyboard.setBounds (keyboardBounds);
    statusLabel.setBounds     (statusBounds.getX() + 10, statusBounds.getY() + 5, statusBounds.getWidth() - 290, 16);
    statusModeLabel.setBounds (statusBounds.getRight() - 280, statusBounds.getY() + 5, 270, 16);
}

// ── Paint chassis ─────────────────────────────────────────
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (C::ROOM);

    g.setColour (C::CHASSIS);
    g.fillRoundedRectangle (hardwareBounds.toFloat(), 10.0f);
    juce::Path bluePanel;
    const float splitLeft = (float) leftPanelBounds.getRight() - 26.0f;
    const float splitPeak = (float) monitorBounds.getRight() + 36.0f;
    bluePanel.startNewSubPath (splitLeft, (float) hardwareBounds.getY());
    bluePanel.lineTo ((float) hardwareBounds.getRight(), (float) hardwareBounds.getY());
    bluePanel.lineTo ((float) hardwareBounds.getRight(), (float) hardwareBounds.getBottom());
    bluePanel.lineTo ((float) rightPanelBounds.getX() - 14.0f, (float) hardwareBounds.getBottom());
    bluePanel.lineTo (splitPeak, (float) hardwareBounds.getCentreY() + 10.0f);
    bluePanel.lineTo ((float) centerPanelBounds.getX() + 30.0f, (float) hardwareBounds.getCentreY() - 120.0f);
    bluePanel.closeSubPath();
    g.setColour (C::PANEL_BLUE);
    g.fillPath (bluePanel);
    g.setColour (juce::Colour (0x33ffffff));
    g.drawLine (splitLeft, (float) hardwareBounds.getY() + 6.0f,
                (float) centerPanelBounds.getX() + 26.0f, (float) hardwareBounds.getCentreY() - 118.0f, 2.0f);
    g.drawLine ((float) centerPanelBounds.getX() + 26.0f, (float) hardwareBounds.getCentreY() - 118.0f,
                splitPeak, (float) hardwareBounds.getCentreY() + 12.0f, 2.0f);
    g.drawLine (splitPeak, (float) hardwareBounds.getCentreY() + 12.0f,
                (float) rightPanelBounds.getX() - 10.0f, (float) hardwareBounds.getBottom() - 6.0f, 2.0f);
    g.setColour (C::BEVEL);
    g.drawRoundedRectangle (hardwareBounds.toFloat(), 10.0f, 1.0f);
    g.setColour (C::SCREW);
    for (const auto& p : { juce::Point<int> (hardwareBounds.getX() + 12, hardwareBounds.getY() + 12),
                           juce::Point<int> (hardwareBounds.getRight() - 12, hardwareBounds.getY() + 12),
                           juce::Point<int> (hardwareBounds.getX() + 12, hardwareBounds.getBottom() - 12),
                           juce::Point<int> (hardwareBounds.getRight() - 12, hardwareBounds.getBottom() - 12) })
        g.fillEllipse ((float) p.x - 3.0f, (float) p.y - 3.0f, 6.0f, 6.0f);

    // Top rail
    g.setColour (C::RAIL);
    g.fillRect  (topRailBounds);
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (topRailBounds.removeFromBottom (2));
    for (int i = 0; i < 4; ++i)
    {
        const float x = (float) topRailBounds.getCentreX() + 24.0f * (float) i - 48.0f;
        const juce::Colour lamp = i == 0 ? C::LED_GREEN : juce::Colour (0xff0c1a0c);
        g.setColour (lamp);
        g.fillEllipse (x, (float) topRailBounds.getY() + 13.0f, 10.0f, 10.0f);
        g.setColour (juce::Colour (0xff030303));
        g.drawEllipse (x, (float) topRailBounds.getY() + 13.0f, 10.0f, 10.0f, 1.0f);
    }

    // Panels border
    g.setColour (juce::Colour (0xffeef1f4));
    g.fillRect (leftPanelBounds);
    g.setColour (juce::Colour (0xff2f4768));
    g.fillRect (monitorBounds.withTrimmedLeft (-6).withTrimmedRight (-6));
    g.fillRect (centerPanelBounds);
    g.fillRect (rightPanelBounds);
    g.setColour (C::CHASSIS_DRK);
    g.fillRect  (leftPanelBounds.getRight() - 1, mainBodyBounds.getY(), 1, mainBodyBounds.getHeight());
    g.fillRect  (rightPanelBounds.getX(),         mainBodyBounds.getY(), 1, mainBodyBounds.getHeight());

    drawPanelFrame (g, monitorBounds, 8.0f);
    drawPanelFrame (g, bankBounds, 8.0f);
    drawPanelFrame (g, slotBounds, 8.0f);
    drawPanelFrame (g, editClusterBounds, 8.0f);
    drawPanelFrame (g, transportClusterBounds, 8.0f);

    // LCD surround
    const int lcdX = lcdBounds.getX();
    const int lcdY = lcdBounds.getY();
    const int lcdW = lcdBounds.getWidth();
    const int lcdH = lcdBounds.getHeight();
    g.setColour (C::LCD_BG);
    g.fillRect  (lcdX - 4, lcdY - 4, lcdW + 8, lcdH + 8);
    g.setColour (C::LCD_FRAME);
    g.drawRect  (lcdX - 5, lcdY - 5, lcdW + 10, lcdH + 10, 1);

    g.setColour (juce::Colour (0xff7b8797));
    g.setFont (monoFont (9.0f));
    g.drawText ("MONITOR", monitorBounds.getX(), monitorBounds.getY() + 8, monitorBounds.getWidth(), 12, juce::Justification::centredTop);
    g.setColour (juce::Colour (0xff9cb7d8));
    g.drawText ("EDITOR DISPLAY", lcdBounds.getX(), lcdBounds.getY() - 20, lcdBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("BANK", bankBounds.getX(), bankBounds.getY() + 8, bankBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("SLOT", slotBounds.getX(), slotBounds.getY() + 8, slotBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("EDIT", editClusterBounds.getX(), editClusterBounds.getY() + 8, editClusterBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("TRANSPORT", transportClusterBounds.getX(), transportClusterBounds.getY() + 8, transportClusterBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("SCRUB", scrubWheelBounds.getX() - 24, scrubWheelBounds.getY() - 18, scrubWheelBounds.getWidth() + 48, 12, juce::Justification::centredTop);
    g.drawText ("VOICES", voicesBounds.getX(), voicesBounds.getY() - 14, voicesBounds.getWidth(), 12, juce::Justification::centredTop);
    g.drawText ("PLAY MODES", lowerDeckBounds.getX(), modeBounds.getY() - 10, lowerDeckBounds.getWidth(), 12, juce::Justification::centredTop);

    g.setColour (juce::Colour (0xff0a0b0c));
    g.fillEllipse (scrubWheelBounds.toFloat());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3e44), (float) scrubWheelBounds.getCentreX() - 10.0f, (float) scrubWheelBounds.getY() + 10.0f,
                                            juce::Colour (0xff1a1c1e), (float) scrubWheelBounds.getCentreX() + 20.0f, (float) scrubWheelBounds.getBottom() - 10.0f, false));
    g.fillEllipse (scrubWheelBounds.reduced (3).toFloat());
    g.setColour (C::LED_AMBER);
    g.fillRoundedRectangle ((float) scrubWheelBounds.getCentreX() - 2.0f, (float) scrubWheelBounds.getY() + 8.0f, 4.0f, 12.0f, 2.0f);
    g.setColour (C::LCD_TEXT);
    g.setFont (monoFont (28.0f, juce::Font::bold));
    g.drawText (juce::String (activeVoiceCount), voicesBounds, juce::Justification::centred, false);

    g.setColour (juce::Colour (0xffc7d5e9));
    g.setFont (monoFont (10.0f));
    g.drawText ("CUT / MARK / FIND / UNDO", editClusterBounds.getX(), editClusterBounds.getBottom() - 24, editClusterBounds.getWidth(), 14, juce::Justification::centred, false);
    g.drawText ("FAST ACCESS PLAYBACK", transportClusterBounds.getX(), transportClusterBounds.getBottom() - 24, transportClusterBounds.getWidth(), 14, juce::Justification::centred, false);

    // Keyboard section
    g.setColour (juce::Colour (0xffe8ecef));
    g.fillRect  (keyboardBounds);
    g.setColour (juce::Colour(0xffb9c0c8));
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
    activeVoiceCount = count;
    statusModeLabel.setText (
        "MODE: " + modeToString() + "  |  " + juce::String(count) + " PLAYING",
        juce::dontSendNotification);
    repaint (rightPanelBounds);
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

void MainComponent::storeUndoFromKey (int ki)
{
    if (ki < 0 || ki >= NUM_KEYS)
        return;

    undoSlot = banks[currentBank][ki];
    undoKey = ki;
}

void MainComponent::copySelectedSlot ()
{
    if (!hasSelection())
    {
        setStatus ("COPY -- no hot key selected");
        return;
    }

    clipboardSlot = currentSlotForKey (selectedKey);
    setStatus ("COPY -- stored [" + hotKeyLabelForIndex (selectedKey) + "] in clipboard");
}

void MainComponent::cutSelectedSlot ()
{
    if (!hasSelection())
    {
        setStatus ("CUT -- no hot key selected");
        return;
    }

    copySelectedSlot();
    eraseSelectedSlot();
    setStatus ("CUT -- moved selected cut into clipboard");
}

void MainComponent::insertClipboardToSelected ()
{
    if (!hasSelection())
    {
        setStatus ("INSERT -- no destination hot key selected");
        return;
    }

    if (!clipboardSlot.has_value())
    {
        setStatus ("INSERT -- clipboard is empty");
        return;
    }

    storeUndoFromKey (selectedKey);
    auto pasted = *clipboardSlot;
    pasted.bankIndex = currentBank;
    pasted.keyIndex = selectedKey;
    pasted.key = TRIGGER_KEYS[selectedKey];
    banks[currentBank][selectedKey] = pasted;
    keyboard.refreshKey (selectedKey);
    selectKey (selectedKey);
    setStatus ("INSERT -- pasted clip to [" + hotKeyLabelForIndex (selectedKey) + "]");
}

void MainComponent::eraseSelectedSlot ()
{
    if (!hasSelection())
    {
        setStatus ("ERASE -- no hot key selected");
        return;
    }

    storeUndoFromKey (selectedKey);
    clearKey (selectedKey);
}

void MainComponent::undoLastEdit ()
{
    if (!undoSlot.has_value() || undoKey < 0 || undoKey >= NUM_KEYS)
    {
        setStatus ("UNDO -- nothing to restore");
        return;
    }

    banks[currentBank][undoKey] = *undoSlot;
    banks[currentBank][undoKey].bankIndex = currentBank;
    banks[currentBank][undoKey].keyIndex = undoKey;
    banks[currentBank][undoKey].key = TRIGGER_KEYS[undoKey];
    keyboard.refreshKey (undoKey);
    selectKey (undoKey);
    setStatus ("UNDO -- restored [" + hotKeyLabelForIndex (undoKey) + "]");
}

void MainComponent::markSelectedKey ()
{
    if (!hasSelection())
    {
        setStatus ("MARK -- no hot key selected");
        return;
    }

    markedKey = selectedKey;
    setStatus ("MARK -- stored location [" + hotKeyLabelForIndex (selectedKey) + "]");
}

void MainComponent::goToMarkedKey ()
{
    if (!markedKey.has_value())
    {
        setStatus ("GO TO -- no mark stored");
        return;
    }

    selectKey (*markedKey);
    setStatus ("GO TO -- returned to marked hot key");
}

void MainComponent::findSlot ()
{
    auto* findWindow = new juce::AlertWindow (
        "Find Cut",
        "Type part of a clip name in the current bank:",
        juce::AlertWindow::NoIcon);

    findWindow->addTextEditor ("query", "");
    findWindow->addButton ("Find", 1, juce::KeyPress (juce::KeyPress::returnKey));
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
                    setStatus ("FIND -- selected [" + hotKeyLabelForIndex (i) + "] " + banks[currentBank][i].name);
                    return;
                }
            }

            setStatus ("FIND -- no cut matched \"" + query + "\" in bank " + juce::String::charToString ('A' + currentBank));
        }),
        true);
}

void MainComponent::setPendingStatus (const juce::String& featureName)
{
    setStatus (featureName + " -- panel control added, deeper behavior coming next");
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
                               active ? juce::Colour(0xffeff6ec) : C::BTN_FACE);
        bankBtns[b].setColour (juce::TextButton::textColourOffId,
                               active ? juce::Colour (0xff238a53) : C::TEXT_DIM);
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
                        active ? juce::Colour(0xffeff6ec) : C::BTN_FACE);
        p.b->setColour (juce::TextButton::textColourOffId,
                        active ? juce::Colour (0xff238a53) : C::TEXT_DIM);
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
                       loopAll ? juce::Colour(0xfffff2db) : C::BTN_FACE);
    btnLoop.setColour (juce::TextButton::textColourOffId,
                       loopAll ? juce::Colour (0xffba7a00) : C::TEXT_MAIN);
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
