# Shortcut Pro — Broadcast Audio Editor

A professional broadcast-grade soundboard modeled on the **360 Systems Shortcut** cart machine, built with JUCE C++ for low-latency audio playback.

**188 assignable slots** across a full QWERTY keyboard layout and 4 banks — pre-labeled with generic broadcast-ready cues, stings, beds, IDs, and utility cuts.

A web prototype (`index.html`) is also included for quick previewing in a browser.

---

## Features

- **47 trigger keys** across the full keyboard (`` ` 1–0 -=`` / QWERTY / ASDF / ZXCV / Space)
- **4 banks** (Ctrl+A/B/C/D) = 188 total assignable slots
- **ASIO / WASAPI support** — sub-10ms latency with a proper audio interface
- **64-voice polyphony** — every key fires independently; nothing stops anything else
- **5 playback modes**: FIRE/STOP · PLAY-END · MOMENTARY · LOOP FIRE · RESTART
- **Per-slot controls**: gain, trim-in, trim-out
- **Live waveform display** with trim handles and animated playhead
- **16-segment LED VU meters** with peak hold (L/R)
- **Drag-and-drop** audio files directly onto keys
- **Right-click context menu**: load, rename, clear, copy to another key

---

## Building

### Requirements

| Tool | Version |
|------|---------|
| Visual Studio 2022 | 17.x (Community is free) |
| CMake | ≥ 3.22 |
| Git | any |

CMake fetches JUCE automatically — no separate JUCE install needed.

### Quick build (Windows)

```powershell
git clone https://github.com/punktilend/AtomicButton.git
cd AtomicButton

cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

.\build\AtomicButton_artefacts\Release\"Shortcut Pro.exe"
```

### macOS

```bash
cmake -B build -G Xcode
cmake --build build --config Release
open build/AtomicButton_artefacts/Release/ShortcutPro.app
```

### Linux

```bash
# Install JUCE system deps first:
sudo apt install libasound2-dev libfreetype-dev libcurl4-openssl-dev \
                 libwebkit2gtk-4.1-dev libgtk-3-dev

cmake -B build
cmake --build build --config Release
./build/AtomicButton_artefacts/Release/ShortcutPro
```

---

## ASIO (Windows — broadcast recommended)

ASIO gives sub-10ms round-trip latency for live radio and TV use.

1. Download the **ASIO SDK 2.3.3** from [Steinberg](https://www.steinberg.net/asiosdk/) (free registration required)
2. Extract to e.g. `C:\SDKs\ASIOSDK2.3.3`
3. Pass the path to CMake:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DASIO_SDK_DIR="C:\SDKs\ASIOSDK2.3.3"
```

4. In-app: **Options → Audio Device Settings** → select your ASIO device

Without ASIO the app uses **WASAPI Exclusive** (~10–20ms) or **DirectSound** (~20–40ms) automatically.

---

## Keyboard layout

```
` 1 2 3 4 5 6 7 8 9 0 - =        ← 13 keys  (Bank row 1)
  q w e r t y u i o p [ ]        ← 12 keys  (Bank row 2)
   a s d f g h j k l ; '         ← 11 keys  (Bank row 3)
    z x c v b n m , . /          ← 10 keys  (Bank row 4)
              [  space  ]         ←  1 key
```

| Shortcut | Action |
|----------|--------|
| Any trigger key | Fire / toggle clip |
| `Ctrl+A` / `B` / `C` / `D` | Switch bank |
| `Ctrl+L` | Toggle loop-all |
| `Ctrl+.` | Kill all voices |
| `Enter` | Play selected slot |
| `Ctrl+Del` | Clear selected slot |
| Drag file onto key | Assign audio |
| Right-click key | Load · Rename · Clear · Copy |

---

## Bank presets

| Bank | Theme |
|------|-------|
| **A** | Imaging / opens / IDs / stagers |
| **B** | Beds / hooks / bumpers / promos |
| **C** | Interviews / actuality / production elements |
| **D** | Comedy / specialty / utility / FX |

All slot names are pre-labeled and fully renameable.

---

## Supported audio formats

| Format | Notes |
|--------|-------|
| WAV / AIFF | Always supported |
| FLAC | Built in |
| OGG Vorbis | Built in |
| MP3 | Disabled by default (Fraunhofer licence required) |

---

## Project structure

```
AtomicButton/
├── CMakeLists.txt              Build system (JUCE via FetchContent)
├── BUILD.md                   Detailed build + ASIO setup guide
├── Source/
│   ├── Main.cpp               JUCE application entry point
│   ├── SoundSlot.h            Data model — slots, voices, preset names
│   ├── AudioEngine.h/.cpp     ASIO/WASAPI audio, 64-voice polyphony
│   ├── WaveformDisplay.h/.cpp LCD waveform, trim handles, playhead
│   ├── VUMeterComponent.h/.cpp 16-seg LED VU with peak hold
│   ├── KeyboardComponent.h/.cpp Full keyboard grid, drag-drop, menus
│   └── MainComponent.h/.cpp   Top-level UI, transport, bank switching
├── index.html                 Web prototype (no install required)
├── style.css                  Broadcast aesthetic CSS
└── soundboard.js              Web prototype JS
```

---

## Web prototype

Open `index.html` in any modern browser for a zero-install preview.  
Drag audio files onto keys, use the same keyboard shortcuts.  
Latency is browser-dependent (~30–100ms) — fine for previewing, not for broadcast.

---

## License

MIT — see [LICENSE](LICENSE)

The ASIO SDK is © Steinberg Media Technologies and is **not included**.  
JUCE is © Raw Material Software — see [JUCE licence](https://juce.com/juce-7-licence/) for terms.
