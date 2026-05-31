/* ============================================================
   INSTANT REPLAY 3 — App (state, wiring, rAF loop, tweaks)
   ============================================================ */
const { useState, useRef, useEffect, useCallback } = React;
const AUDIO = window.IR3Audio;

const BANKS = ["A", "B", "C", "D"];

/* hot-key face labels (slot 0..49) */
const LABELS = [
  "1","2","3","4","5","6","7","8","9","0",
  "Q","W","E","R","T","Y","U","I","O","P",
  "A","S","D","F","G","H","J","K","L",";",
  "Z","X","C","V","B","N","M",",",".","/",
  "~","@","#","SPACE","%","^","&","*","?","BACK",
];
/* keyboard key → slot index */
const TRIG = {};
"1234567890".split("").forEach((k, i) => TRIG[k] = i);
"qwertyuiop".split("").forEach((k, i) => TRIG[k] = 10 + i);
"asdfghjkl;".split("").forEach((k, i) => TRIG[k] = 20 + i);
"zxcvbnm,./".split("").forEach((k, i) => TRIG[k] = 30 + i);
TRIG[" "] = 43;

/* LCD color presets */
const LCD_THEMES = {
  green: { bg:"#02160c", bg2:"#04230f", ink:"#54ff9e", dim:"#1f7a47", grid:"rgba(84,255,158,.12)", glow:"rgba(84,255,158,.35)" },
  amber: { bg:"#1a0e02", bg2:"#241404", ink:"#ffb454", dim:"#7a531f", grid:"rgba(255,180,84,.12)", glow:"rgba(255,180,84,.35)" },
  blue:  { bg:"#02101a", bg2:"#041a26", ink:"#62b6ff", dim:"#1f4f7a", grid:"rgba(98,182,255,.12)", glow:"rgba(98,182,255,.35)" },
};
/* chassis green palettes */
const CHASSIS = {
  Emerald: { hi:"#1f9c63", c1:"#137a4c", c2:"#0d5d3a", c3:"#08412a", edge:"#042016" },
  Forest:  { hi:"#3f7d3a", c1:"#2c5e2a", c2:"#1d451d", c3:"#122e12", edge:"#081a08" },
  Teal:    { hi:"#159c8a", c1:"#0d7568", c2:"#0a594f", c3:"#063b34", edge:"#03201c" },
};

const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "finish": "hardware",
  "lcdTheme": "blue",
  "swoosh": "#2ec27e",
  "chassis": "Teal",
  "showLetters": true
}/*EDITMODE-END*/;

function emptyDecks() {
  const d = {}; BANKS.forEach(b => d[b] = new Array(50).fill(null)); return d;
}

function App() {
  const [t, setTweak] = useTweaks(TWEAK_DEFAULTS);
  const [decks, setDecks] = useState(emptyDecks);
  const [bank, setBank] = useState("A");
  const [selected, setSelected] = useState(0);
  const [recSlot, setRecSlot] = useState(null);
  const [loopOn, setLoopOn] = useState(false);
  const [followOn, setFollowOn] = useState(false);
  const [playing, setPlaying] = useState(() => new Set());
  const [menu, setMenu] = useState(null);   // {slot,x,y}
  const [status, setStatus] = useState("READY — drop audio onto keys · right-click a key for options · keyboard triggers slots");
  const [inLvl, setInLvl] = useState(0.7);
  const [phones, setPhones] = useState(0.85);
  const [scale, setScale] = useState(1);

  const theme = LCD_THEMES[t.lcdTheme] || LCD_THEMES.green;
  const slots = decks[bank];
  const clip = slots[selected];

  /* refs mirroring state for the rAF loop */
  const ref = useRef({});
  ref.current = { decks, bank, selected, playing, followOn };

  /* ---------- seed demo clips once ---------- */
  useEffect(() => {
    try {
      const demos = AUDIO.genDemos();
      const spots = [0, 1, 3, 5, 11, 12, 22, 30];
      setDecks(d => {
        const A = d.A.slice();
        demos.forEach((c, i) => { if (spots[i] != null) A[spots[i]] = c; });
        return { ...d, A };
      });
    } catch (e) { console.warn("demo gen failed", e); }
  }, []);

  /* ---------- engine change → playing set + follow-on advance ---------- */
  useEffect(() => {
    AUDIO.onChange(() => {
      const set = new Set(AUDIO.voices.map(v => v.slot));
      setPlaying(set);
      // follow-on auto advance when everything has stopped
      const { followOn: fo, decks: dk, bank: bk, selected: sel } = ref.current;
      if (fo && AUDIO.playingCount() === 0) {
        const nxt = nextLoaded(dk[bk], sel);
        if (nxt != null && nxt !== sel) {
          setSelected(nxt);
          const c = dk[bk][nxt];
          if (c) { AUDIO.play(c, nxt, { loop: false }); }
        }
      }
    });
  }, []);

  /* ---------- 60fps loop: meters, playhead, timecode, clock ---------- */
  const peakHold = useRef({ l: 0, r: 0 });
  useEffect(() => {
    let raf;
    const tick = () => {
      // clock
      const now = new Date();
      let h = now.getHours(); const ap = h >= 12 ? "pm" : "am"; h = h % 12 || 12;
      const cl = document.getElementById("ir3-clock");
      if (cl) cl.textContent = h + ":" + String(now.getMinutes()).padStart(2, "0") + ":" + String(now.getSeconds()).padStart(2, "0") + ap;

      // meters
      const lv = AUDIO.levels();
      const ph = peakHold.current;
      ph.l = Math.max(lv.l, ph.l - 0.012); ph.r = Math.max(lv.r, ph.r - 0.012);
      setMeter("l", lv.l, ph.l); setMeter("r", lv.r, ph.r);

      // key progress
      const set = ref.current.playing;
      document.querySelectorAll(".key.playing .prog").forEach(el => {
        const s = +el.parentElement.dataset.slot;
        const info = AUDIO.slotElapsed(s);
        if (info) el.style.width = (info.elapsed / info.duration * 100) + "%";
      });

      // LCD timecode + playhead (selected slot)
      const sel = ref.current.selected;
      const info = AUDIO.slotElapsed(sel);
      const el = document.getElementById("lcd-elapsed");
      const rem = document.getElementById("lcd-remain");
      const head = document.getElementById("lcd-playhead");
      const selClip = ref.current.decks[ref.current.bank][sel];
      if (info) {
        if (el) el.textContent = fmt(info.elapsed);
        if (rem) rem.textContent = "-" + fmt(info.duration - info.elapsed);
        if (head) { head.style.opacity = 1; head.style.left = (info.elapsed / info.duration * 100) + "%"; }
      } else {
        if (el) el.textContent = "0:00.0";
        if (rem) rem.textContent = selClip ? "-" + fmt(selClip.duration) : "-0:00.0";
        if (head) head.style.opacity = 0;
      }
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, []);

  function setMeter(ch, v, peak) {
    const f = document.getElementById("meter-" + ch + "-fill");
    const p = document.getElementById("meter-" + ch + "-peak");
    if (f) f.style.height = (v * 100) + "%";
    if (p) p.style.bottom = (Math.min(1, peak) * 100) + "%";
  }

  /* ---------- responsive scaling ---------- */
  useEffect(() => {
    const fit = () => setScale(Math.min(window.innerWidth / 1600, window.innerHeight / 900) * 0.94);
    fit(); window.addEventListener("resize", fit); return () => window.removeEventListener("resize", fit);
  }, []);

  /* ---------- helpers ---------- */
  function nextLoaded(arr, from) {
    for (let i = from + 1; i < 50; i++) if (arr[i]) return i;
    return null;
  }
  const setSlot = (b, i, c) => setDecks(d => { const a = d[b].slice(); a[i] = c; return { ...d, [b]: a }; });

  const triggerSlot = useCallback((i) => {
    AUDIO.ensure();
    setSelected(i);
    const c = ref.current.decks[ref.current.bank][i];
    if (c) { AUDIO.play(c, i, { loop: ref.current.loopOnNow }); setStatus("PLAY  ▸  " + c.name); }
    else setStatus("SLOT " + (i + 1) + " empty — drop audio or press REC to record");
  }, []);
  // keep loop flag fresh inside callback
  ref.current.loopOnNow = loopOn;

  const loadFileToSlot = async (i, file) => {
    try {
      setStatus("LOADING  " + file.name + " …");
      const c = await AUDIO.decodeFile(file);
      setSlot(ref.current.bank, i, c);
      setSelected(i);
      setStatus("LOADED  ▸  " + c.name);
    } catch (e) { setStatus("✕ could not decode " + file.name); }
  };

  /* hidden file input for ASSIGN / menu load */
  const fileInput = useRef(null);
  const assignTarget = useRef(null);
  const pickFile = (i) => { assignTarget.current = i; fileInput.current.click(); };
  const onFilePicked = (e) => {
    const f = e.target.files[0]; if (f) loadFileToSlot(assignTarget.current, f);
    e.target.value = "";
  };

  /* ---------- keyboard ---------- */
  useEffect(() => {
    const onKey = (e) => {
      if (e.target.closest && e.target.closest("input,textarea,.tweaks-panel")) return;
      if (e.ctrlKey || e.metaKey) {
        if (e.key >= "1" && e.key <= "4") { e.preventDefault(); setBank(BANKS[+e.key - 1]); }
        return;
      }
      const k = e.key.length === 1 ? e.key.toLowerCase() : e.key;
      if (k in TRIG) { e.preventDefault(); triggerSlot(TRIG[k]); }
      else if (e.key === "Enter") triggerSlot(ref.current.selected);
      else if (e.key === "Escape") AUDIO.stopAll();
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [triggerSlot]);

  /* ---------- transport ---------- */
  const onTransport = async (a) => {
    AUDIO.ensure();
    if (a === "play") { const c = slots[selected]; if (c) { AUDIO.play(c, selected, { loop: loopOn }); setStatus("PLAY ▸ " + c.name); } }
    else if (a === "stop") { AUDIO.stopAll(); setStatus("STOP — all clips"); }
    else if (a === "rew") { AUDIO.stopSlot(selected); const c = slots[selected]; if (c) AUDIO.play(c, selected, { loop: loopOn }); }
    else if (a === "ff") { const n = nextLoaded(slots, selected); if (n != null) triggerSlot(n); }
    else if (a === "rec") {
      if (recSlot != null) {
        setStatus("RECORDING stopped — decoding…");
        const name = "REC " + bank + (selected + 1);
        const c = await AUDIO.stopRecording(name).catch(() => null);
        if (c) { setSlot(bank, recSlot, c); setStatus("RECORDED ▸ " + c.name); }
        setRecSlot(null);
      } else {
        try { await AUDIO.startRecording(); setRecSlot(selected); setStatus("● RECORDING into slot " + (selected + 1) + " — press REC to stop"); }
        catch (e) { setStatus("✕ microphone unavailable"); }
      }
    }
  };

  /* ---------- context menu ---------- */
  const openMenu = (slot, e) => { setSelected(slot); setMenu({ slot, x: e.clientX, y: e.clientY }); };
  const closeMenu = () => setMenu(null);
  useEffect(() => {
    if (!menu) return;
    const h = () => closeMenu();
    window.addEventListener("click", h); window.addEventListener("scroll", h, true);
    return () => { window.removeEventListener("click", h); window.removeEventListener("scroll", h, true); };
  }, [menu]);

  const menuAction = (act) => {
    const s = menu.slot; closeMenu();
    if (act === "play") triggerSlot(s);
    else if (act === "stop") AUDIO.stopSlot(s);
    else if (act === "load") pickFile(s);
    else if (act === "clear") { AUDIO.stopSlot(s); setSlot(bank, s, null); setStatus("CLEARED slot " + (s + 1)); }
    else if (act === "recolor") { const c = slots[s]; if (c) setSlot(bank, s, { ...c, tag: AUDIO.nextTag() }); }
  };

  /* ---------- counts ---------- */
  const counts = {}; BANKS.forEach(b => counts[b] = decks[b].filter(Boolean).length);
  const cued = followOn ? nextLoaded(slots, selected) : null;
  const cuedClip = cued != null ? slots[cued] : null;

  /* ---------- device style vars ---------- */
  const ch = CHASSIS[t.chassis] || CHASSIS.Emerald;
  const deviceStyle = {
    "--swoosh": t.swoosh,
    "--swoosh-2": "color-mix(in srgb, " + t.swoosh + " 60%, #000)",
    "--lcd-bg": theme.bg, "--lcd-bg2": theme.bg2, "--lcd-ink": theme.ink,
    "--lcd-dim": theme.dim, "--lcd-grid": theme.grid, "--lcd-glow": theme.glow,
    "--chassis-hi": ch.hi, "--chassis-1": ch.c1, "--chassis-2": ch.c2, "--chassis-3": ch.c3, "--chassis-edge": ch.edge,
  };

  return (
    React.createElement("div", { className: "stage" },
      React.createElement("input", { ref: fileInput, type: "file", accept: "audio/*", style: { display: "none" }, onChange: onFilePicked }),
      React.createElement("div", { className: "device-wrap", style: { position: "fixed", left: "50%", top: "50%", transform: "translate(-50%,-50%) scale(" + scale + ")" } },
        React.createElement("div", { className: "device finish-" + t.finish, style: deviceStyle },
          React.createElement("div", { className: "screw tl" }), React.createElement("div", { className: "screw tr" }),
          React.createElement("div", { className: "screw bl" }), React.createElement("div", { className: "screw br" }),

          /* TOP DECK */
          React.createElement("div", { className: "deck-top" },
            React.createElement(Swoosh, null),
            React.createElement("div", { className: "brand" },
              React.createElement("span", { className: "maker" }, "360° BROADCAST"),
              React.createElement("span", { className: "model" }, "ATOMIC ",
                React.createElement("span", { className: "three" }, "BUTTON"))
            ),
            React.createElement("div", { className: "deck-status" },
              React.createElement("div", { className: "clk", id: "ir3-clock" }, "—:—:—"),
              React.createElement("div", { className: "sub" }, "50 HOT-KEYS · 4 BANKS")
            )
          ),

          /* BODY */
          React.createElement("div", { className: "body" },
            React.createElement(Deck, {
              banks: BANKS, active: bank, counts, slots, keyChars: t.showLetters ? LABELS : LABELS.map((_, i) => ""),
              selected, cued, recSlot, playingSet: playing,
              onSelect: setBank, onKey: triggerSlot, onMenu: openMenu, onDropFile: loadFileToSlot,
              followOn, onFollow: () => setFollowOn(v => !v), onPause: () => AUDIO.pauseAll(),
            }),
            React.createElement(Console, {
              bank, slot: selected, clip, theme, loopOn, recOn: recSlot != null, cuedName: cuedClip ? cuedClip.name : null,
              playCount: playing.size,
              onSoft: (i) => setStatus("SOFT KEY F" + (i + 1)),
              onEnter: () => triggerSlot(selected),
              onPower: () => AUDIO.stopAll(),
              onNav: (d) => onNav(d),
              onLoop: () => setLoopOn(v => !v),
              onPreview: () => { const c = slots[selected]; if (c) AUDIO.play(c, selected, { loop: false }); setStatus("PREVIEW"); },
              onTransport,
              onMenu: () => setStatus("MENU"), onBankSel: () => setBank(BANKS[(BANKS.indexOf(bank) + 1) % 4]),
              onAssign: () => pickFile(selected), onFind: () => setStatus("FIND"),
              onHotList: () => setStatus("HOT LIST — " + counts[bank] + " clips in bank " + bank),
              onCancel: () => { AUDIO.stopAll(); setStatus("CANCEL"); },
            }),
            React.createElement(Rail, {
              inL: inLvl, inR: inLvl, phones,
              onIn: setInLvl, onPhones: (v) => { setPhones(v); AUDIO.setPhones(v); },
            })
          ),

          /* STATUS BAR */
          React.createElement("div", { className: "statusbar" },
            React.createElement("span", null, status),
            React.createElement("span", { className: "play-ct" }, "BANK ", bank, "  ·  ", React.createElement("b", null, playing.size), " PLAYING")
          )
        )
      ),

      /* context menu */
      menu && React.createElement(KeyMenu, { menu, clip: slots[menu.slot], onAction: menuAction }),

      /* TWEAKS */
      React.createElement(TweaksPanel, null,
        React.createElement(TweakSection, { label: "Finish" }),
        React.createElement(TweakRadio, { label: "Look", value: t.finish, options: ["hardware", "software"], onChange: (v) => setTweak("finish", v) }),
        React.createElement(TweakSection, { label: "Chassis" }),
        React.createElement(TweakRadio, { label: "Green", value: t.chassis, options: ["Emerald", "Forest", "Teal"], onChange: (v) => setTweak("chassis", v) }),
        React.createElement(TweakColor, { label: "Swoosh accent", value: t.swoosh,
          options: ["#e23b2e", "#ffa033", "#2ec27e", "#19b6c9", "#e0408a", "#eef2f4"], onChange: (v) => setTweak("swoosh", v) }),
        React.createElement(TweakSection, { label: "Display" }),
        React.createElement(TweakRadio, { label: "LCD theme", value: t.lcdTheme, options: ["green", "amber", "blue"], onChange: (v) => setTweak("lcdTheme", v) }),
        React.createElement(TweakToggle, { label: "Key letters", value: t.showLetters, onChange: (v) => setTweak("showLetters", v) }),
      )
    )
  );

  function onNav(d) {
    if (d === "del") { AUDIO.stopSlot(selected); setSlot(bank, selected, null); setStatus("DEL slot " + (selected + 1)); return; }
    let s = selected;
    if (d === "left") s = Math.max(0, s - 1);
    else if (d === "right") s = Math.min(49, s + 1);
    else if (d === "up") s = Math.max(0, s - 10);
    else if (d === "down") s = Math.min(49, s + 10);
    else if (d === "scanback") s = Math.max(0, s - 1);
    else if (d === "scanfwd") s = Math.min(49, s + 1);
    setSelected(s);
  }
}

/* swoosh graphic */
function Swoosh() {
  return React.createElement("svg", { className: "swoosh", viewBox: "0 0 1000 46", preserveAspectRatio: "none" },
    React.createElement("path", { d: "M0,30 C220,2 380,40 560,22 C720,6 860,30 1000,14 L1000,46 L0,46 Z", fill: "var(--swoosh)", opacity: ".92" }),
    React.createElement("path", { d: "M0,38 C220,12 380,48 560,30 C720,14 860,38 1000,22", fill: "none", stroke: "#2b6db0", strokeWidth: "3", opacity: ".8" })
  );
}

/* context menu */
function KeyMenu({ menu, clip, onAction }) {
  const items = clip
    ? [["play", "Play"], ["stop", "Stop"], ["recolor", "Change colour tag"], ["load", "Replace audio…"], ["clear", "Clear slot"]]
    : [["load", "Load audio…"]];
  return React.createElement("div", {
    className: "keymenu",
    style: { position: "fixed", left: Math.min(menu.x, window.innerWidth - 200), top: Math.min(menu.y, window.innerHeight - 220), zIndex: 9999 },
    onClick: (e) => e.stopPropagation(),
  },
    React.createElement("div", { className: "keymenu-h" }, clip ? clip.name : "SLOT " + (menu.slot + 1)),
    items.map(([a, label]) => React.createElement("button", { key: a, onClick: () => onAction(a) }, label))
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(React.createElement(App));
