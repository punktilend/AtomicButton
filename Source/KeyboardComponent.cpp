#include "KeyboardComponent.h"

// Key layout — row offsets in units (1 unit = KEY_W + KEY_GAP)
// Row 0: number row (13 keys), offset 0
// Row 1: QWERTY (12 keys),     offset 1.5 units
// Row 2: ASDF   (11 keys),     offset 1.75 units
// Row 3: ZXCV   (10 keys),     offset 2.25 units
// Row 4: Space   (1 key wide), offset 3.75 units

static constexpr int KEY_W   = 60;
static constexpr int KEY_H   = 68;
static constexpr int KEY_GAP = 3;
static constexpr int KEY_UNIT = KEY_W + KEY_GAP;

// Row start indices into TRIGGER_KEYS[]
static constexpr int ROW_STARTS[5] = { 0, 13, 25, 36, 46 };
static constexpr int ROW_COUNTS[5] = { 13, 12, 11, 10, 1 };
// Offsets in pixels for each row (fractional units → pixels)
// Row 0: 0, Row 1: 1.5u, Row 2: 1.75u, Row 3: 2.25u, Row 4: space centered
static const float ROW_OFFSETS[5] = { 0.0f, 1.5f, 1.75f, 2.25f, 3.75f };

static constexpr int SPACE_KEY_WIDTH = (int)(KEY_UNIT * 4.0f);

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
    const int startX = 8;
    const int startY = 4;

    int ki = 0;
    for (int row = 0; row < 5; ++row)
    {
        const int count  = ROW_COUNTS[row];
        const int xOff   = startX + (int)(ROW_OFFSETS[row] * KEY_UNIT);
        const int y      = startY + row * (KEY_H + KEY_GAP);

        for (int col = 0; col < count; ++col)
        {
            if (ki >= NUM_KEYS) break;
            const int w = (TRIGGER_KEYS[ki] == ' ') ? SPACE_KEY_WIDTH : KEY_W;
            cells[ki].bounds = { xOff + col * KEY_UNIT, y, w, KEY_H };
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
    g.fillAll (juce::Colour (0xff131517));

    // Modifier placeholders (non-functional keys for visual authenticity)
    // Tab
    g.setColour (juce::Colour (0xff111315));
    auto modRect = [&](int col, int row, float widthUnits, const char* lbl)
    {
        const int x = 8 + (int)(ROW_OFFSETS[row] * KEY_UNIT) + col * KEY_UNIT;
        const int y = 4 + row * (KEY_H + KEY_GAP);
        const int w = (int)(KEY_UNIT * widthUnits) - KEY_GAP;
        juce::Rectangle<int> r(x, y, w, KEY_H);
        g.setColour (juce::Colour (0xff111315));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (juce::Colour (0xff2a3a2a));
        g.drawRoundedRectangle (r.toFloat(), 4.0f, 1.0f);
        g.setColour (juce::Colour (0xff2a3a2a));
        g.setFont (juce::Font ("Courier New", 8.0f, juce::Font::plain));
        g.drawText (lbl, r, juce::Justification::bottomLeft, false);
    };

    // Row offsets shifted so mods appear to the left of key rows
    // Tab: left of row1
    {
        const int x = 8;
        const int y = 4 + 1 * (KEY_H + KEY_GAP);
        const int w = (int)(KEY_UNIT * 1.5f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,KEY_H);
        g.setColour (juce::Colour (0xff111315));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff1a2a1a));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff2a4a2a));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("TAB",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Caps: left of row2
    {
        const int x = 8;
        const int y = 4 + 2 * (KEY_H + KEY_GAP);
        const int w = (int)(KEY_UNIT * 1.75f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,KEY_H);
        g.setColour(juce::Colour(0xff111315));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff1a2a1a));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff2a4a2a));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("CAPS",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Shift: left of row3
    {
        const int x = 8;
        const int y = 4 + 3 * (KEY_H + KEY_GAP);
        const int w = (int)(KEY_UNIT * 2.25f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,KEY_H);
        g.setColour(juce::Colour(0xff111315));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff1a2a1a));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff2a4a2a));
        g.setFont(juce::Font("Courier New",7.0f,juce::Font::plain));
        g.drawText("SHIFT",r.reduced(4),juce::Justification::bottomLeft,false);
    }
    // Enter: right of row2
    {
        const int x = 8 + (int)(ROW_OFFSETS[2] * KEY_UNIT) + ROW_COUNTS[2] * KEY_UNIT;
        const int y = 4 + 2 * (KEY_H + KEY_GAP);
        const int w = (int)(KEY_UNIT * 2.25f) - KEY_GAP;
        juce::Rectangle<int> r(x,y,w,KEY_H);
        g.setColour(juce::Colour(0xff111315));
        g.fillRoundedRectangle(r.toFloat(),4.0f);
        g.setColour(juce::Colour(0xff1a2a1a));
        g.drawRoundedRectangle(r.toFloat(),4.0f,1.0f);
        g.setColour(juce::Colour(0xff2a4a2a));
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
        bg = juce::ColourGradient (juce::Colour(0xff003a18), 0.0f, (float)r.getY(),
                                    juce::Colour(0xff001508), 0.0f, (float)r.getBottom(), false);
    }
    else
    {
        bg = juce::ColourGradient (juce::Colour(0xff2c3035), 0.0f, (float)r.getY(),
                                    juce::Colour(0xff131517), 0.0f, (float)r.getBottom(), false);
    }
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r.toFloat(), 4.0f);

    // Border
    juce::Colour borderCol;
    if      (dragOver)  borderCol = juce::Colour (0xffffaa00);
    else if (playing)   borderCol = juce::Colour (0xff00cc44);
    else if (selected)  borderCol = juce::Colour (0xff4488ff);
    else if (loaded)    borderCol = juce::Colour (0xff1a3a1a);
    else                borderCol = juce::Colour (0xff0c0d0e);
    g.setColour (borderCol);
    g.drawRoundedRectangle (r.toFloat(), 4.0f, 1.0f);

    // Drop shadow / depth
    g.setColour (juce::Colour (0x88000000));
    g.fillRect (r.getX(), r.getBottom(), r.getWidth(), 3);

    // LED strip (top 3px)
    juce::Colour ledCol;
    if      (playing) ledCol = juce::Colour (0xff00ff44);
    else if (loaded)  ledCol = juce::Colour (0xff143a14);
    else              ledCol = juce::Colour (0xff0a140a);
    g.setColour (ledCol);
    g.fillRect  (r.getX(), r.getY(), r.getWidth(), 3);

    // Mini waveform
    if (loaded && !slot.waveformPeaks.empty())
    {
        const int waveY = r.getY() + 3;
        const int waveH = r.getHeight() - 3 - 18;
        const int waveW = r.getWidth();
        const auto& peaks = slot.waveformPeaks;
        const int count = (int)peaks.size();
        const float opacity = playing ? 0.9f : (loaded ? 0.5f : 0.2f);
        g.setColour (juce::Colour(0xff00ff44).withAlpha (opacity));
        for (int x = 0; x < waveW; ++x)
        {
            const int idx = juce::jlimit (0, count-1, (int)((float)x / waveW * count));
            const float peak = peaks[idx];
            const int   h    = juce::jmax (1, (int)(peak * waveH * 0.9f));
            g.fillRect (r.getX() + x, waveY + waveH/2 - h/2, 1, h);
        }
    }

    // Label strip (bottom 18px)
    const int labelY = r.getBottom() - 18;
    g.setColour (juce::Colour (0x99000000));
    g.fillRect  (r.getX(), labelY, r.getWidth(), 18);

    // Key character (bottom-left)
    g.setFont (juce::Font ("Courier New", 8.0f, juce::Font::bold));
    g.setColour (playing ? juce::Colour(0xff00ff44)
               : loaded  ? juce::Colour(0xff4a6a4a)
                         : juce::Colour(0xff2a3a2a));
    g.drawText (keyDisplayChar(cell.keyIndex),
                r.getX() + 3, labelY, 16, 16,
                juce::Justification::bottomLeft, false);

    // Clip name (bottom-right, truncated)
    g.setFont  (juce::Font ("Courier New", 6.5f, juce::Font::plain));
    g.setColour (playing ? juce::Colour(0xff00ff44)
               : loaded  ? juce::Colour(0xff7a9a7a)
                         : juce::Colour(0xff3a4a3a));
    g.drawText (slot.name,
                r.getX() + 19, labelY, r.getWidth() - 22, 16,
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
                juce::FileChooser fc ("Load audio for [" + keyLabel(ki) + "]",
                                      juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                      "*.wav;*.aiff;*.aif;*.flac;*.ogg;*.mp3");
                if (fc.browseForFileToOpen())
                    if (onFileDrop) onFileDrop (ki, fc.getResult());
            }
            else if (result == 2)
            {
                juce::AlertWindow::showInputBoxAsync (
                    "Rename key [" + keyLabel(ki) + "]", "Enter new name:",
                    slot.name, nullptr,
                    [this, ki] (const juce::String& val) {
                        if (val.isNotEmpty() && currentSlots)
                            (*currentSlots)[ki].name = val.toUpperCase().substring(0,14);
                        refreshKey (ki);
                    });
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
