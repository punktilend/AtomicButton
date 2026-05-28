#include "KeyboardComponent.h"

// ── Display chars ─────────────────────────────────────────
juce::String KeyboardComponent::keyDisplayChar (int idx)
{
    if (idx < 0 || idx >= NUM_KEYS) return {};
    const char k = TRIGGER_KEYS[idx];
    if (k == '\0') return {};
    if (k == ' ')  return "SPC";
    if (k == '\'') return "'";
    if (k == '-')  return "-";
    if (k == '=')  return "=";
    if (k == ',')  return ",";
    if (k == '.')  return ".";
    if (k == '/')  return "/";
    return juce::String::charToString ((juce::juce_wchar)toupper(k));
}

juce::String KeyboardComponent::keyLabel (int idx)
{
    if (idx < 0 || idx >= NUM_KEYS) return {};
    const char k = TRIGGER_KEYS[idx];
    if (k == '\0') return {};
    if (k == ' ')  return "SPACE";
    return juce::String::charToString ((juce::juce_wchar)toupper(k));
}

// ── Construction ──────────────────────────────────────────
KeyboardComponent::KeyboardComponent()
{
    for (int i = 0; i < NUM_KEYS; ++i)
        cells[i].keyIndex = i;
}

void KeyboardComponent::setBank (int bankIndex, std::array<SoundSlot, NUM_KEYS>* slots)
{
    currentBank  = bankIndex;
    currentSlots = slots;
    refreshAll();
}

void KeyboardComponent::setActiveKey (int ki)
{
    for (auto& c : cells) c.isSelected = false;
    if (ki >= 0 && ki < NUM_KEYS) cells[ki].isSelected = true;
    repaint();
}

void KeyboardComponent::setKeyPlaying (int ki, bool playing)
{
    if (ki >= 0 && ki < NUM_KEYS)
    {
        cells[ki].isPlaying = playing;
        repaint();
    }
}

void KeyboardComponent::refreshKey (int ki)
{
    if (ki >= 0 && ki < NUM_KEYS && currentSlots)
    {
        cells[ki].isLoaded = (*currentSlots)[ki].isLoaded();
        repaint();
    }
}

void KeyboardComponent::refreshAll()
{
    for (int i = 0; i < NUM_KEYS; ++i)
    {
        cells[i].isLoaded  = currentSlots ? (*currentSlots)[i].isLoaded() : false;
        cells[i].isPlaying = false;
    }
    repaint();
}

void KeyboardComponent::setUIMode (int mode)
{
    uiMode = mode;
    repaint();
}

void KeyboardComponent::setHotList (const std::vector<int>& list)
{
    hotListKeys = list;
    repaint();
}

// ── Layout ────────────────────────────────────────────────
void KeyboardComponent::buildLayout()
{
    const int w = getWidth();
    const int h = getHeight();

    // Available area after margins and bottom label
    const int gridW = w - 2 * MARGIN - 9 * GAP;
    const int gridH = h - 2 * MARGIN - LABEL_H - 4 * GAP;

    const int keyW = gridW / 10;
    const int keyH = gridH / 5;

    for (int i = 0; i < NUM_KEYS; ++i)
    {
        const int row = i / 10;
        const int col = i % 10;
        const int x   = MARGIN + col * (keyW + GAP);
        const int y   = MARGIN + row * (keyH + GAP);
        cells[i].bounds = { x, y, keyW, keyH };
    }
}

void KeyboardComponent::resized()
{
    buildLayout();
}

// ── Hit test ─────────────────────────────────────────────
int KeyboardComponent::keyIndexAt (int x, int y) const
{
    for (int i = NUM_KEYS - 1; i >= 0; --i)
        if (cells[i].bounds.contains (x, y))
            return i;
    return -1;
}

// ── Painting ─────────────────────────────────────────────
void KeyboardComponent::paint (juce::Graphics& g)
{
    // IR2-style light gray background
    g.fillAll (juce::Colour (0xffe4e8ec));

    // Draw all keys
    for (int i = 0; i < NUM_KEYS; ++i)
        paintKey (g, cells[i]);

    // "HOT-KEYS" label at bottom-left
    g.setColour (juce::Colour (0xff1e3a5f));
    g.setFont (juce::Font ("Courier New", 11.0f, juce::Font::bold));
    g.drawText ("HOT-KEYS",
                MARGIN, getHeight() - LABEL_H + 2,
                120, LABEL_H - 2,
                juce::Justification::centredLeft, false);

    // Mode banner
    if (uiMode != 0)
    {
        juce::String modeLabel;
        juce::Colour modeCol;
        if (uiMode == 1) { modeLabel = "[ BANK SELECT MODE ]";   modeCol = juce::Colour (0xffffe066); }
        if (uiMode == 2) { modeLabel = "[ ASSIGN HOT KEY MODE ]"; modeCol = juce::Colour (0xff66bbff); }
        if (uiMode == 3) { modeLabel = "[ HOT LIST MODE ]";       modeCol = juce::Colour (0xff66ff99); }
        g.setColour (modeCol.withAlpha (0.15f));
        g.fillRect (getLocalBounds());
        g.setColour (modeCol);
        g.setFont (juce::Font ("Courier New", 11.0f, juce::Font::bold));
        g.drawText (modeLabel, MARGIN, getHeight() - LABEL_H + 2, getWidth() - 2 * MARGIN, LABEL_H - 2,
                    juce::Justification::centredRight, false);
    }
}

void KeyboardComponent::paintKey (juce::Graphics& g, const KeyCell& cell)
{
    if (!currentSlots) return;
    const auto& slot  = (*currentSlots)[cell.keyIndex];
    const auto& r     = cell.bounds;
    if (r.isEmpty()) return;

    const bool loaded   = cell.isLoaded;
    const bool playing  = cell.isPlaying;
    const bool selected = cell.isSelected;
    const bool dragOver = (dragOverKey == cell.keyIndex);

    // Key face gradient
    juce::ColourGradient bg;
    if (playing)
    {
        bg = juce::ColourGradient (juce::Colour(0xffe8fff0), 0.0f, (float)r.getY(),
                                    juce::Colour(0xffc0e8cc), 0.0f, (float)r.getBottom(), false);
    }
    else if (selected)
    {
        bg = juce::ColourGradient (juce::Colour(0xfff0f4ff), 0.0f, (float)r.getY(),
                                    juce::Colour(0xffd8e4f8), 0.0f, (float)r.getBottom(), false);
    }
    else
    {
        bg = juce::ColourGradient (juce::Colour(0xffffffff), 0.0f, (float)r.getY(),
                                    juce::Colour(0xffe8eaed), 0.0f, (float)r.getBottom(), false);
    }
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r.toFloat(), 5.0f);

    // Border
    juce::Colour borderCol;
    if      (dragOver)  borderCol = juce::Colour (0xffffaa00);
    else if (playing)   borderCol = juce::Colour (0xff1fb86d);
    else if (selected)  borderCol = juce::Colour (0xff4a79be);
    else                borderCol = juce::Colour (0xffb0b8c0);
    g.setColour (borderCol);
    g.drawRoundedRectangle (r.toFloat(), 5.0f, playing || selected ? 1.5f : 1.0f);

    // Drop shadow strip below key
    g.setColour (juce::Colour (0x20000000));
    g.fillRect (r.getX(), r.getBottom(), r.getWidth(), 2);

    // ── Mode overlays ──────────────────────────────────────
    if (uiMode == 1)  // BankSelect — show bank number large
    {
        g.setColour (juce::Colour (0xcc1a2a3a));
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
        g.setFont (juce::Font ("Courier New", 14.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);
        g.drawText ("BNK\n" + juce::String (cell.keyIndex + 1),
                    r, juce::Justification::centred, false);
        return;
    }
    if (uiMode == 2 && !loaded)  // AssignHotKey — highlight empty keys
    {
        g.setColour (juce::Colour (0x40ffffaa));
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
    }
    if (uiMode == 3)  // HotList — show queue position if in list
    {
        auto it = std::find (hotListKeys.begin(), hotListKeys.end(), cell.keyIndex);
        if (it != hotListKeys.end())
        {
            const int pos = (int)(it - hotListKeys.begin()) + 1;
            g.setColour (juce::Colour (0x601a6a3a));
            g.fillRoundedRectangle (r.toFloat(), 5.0f);
            // Queue number badge top-right
            g.setFont (juce::Font ("Courier New", 10.0f, juce::Font::bold));
            g.setColour (juce::Colour (0xff44ff88));
            g.drawText ("#" + juce::String (pos),
                        r.getRight() - 24, r.getY() + 2, 22, 14,
                        juce::Justification::topRight, false);
        }
    }

    // LED strip at top (3px)
    juce::Colour ledCol;
    if      (playing)  ledCol = juce::Colour (0xff00ff44);
    else if (loaded)   ledCol = juce::Colour (0xffffaa44);
    else               ledCol = juce::Colour (0xff2a3038);
    g.setColour (ledCol);
    g.fillRect (r.getX(), r.getY(), r.getWidth(), 3);

    // Mini waveform (subtle, in center area)
    if (loaded && !slot.waveformPeaks.empty())
    {
        const int waveTop  = r.getY() + 3;
        const int waveBot  = r.getBottom() - 32;   // clear the taller label strip
        const int waveH    = juce::jmax (1, waveBot - waveTop);
        const int waveW    = r.getWidth();
        const auto& peaks  = slot.waveformPeaks;
        const int count    = (int)peaks.size();
        const float opacity = playing ? 0.85f : 0.4f;
        g.setColour (juce::Colour(0xffe8283a).withAlpha (opacity));
        for (int x = 0; x < waveW; ++x)
        {
            const int idx = juce::jlimit (0, count - 1, (int)((float)x / waveW * count));
            const float peak = peaks[idx];
            const int ph = juce::jmax (1, (int)(peak * waveH * 0.85f));
            g.fillRect (r.getX() + x, waveTop + waveH / 2 - ph / 2, 1, ph);
        }
    }

    // Label area background (bottom strip)
    const int labelH = loaded ? 30 : 20;
    const int labelY = r.getBottom() - labelH;
    g.setColour (loaded ? juce::Colour (0x28000000) : juce::Colour (0x12000000));
    g.fillRect (r.getX(), labelY, r.getWidth(), labelH);

    // Slot number — small, top-left of label strip
    const float numFontSize = 11.0f;
    g.setFont (juce::Font ("Courier New", numFontSize, juce::Font::bold));
    g.setColour (juce::Colour (0xffe8283a));
    g.drawText (juce::String (cell.keyIndex + 1),
                r.getX() + 3, labelY + 1, 24, 12,
                juce::Justification::topLeft, false);

    // Clip name — shown when loaded, centred in label strip
    if (loaded && slot.name.isNotEmpty())
    {
        const juce::Colour nameCol = playing
            ? juce::Colour (0xff003318)
            : juce::Colour (0xff1a2a3a);
        g.setFont (juce::Font ("Courier New", 9.0f, juce::Font::bold));
        g.setColour (nameCol);
        // Trim to fit, leave room for number on left
        const juce::String display = slot.name.toUpperCase().substring (0, 12);
        g.drawText (display,
                    r.getX() + 3, labelY + 14,
                    r.getWidth() - 6, 13,
                    juce::Justification::centredLeft, true);
    }

    // Keyboard trigger char (top-right corner, small gray)
    const juce::String trigChar = keyDisplayChar (cell.keyIndex);
    if (trigChar.isNotEmpty())
    {
        g.setFont (juce::Font ("Courier New", 8.0f, juce::Font::plain));
        g.setColour (juce::Colour (0xff8090a0));
        g.drawText (trigChar,
                    r.getRight() - 18, r.getY() + 3, 15, 11,
                    juce::Justification::topRight, false);
    }
}

// ── Mouse ────────────────────────────────────────────────
void KeyboardComponent::mouseDown (const juce::MouseEvent& e)
{
    const int ki = keyIndexAt (e.x, e.y);
    if (ki < 0) return;

    if (e.mods.isRightButtonDown())
    {
        if (onKeySelected) onKeySelected (ki);
        showContextMenu (ki);
        return;
    }

    if (onKeySelected) onKeySelected (ki);

    // Only fire if the key has a trigger character (not a blank slot)
    if (TRIGGER_KEYS[ki] != '\0')
        if (onKeyFired) onKeyFired (ki);
}

void KeyboardComponent::mouseUp (const juce::MouseEvent&) {}

void KeyboardComponent::showContextMenu (int ki)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Load audio file\xe2\x80\xa6");
    menu.addItem (5, "Edit Clip...", (*currentSlots)[ki].isLoaded());
    menu.addItem (2, "Rename");
    menu.addSeparator();
    menu.addItem (3, "Clear");
    menu.addItem (4, "Stop this key");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
        [this, ki] (int result)
        {
            if (!currentSlots) return;
            auto& slot = (*currentSlots)[ki];

            if (result == 1)
            {
                activeFileChooser = std::make_unique<juce::FileChooser> (
                    "Load audio for slot " + juce::String (ki + 1),
                    juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                    "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3");

                activeFileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                                    | juce::FileBrowserComponent::canSelectFiles,
                    [this, ki] (const juce::FileChooser& fc)
                    {
                        const auto resultFile = fc.getResult();
                        activeFileChooser.reset();

                        if (resultFile.existsAsFile() && onFileDrop)
                            onFileDrop (ki, resultFile);
                    });
            }
            else if (result == 2)
            {
                auto* renameWindow = new juce::AlertWindow (
                    "Rename slot " + juce::String (ki + 1),
                    "Enter new name:",
                    juce::AlertWindow::NoIcon);

                renameWindow->addTextEditor ("name", slot.name);
                renameWindow->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
                renameWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

                juce::Component::SafePointer<juce::AlertWindow> safeWindow (renameWindow);
                renameWindow->enterModalState (
                    true,
                    juce::ModalCallbackFunction::create ([this, ki, safeWindow] (int result)
                    {
                        if (result == 1 && safeWindow != nullptr && currentSlots)
                        {
                            const auto val = safeWindow->getTextEditorContents ("name");
                            if (val.isNotEmpty())
                                (*currentSlots)[ki].name = val.toUpperCase().substring (0, 14);
                        }
                        refreshKey (ki);
                    }),
                    true);
            }
            else if (result == 3)
            {
                slot = SoundSlot();
                slot.bankIndex = currentBank;
                slot.keyIndex  = ki;
                slot.key       = TRIGGER_KEYS[ki];
                slot.name      = (currentBank < 4) ? PRESET_NAMES[currentBank][ki] : ("SLOT " + juce::String (ki + 1));
                refreshKey (ki);
            }
            else if (result == 4)
            {
                if (cells[ki].isPlaying && onKeyFired)
                    onKeyFired (ki);
            }
            else if (result == 5)
            {
                if (onEditClip) onEditClip (ki);
            }
        });
}

// ── Drag & drop ──────────────────────────────────────────
bool KeyboardComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        juce::File file (f);
        auto ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".aif" ||
            ext == ".flac" || ext == ".ogg"  || ext == ".mp3")
            return true;
    }
    return false;
}

void KeyboardComponent::fileDragEnter (const juce::StringArray&, int x, int y)
{
    dragOverKey = keyIndexAt (x, y);
    repaint();
}
void KeyboardComponent::fileDragMove (const juce::StringArray&, int x, int y)
{
    const int newKey = keyIndexAt (x, y);
    if (newKey != dragOverKey) { dragOverKey = newKey; repaint(); }
}
void KeyboardComponent::fileDragExit (const juce::StringArray&)
{
    dragOverKey = -1;
    repaint();
}
void KeyboardComponent::filesDropped (const juce::StringArray& files, int x, int y)
{
    dragOverKey = -1;
    const int ki = keyIndexAt (x, y);
    if (ki >= 0 && files.size() > 0 && onFileDrop)
        onFileDrop (ki, juce::File (files[0]));
    repaint();
}
