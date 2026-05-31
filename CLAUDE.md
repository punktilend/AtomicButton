# AtomicButton / Shortcut Pro Project Context

## Working Preferences

- Default editor for this project is VS Code, not Cursor.
- Visual Studio 2022 is available and useful for CMake, Windows C++, and debugging, but day-to-day editing should assume VS Code first.
- Recent screenshots for visual review live in `C:\Users\adamm\Pictures\Screenshots\`.

## Project Identity

- This project started as a JUCE/C++ desktop soundboard with a web prototype.
- The direction has changed: this should feel less like a generic pad launcher and more like a dedicated 360 Systems-style broadcast appliance.
- Current design lead:
  - `Instant Replay 2` is now the primary front-panel layout and product-feel reference going forward.
- Secondary influence:
  - `Short/cut / Short/cut 2000` remains important for edit workflow, scrub behavior, waveform editing, and recorder/editor ergonomics.
- In practical terms, aim for:
  - `Instant Replay 2` for overall organization, control grouping, hot-key philosophy, and appliance feel.
  - `Short/cut` for deeper editing behaviors and certain tape-machine/editor metaphors.

## Current App State

- Native app builds on Windows with Visual Studio 2022 + CMake.
- A Release build currently exists at:
  - `build\AtomicButton_artefacts\Release\Shortcut Pro.exe`
- The web prototype is now the **Atomic Button** React design (an Instant Replay 3 / IR3-style deck delivered by Claude Design):
  - The on-screen front-panel brand reads **ATOMIC BUTTON** and the browser title is "Atomic Button — Hot-Key Deck". The source files keep their original `ir3-*` names (IR3 = the design lineage), but the product name on the panel is Atomic Button.
  - Entry point: `index.html` (loads React/Babel from the unpkg CDN, so it needs internet).
  - Design files: `ir3-app.jsx` (state/wiring/rAF loop + responsive fit), `ir3-components.jsx` (presentational), `ir3-audio.js` (Web Audio engine + demo-clip synth), `ir3.css`, and `tweaks-panel.jsx` (live design-tweak panel: chassis green, LCD theme, hardware/software finish, key letters).
  - It is a 50-key / 4-bank IR2/IR3-style green appliance deck — the agreed silhouette north star. Drag audio onto keys, keyboard triggers slots, right-click for options, REC captures the mic.
  - Fit: the chassis is a fixed 1600×900 deck scaled to the viewport. It is centered against the viewport via `position:fixed; left/top:50%; translate(-50%,-50%) scale(...)` in `ir3-app.jsx` (do NOT rely on the `.stage` grid centering — sibling nodes break it and the deck overflows the bottom).
  - Must be served over HTTP (Babel can't fetch `.jsx` over `file://`): `python -m http.server 8765` from the project root, then open `http://127.0.0.1:8765/index.html`. Hard-refresh (Ctrl+Shift+R) after edits — the `.jsx` is browser-cached.
  - Role: this is the **design lab** for locking the look before porting it into the native app — native JUCE remains the primary product and still needs the Atomic Button port.
  - The previous vanilla-JS prototype (old `index.html` + `style.css` + `soundboard.js`) is preserved under `legacy-prototype/`.
- Recent native build fixes already completed:
  - Added generated JUCE header support in `CMakeLists.txt`.
  - Made ASIO optional unless the Steinberg SDK path is configured.
  - Updated code for current JUCE APIs (`FileChooser`, rename dialog flow, FIFO write args, etc.).
  - Fixed current compile issues and rebuilt successfully.
- The project has been de-themed:
  - Removed `Weird Al` branding from the window title, subtitle, metadata, web prototype, docs, and default preset banks.
  - Current visible product naming is closer to `Shortcut Pro — Broadcast Audio Editor`.

## Design Direction Agreed So Far

- The native app should not stretch like a web canvas when full screen.
- It should read as a centered piece of hardware with a believable front panel and fixed-ish physical proportions.
- The keyboard/hot-key area should not dominate the entire identity.
- The top/center of the UI should emphasize editing, transport, display, and appliance-like workflow.
- We want a stronger sense of:
  - chassis
  - panel groupings
  - hardware labeling
  - scrub/edit workflow
  - broadcast utility
- New direction update:
  - Going forward, the layout should mirror `Instant Replay 2` more directly.
  - That means prioritizing the IR2-style front-panel structure over the earlier hybrid/sketch approach.

## Changes Already Made Toward That Goal

- The native JUCE layout has already been adjusted to behave more like a centered hardware deck instead of a fully stretched screen-filling UI.
- The main app window now opens larger and with more realistic resize limits.
- The app now has stronger rack/panel framing cues, status lamps, a scrub-wheel visual, and a voices readout.
- A first hardware-style edit bank has been added with visible/front-panel controls for:
  - `MARK`
  - `ZERO`
  - `GO TO`
  - `FIND`
  - `CUT`
  - `COPY`
  - `INSERT`
  - `ERASE`
  - `UNDO`
  - `ZOOM+`
  - `ZOOM-`
- Some of those edit controls already have slot-level behavior:
  - clipboard copy/paste
  - clear/erase
  - undo last slot edit
  - mark/go-to selected key
  - find by clip name
- The hot-key area has been reworked so it reads more like:
  - dedicated hot keys first
  - supporting keyboard second
- The full-screen behavior is improved, but the redesign is still early and should continue toward a more authentic 360-style control layout.

## Current Layout Assessment

- The app has improved materially, but the current structure still does not fully match the `Instant Replay 2` reference.
- The biggest lesson from recent screenshot comparisons:
  - the problem is now mostly `overall silhouette`, not button styling.
- The target/reference image is characterized by:
  - dominant left-side hot-key field
  - compact upper-right display block
  - tight right-side control tower
  - very clear hierarchy between screen, controls, and hot keys
- The current app still risks feeling too horizontally spread or too panelized if not corrected.
- The current preferred direction is:
  - prominent left hot-key matrix
  - compact but readable display block on the right
  - vertically stacked right-side controls
  - less horizontal sprawl across the middle of the unit

## Key Reference Sources

Primary references:

- `https://www.360systems.com/pdf/Shortcut.pdf`
- `https://360systems.com/pdf/shortcut2000SCSImanual.pdf`
- `https://360systems.com/pdf/irDR554manual.pdf`
- `https://360systems.com/pdf/irDR552manual.pdf`
- `https://360systems.com/pdf/irDR550manual.pdf`
- `https://360systems.com/pdf/irDR600manual.pdf`

Additional review/reference text provided by user:

- "Test Drive: The 360 Systems Short/cut Personal Audio Editor" by Jerry Vigil

## What The References Tell Us

### Instant Replay / Instant Replay 2 cues

- Banked hot-keys are central to the identity.
- Assignment workflows matter:
  - assign hot-key
  - preview
  - find by name/ID
  - overwrite assignment without unnecessary ceremony
- Playlist/follow-on/manual-step behavior is a good source of future features.
- Looping and broadcast playback ergonomics should feel fast and confidence-building.
- The product should feel like a fast playback/routing appliance first, not a generic multimedia app.
- The overall front-panel layout should now follow Instant Replay 2 more closely than earlier Shortcut-led explorations.

### Short/cut / Short/cut 2000 cues

- Self-contained 2-track digital recorder/editor.
- Large backlit LCD waveform display.
- Built-in speakers and headphone monitoring.
- QWERTY keyboard is part of the workflow, not decoration.
- Large weighted scrub wheel is central to the experience.
- Dedicated physical button groups are a core part of the identity.
- One-button record and very fast edit workflow are essential.
- Strong tape-machine metaphors:
  - `ZERO MARK`
  - `EDIT IN`
  - `EDIT OUT`
  - `MARK`
  - `GO TO`
  - `FIND`
  - `LOOP`
  - `BLEEP`

## Features/Ideas Worth Adding Next

High-priority visual/workflow ideas:

1. Rebuild the native layout around real `Instant Replay 2` style control groups.
2. Add an explicit hardware-style edit/control bank:
   - `CUT`
   - `COPY`
   - `INSERT`
   - `ERASE`
   - `UNDO`
   - `MARK`
   - `ZERO`
   - `GO TO`
   - `FIND`
   - `ZOOM`
   - `LOOP`
   - `BLEEP`
3. Make the keyboard area read as integrated `Hot Keys + keyboard`, not the whole product.
4. Add stronger front-panel features:
   - speaker grilles
   - more realistic transport cluster
   - more obvious scrub-wheel prominence
   - better labeled sections
   - waveform marks/locators

## Progress Against The 1-4 Plan

- Step 1: started and partially completed
  - multiple native layout passes completed
  - current work has moved the app closer to an IR2-inspired appliance layout
  - still needs refinement at the silhouette level
- Step 2: completed first pass
  - edit-button bank added
  - several controls already functional at slot level
- Step 3: completed first pass
  - lower section now reads more as `Hot Keys + keyboard`
  - still subject to structural refinement as the overall IR2 layout evolves
- Step 4: not complete
  - some appliance cues exist now
  - speaker/meter/control-tower feel still needs a stronger dedicated pass

## Rear Panel / Routing Direction

- The software should emulate not only the front panel, but also the practical equivalent of the hardware rear panel.
- Treat physical-style connectors as software-routable endpoints.
- This means creating digital equivalents of things like:
  - analog input/output
  - mic/line input behavior
  - AES/EBU-style digital input/output
  - headphone/cue output
  - hot-key or playback buses
  - remote/GPI-style triggers
  - external storage/import/export style workflows
- Architecture direction:
  - front panel = operator workflow and tactile appliance feel
  - rear panel = routing, I/O, integration, and system configuration
- Long-term implementation target:
  - virtual connectors that can map to OS audio devices, virtual audio cables, MIDI, OSC, network control, or future DAW/plugin I/O

Secondary/fun ideas from the manuals:

- Preview mode
- Playlist mode
- Manual-step vs follow-on playback
- Variable zoom behavior
- Visible zero mark / edit mark indicators
- More authentic file/find/naming workflows
- Potential future support for more than 4 banks if the product direction leans toward Instant Replay behavior

## Product Strategy Notes

- Standalone desktop app should remain the primary product identity for now.
- A DAW plugin may be interesting later, but the current design target is an appliance-like broadcast editor/player, not a plugin-first UI.
- If plugin support is explored later, it should likely share the engine, not replace the standalone app's identity.
- The current visual/interaction north star is closer to `Instant Replay 2 as software`, with Short/cut-derived editing power folded in.

## Immediate Next Steps

The current agreed implementation order is:

1. Rework the native layout around the real `Instant Replay 2` control groups.
2. Add a more authentic edit-button bank:
   - `CUT`, `COPY`, `INSERT`, `ERASE`, `UNDO`, `MARK`, `GO TO`, `FIND`, `ZOOM`, `LOOP`
3. Make the keyboard area feel like integrated hot keys instead of a giant floating QWERTY slab.
4. Add visual speaker-grille and meter/level hardware cues so the chassis feels like an appliance.

Additional architecture work to keep in mind while implementing 1-4:

5. Begin thinking in terms of a software rear panel with routable digital equivalents of the hardware I/O and control connectors.

## What To Do Next

- Continue the structural IR2 pivot rather than micro-tuning the old wide-layout skeleton.
- Preserve a `readable` display block:
  - do not shrink the viewer to literal hardware size
  - keep it compact, but large enough for waveform, timer, and clip name readability
- Prefer these hierarchy rules:
  - left = hot-key matrix
  - narrow bridge = meters/monitoring
  - right upper = display
  - right middle/lower = control tower, transport, and mode buttons
- Use the latest screenshots in `C:\Users\adamm\Pictures\Screenshots\` as the truth source for current appearance.

## Notes For Future Sessions

- When reviewing visual changes, check the latest screenshots in `C:\Users\adamm\Pictures\Screenshots\`.
- Prefer grounding design changes in the 360 reference manuals rather than generic retro hardware styling.
- Preserve the user's preference for practical, broadcast-friendly usefulness over flashy UI.
