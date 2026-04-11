#include "KeyboardComponent.h"

// Key layout — row offsets in units (1 unit = KEY_W + KEY_GAP)
// Row 0: number row (13 keys), offset 0
// Row 1: QWERTY (12 keys),     offset 1.5 units
// Row 2: ASDF   (11 keys),     offset 1.75 units
// Row 3: ZXCV   (10 keys),     offset 2.25 units
// Row 4: Space   (1 key wide), offset 3.75 units

static constexpr int KEY_GAP = 4;

// Row start indices into TRIGGER_KEYS[]
static constexpr int ROW_STARTS[5] = { 0, 13, 25, 36, 46 };
static constexpr int ROW_COUNTS[5] = { 13, 12, 11, 10, 1 };
// Offsets in pixels for each row (fractional units → pixels)
// Row 0: 0, Row 1: 1.5u, Row 2: 1.75u, Row 3: 2.25u, Row 4: space centered
static const float ROW_OFFSETS[5] = { 0.0f, 1.5f, 1.75f, 2.25f, 3.75f };

// ── Display chars ─────────────────────────────────────────
juce::String KeyboardComponent::keyDisplayChar (int idx)
{
    if (idx < 0 || idx >= TRIGGER_KEY_COUNT) return {};
    char k = TRIGGER_KEYS[idx];
    if (k == ' ')  return "SPC";
    if (k == '\'') return "'";
    return juce::String::charToString ((juce::juce_wchar)toupper(k));
}

juce::String KeyboardComponent::keyLabel (int idx)
{
    if (idx < 0 || idx >= TRIGGER_KEY_COUNT) return {};
    char k = TRIGGER_KEYS[idx];
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

// ── Layout ────────────────────────────────────────────────
void KeyboardComponent::buildLayout()
{
    const auto area = getLocalBounds().reduced (12, 10);

    const int hotLabelH = 16;
    const int hotKeyH = 96;
    const int keyboardLabelH = 16;
    const int compactKeyH = 66;
    const int smallKeyW = 58;
    const int hotGap = 8;

    const int hotY = area.getY() + hotLabelH + 12;
    const int hotTotalW = area.getWidth() - 16;
    const int hotKeyW = juce::jmax (76, (hotTotalW - hotGap * 9) / 10);
    const int hotRowW = hotKeyW * 10 + hotGap * 9;
    const int hotX = area.getCentreX() - hotRowW / 2;

    for (int i = 0; i < NUM_KEYS; ++i)
    {
        cells[i].bounds = {};
        cells[i].isHotKey = (i < 10);
    }

    for (int i = 0; i < 10; ++i)
        cells[i].bounds = { hotX + i * (hotKeyW + hotGap), hotY, hotKeyW, hotKeyH };

    const int utilityY = hotY + hotKeyH + 10;
    const int utilityW = 54;
    const int utilityTotalW = utilityW * 3 + KEY_GAP * 2;
    const int utilityX = area.getRight() - utilityTotalW - 4;
    for (int i = 10; i <= 12; ++i)
        cells[i].bounds = { utilityX + (i - 10) * (utilityW + KEY_GAP), utilityY, utilityW, 38 };

    const int keyboardStartY = utilityY + 38 + keyboardLabelH + 12;
    const int keyboardStartX = area.getX() + 8;
    const int keyUnit = smallKeyW + KEY_GAP;
    const int spaceWidth = keyUnit * 4;

    int ki = 13;
    for (int row = 1; row < 5; ++row)
    {
        const int count = ROW_COUNTS[row];
        const int xOff = keyboardStartX + (int) (ROW_OFFSETS[row] * keyUnit);
        const int y = keyboardStartY + (row - 1) * (compactKeyH + KEY_GAP);

        for (int col = 0; col < count; ++col)
        {
            if (ki >= NUM_KEYS)
                break;

            const int w = (TRIGGER_KEYS[ki] == ' ') ? spaceWidth : smallKeyW;
            cells[ki].bounds = { xOff + col * keyUnit, y, w, compactKeyH };
            ++ki;
        }
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
    g.fillAll (juce::Colour (0xffedf0f2));

    g.setColour (juce::Colour (0xff6c7682));
    g.setFont (juce::Font ("Courier New", 9.0f, juce::Font::plain));
    g.drawText ("HOT KEYS", 12, 2, getWidth() - 24, 14, juce::Justification::centredLeft, false);
    g.drawText ("QWERTY / FIND / RENAME", 12, 146, getWidth() - 24, 14, juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xffbcc4cb));
    g.drawLine (12.0f, 18.0f, (float) getWidth() - 12.0f, 18.0f, 1.0f);
    g.drawLine (12.0f, 162.0f, (float) getWidth() - 12.0f, 162.0f, 1.0f);

    // Modifier placeholders (non-functional keys for visual authenticity)
    // Tab
    g.setColour (juce::Colour (0xff111315));
    const int smallKeyW = 58;
    const int compactKeyH = 66;
    const int keyUnit = smallKeyW + KEY_GAP;
    const int keyboardStartX = 20;
    const int keyboardStartY = 172;
    auto modRect = [&](int row, float widthUnits, const char* lbl)
    {
        const int x = keyboardStartX;
        const int y = keyboardStartY + (row - 1) * (compactKeyH + KEY_GAP);
        const int w = (int)(keyUnit * widthUnits) - KEY_GAP;
        juce::Rectangle<int> r(x, y, w, compactKeyH);
        g.setColour (juce::Colour (0xffd7dde2));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff9aa6b1));
        g.drawRoundedRectangle (r.toFloat(), 4.0f, 1.0f);
        g.setColour (juce::Colour (0xff7a8692));
        g.setFont (juce::Font ("Courier New", 8.0f, juce::Font::plain));
        g.drawText (lbl, r, juce::Justification::bottomLeft, false);
    };

    // Row offsets shifted so mods appear to the left of key rows
    // Tab: left of row1
    {
        const int x = keyboardStartX;
        const int y = keyboardStartY;
        const int w = (int)(keyUnit * 1.5f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,compactKeyH);
        g.setColour (juce::Colour (0xffd7dde2));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff9aa6b1));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff7a8692));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("TAB",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Caps: left of row2
    {
        const int x = keyboardStartX;
        const int y = keyboardStartY + 1 * (compactKeyH + KEY_GAP);
        const int w = (int)(keyUnit * 1.75f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,compactKeyH);
        g.setColour(juce::Colour(0xffd7dde2));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff9aa6b1));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff7a8692));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("CAPS",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Shift: left of row3
    {
        const int x = keyboardStartX;
        const int y = keyboardStartY + 2 * (compactKeyH + KEY_GAP);
        const int w = (int)(keyUnit * 2.25f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,compactKeyH);
        g.setColour(juce::Colour(0xffd7dde2));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff9aa6b1));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff7a8692));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("SHIFT",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Enter: right of row2
    {
        const int x = keyboardStartX + (int)(ROW_OFFSETS[2] * keyUnit) + ROW_COUNTS[2] * keyUnit;
        const int y = keyboardStartY + 1 * (compactKeyH + KEY_GAP);
        const int w = (int)(keyUnit * 2.25f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,compactKeyH);
        g.setColour(juce::Colour(0xffd7dde2));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff9aa6b1));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff7a8692));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("ENTER",r.reduced(4),juce::Justification::bottomLeft,false);
    }

    // Draw trigger keys
    for (int i = 0; i < NUM_KEYS; ++i)
        paintKey (g, cells[i]);
}

void KeyboardComponent::paintKey (juce::Graphics& g, const KeyCell& cell)
{
    if (!currentSlots) return;
    const auto& slot  = (*currentSlots)[cell.keyIndex];
    const auto& r     = cell.bounds;
    const bool  loaded   = cell.isLoaded;
    const bool  playing  = cell.isPlaying;
    const bool  selected = cell.isSelected;
    const bool  dragOver = (dragOverKey == cell.keyIndex);

    // Key face
    juce::ColourGradient bg;
    if (playing)
    {
        bg = juce::ColourGradient (juce::Colour(0xfff7fbf8), 0.0f, (float)r.getY(),
                                    juce::Colour(0xffdcebdd), 0.0f, (float)r.getBottom(), false);
    }
    else
    {
        bg = juce::ColourGradient (juce::Colour(0xfff8fafc), 0.0f, (float)r.getY(),
                                    juce::Colour(0xffd8dfe6), 0.0f, (float)r.getBottom(), false);
    }
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r.toFloat(), cell.isHotKey ? 6.0f : 4.0f);

    // Border
    juce::Colour borderCol;
    if      (dragOver)  borderCol = juce::Colour (0xffffaa00);
    else if (playing)   borderCol = juce::Colour (0xff1fb86d);
    else if (selected)  borderCol = juce::Colour (0xff4a79be);
    else if (loaded)    borderCol = juce::Colour (0xff9ba8b5);
    else                borderCol = juce::Colour (0xffb3bcc5);
    g.setColour (borderCol);
    g.drawRoundedRectangle (r.toFloat(), cell.isHotKey ? 6.0f : 4.0f, cell.isHotKey ? 1.5f : 1.0f);

    // Drop shadow / depth
    g.setColour (juce::Colour (0x22000000));
    g.fillRect (r.getX(), r.getBottom(), r.getWidth(), 3);

    // LED strip (top 3px)
    juce::Colour ledCol;
    if      (playing) ledCol = juce::Colour (0xff00ff44);
    else if (loaded)  ledCol = juce::Colour (0xffc8ced5);
    else              ledCol = juce::Colour (0xffd8dfe5);
    g.setColour (ledCol);
    g.fillRect  (r.getX(), r.getY(), r.getWidth(), 3);

    // Mini waveform
    if (loaded && !slot.waveformPeaks.empty())
    {
    const int waveY = r.getY() + 3;
    const int waveH = r.getHeight() - 3 - (cell.isHotKey ? 24 : 18);
        const int waveW = r.getWidth();
        const auto& peaks = slot.waveformPeaks;
        const int count = (int)peaks.size();
        const float opacity = playing ? 0.9f : (loaded ? 0.5f : 0.2f);
        g.setColour (juce::Colour(0xffe14561).withAlpha (opacity));
        for (int x = 0; x < waveW; ++x)
        {
            const int idx = juce::jlimit (0, count-1, (int)((float)x / waveW * count));
            const float peak = peaks[idx];
            const int   h    = juce::jmax (1, (int)(peak * waveH * 0.9f));
            g.fillRect (r.getX() + x, waveY + waveH/2 - h/2, 1, h);
        }
    }

    // Label strip (bottom 18px)
    const int labelH = cell.isHotKey ? 28 : 18;
    const int labelY = r.getBottom() - labelH;
    g.setColour (juce::Colour (0x15000000));
    g.fillRect  (r.getX(), labelY, r.getWidth(), labelH);

    // Key character (bottom-left)
    g.setFont (juce::Font ("Courier New", cell.isHotKey ? 12.0f : 8.0f, juce::Font::bold));
    g.setColour (playing ? juce::Colour(0xffd64d66)
               : loaded  ? juce::Colour(0xffd64d66)
                         : juce::Colour(0xff718091));
    g.drawText (keyDisplayChar(cell.keyIndex),
                r.getX() + 4, labelY, cell.isHotKey ? 26 : 16, labelH - 2,
                juce::Justification::bottomLeft, false);

    // Clip name (bottom-right, truncated)
    g.setFont  (juce::Font ("Courier New", cell.isHotKey ? 8.0f : 6.5f, juce::Font::plain));
    g.setColour (playing ? juce::Colour(0xffa63a50)
               : loaded  ? juce::Colour(0xff7a8695)
                         : juce::Colour(0xff8c98a6));
    g.drawText (slot.name,
                r.getX() + (cell.isHotKey ? 28 : 19), labelY, r.getWidth() - (cell.isHotKey ? 34 : 22), labelH - 2,
                juce::Justification::bottomRight, true);
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
    if (onKeyFired)    onKeyFired    (ki);
}

void KeyboardComponent::mouseUp (const juce::MouseEvent&) {}

void KeyboardComponent::showContextMenu (int ki)
{
    juce::PopupMenu menu;
    menu.addItem (1, "Load audio file\xe2\x80\xa6");
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
                    "Load audio for [" + keyLabel (ki) + "]",
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
                    "Rename key [" + keyLabel (ki) + "]",
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
                slot.name      = PRESET_NAMES[currentBank][ki];
                refreshKey (ki);
            }
            else if (result == 4)
            {
                // Handled by parent via key state check
                if (cells[ki].isPlaying && onKeyFired)
                    onKeyFired (ki);   // FIRE/STOP mode toggles off
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
