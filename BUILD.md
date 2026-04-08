# Shortcut Pro — Build Guide

## Requirements

| Tool | Version | Notes |
|------|---------|-------|
| Visual Studio 2022 | 17.x | Community edition is free |
| CMake | 3.22+ | https://cmake.org/download/ |
| Git | any | For JUCE auto-download |
| ASIO SDK | 2.3.3+ | Optional but recommended |

## Quick start

```powershell
# 1. Clone / open project
cd C:\Users\adamm\AtomicButton

# 2. Configure
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. Build (Release)
cmake --build build --config Release

# 4. Run
.\build\AtomicButton_artefacts\Release\Shortcut Pro.exe
```

## ASIO (recommended for broadcast)

ASIO gives you sub-10ms latency — essential for live radio/TV.

1. Download the **ASIO SDK 2.3.3** from Steinberg:
   https://www.steinberg.net/asiosdk/
2. Extract to e.g. `C:\SDKs\ASIOSDK2.3.3`
3. Configure with the path:
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64 ^
         -DASIO_SDK_DIR="C:\SDKs\ASIOSDK2.3.3"
   ```
4. In-app: go to **Options > Audio Device Settings** and select your
   ASIO device from the dropdown.

Without ASIO, the app uses WASAPI (Windows) which gives ~10-30ms latency
— perfectly fine for non-live use.

## Audio device settings

At runtime, press **Ctrl+,** (or via the menu) to open the audio device
settings panel. Select:
- Device type: ASIO (if installed) or WASAPI Exclusive
- Output device: your interface
- Sample rate: 48000 Hz (broadcast standard) or 44100 Hz
- Buffer size: 128–256 samples for low latency; 512 for stability

## Supported audio formats

- WAV / AIFF / AIFF-C — always
- FLAC — built in
- OGG Vorbis — built in
- MP3 — disabled by default (licensing); enable with
  `-DCMAKE_CXX_FLAGS="-DJUCE_USE_MP3AUDIOFORMAT=1"` if you have a valid
  Fraunhofer licence or use the system decoder

## Keyboard layout

```
` 1 2 3 4 5 6 7 8 9 0 - =          (13 keys)
  q w e r t y u i o p [ ]          (12 keys)
   a s d f g h j k l ; '           (11 keys)
    z x c v b n m , . /            (10 keys)
              [  space  ]           ( 1 key)
```

47 trigger keys × 4 banks (Ctrl+A/B/C/D) = **188 assignable slots**

## Controls

| Key / Action | Function |
|---|---|
| `1`–`0`, letters, symbols | Fire/toggle clip |
| `Space` | Fire space key slot |
| `Ctrl+A/B/C/D` | Switch bank |
| `Ctrl+L` | Toggle loop-all |
| `Ctrl+.` | Kill all voices |
| `Ctrl+Del` | Clear selected slot |
| `Enter` | Play selected slot |
| Drag audio file onto key | Assign audio |
| Right-click key | Context menu (load, rename, clear, copy) |

## Project structure

```
AtomicButton/
├── CMakeLists.txt
├── BUILD.md
└── Source/
    ├── Main.cpp              — JUCE app entry point
    ├── SoundSlot.h           — Data model (slot, voice, presets)
    ├── AudioEngine.h/.cpp    — ASIO/WASAPI audio, polyphonic playback
    ├── WaveformDisplay.h/.cpp— LCD waveform with trim/playhead
    ├── VUMeterComponent.h/.cpp — 16-segment LED VU with peak hold
    ├── KeyboardComponent.h/.cpp — Full keyboard grid UI + drag-drop
    ├── MainComponent.h/.cpp  — Top-level orchestrator, transport
    └── Main.cpp              — Window + application class
```
