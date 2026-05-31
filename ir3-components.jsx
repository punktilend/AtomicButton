/* ============================================================
   IR3 COMPONENTS  (presentational) — exported to window
   Dynamic 60fps bits (meters, playhead, key progress) are
   driven imperatively by the App rAF loop via DOM refs.
   ============================================================ */
const { useRef, useEffect } = React;

/* ---- waveform thumbnail on a hot key ---- */
function WaveThumb({ peaks, color }) {
  const ref = useRef(null);
  useEffect(() => {
    const c = ref.current; if (!c || !peaks) return;
    const W = 120, H = 44; c.width = W; c.height = H;
    const ctx = c.getContext("2d"); ctx.clearRect(0, 0, W, H);
    const n = peaks.length, bw = W / n, mid = H / 2;
    ctx.fillStyle = color || "#9aa";
    for (let i = 0; i < n; i++) {
      const h = Math.max(1.5, peaks[i] * (H * 0.46));
      ctx.globalAlpha = 0.35 + peaks[i] * 0.5;
      ctx.fillRect(i * bw, mid - h, Math.max(0.7, bw - 0.6), h * 2);
    }
    ctx.globalAlpha = 1;
  }, [peaks, color]);
  return React.createElement("canvas", { ref, className: "wav" });
}

/* ---- one hot key ---- */
function HotKey({ slot, keyChar, clip, selected, cued, recording, playing, onClick, onMenu, onDropFile }) {
  const cls = ["key"];
  if (recording) cls.push("rec");
  else if (playing) cls.push("playing");
  else if (clip) cls.push("loaded");
  else cls.push("empty");
  if (selected) cls.push("selected");
  if (cued) cls.push("cued");

  const onDragOver = (e) => { e.preventDefault(); e.currentTarget.classList.add("dragover"); };
  const onDragLeave = (e) => e.currentTarget.classList.remove("dragover");
  const onDrop = (e) => {
    e.preventDefault(); e.currentTarget.classList.remove("dragover");
    const f = e.dataTransfer.files && e.dataTransfer.files[0];
    if (f) onDropFile(slot, f);
  };

  return React.createElement("div", {
    className: cls.join(" "),
    "data-slot": slot,
    style: clip ? { ["--tag"]: clip.tag } : undefined,
    onClick: () => onClick(slot),
    onContextMenu: (e) => { e.preventDefault(); onMenu(slot, e); },
    onDragOver, onDragLeave, onDrop,
  },
    clip && React.createElement("div", { className: "tag" }),
    clip && React.createElement(WaveThumb, { peaks: clip.peaks, color: clip.tag }),
    clip && React.createElement("div", { className: "name" }, clip.name),
    playing && React.createElement("div", { className: "prog" }),
    cued && React.createElement("div", { className: "nx" }, "NEXT"),
    React.createElement("div", { className: "kc" }, keyChar),
    React.createElement("div", { className: "slot" }, slot + 1),
    React.createElement("div", { className: "led" }),
  );
}

/* ---- bank tabs ---- */
function BankBar({ banks, active, counts, onSelect }) {
  return React.createElement("div", { className: "bankbar" },
    React.createElement("span", { className: "lbl" }, "BANK"),
    banks.map((b) =>
      React.createElement("button", {
        key: b, className: "banktab" + (b === active ? " active" : ""),
        onClick: () => onSelect(b),
      },
        "BANK " + b,
        React.createElement("span", { className: "ct" }, counts[b] + " LOADED")
      )
    )
  );
}

/* ---- the hot-key deck ---- */
function Deck({ banks, active, counts, slots, keyChars, selected, cued, recSlot, playingSet,
                onSelect, onKey, onMenu, onDropFile, followOn, onFollow, onPause }) {
  return React.createElement("div", { className: "deck" },
    React.createElement(BankBar, { banks, active, counts, onSelect }),
    React.createElement("div", { className: "keygrid" },
      slots.map((clip, i) =>
        React.createElement(HotKey, {
          key: i, slot: i, keyChar: keyChars[i], clip,
          selected: selected === i, cued: cued === i, recording: recSlot === i,
          playing: playingSet.has(i),
          onClick: onKey, onMenu, onDropFile,
        })
      )
    ),
    React.createElement("div", { className: "deck-bottom" },
      React.createElement("div", { className: "hk-label" }, "HOT KEYS"),
      React.createElement("button", { className: "bigbtn" + (followOn ? " on" : ""), onClick: onFollow },
        "FOLLOW ON", React.createElement("small", null, "auto-advance")),
      React.createElement("button", { className: "bigbtn", onClick: onPause },
        "PAUSE", React.createElement("small", null, "all decks")),
    )
  );
}

/* ---- LCD waveform (static; playhead overlaid by App) ---- */
function LcdWave({ clip, theme }) {
  const ref = useRef(null);
  useEffect(() => {
    const c = ref.current; if (!c) return;
    const W = 520, H = 92; c.width = W; c.height = H;
    const ctx = c.getContext("2d"); ctx.clearRect(0, 0, W, H);
    // grid
    ctx.strokeStyle = theme.grid; ctx.lineWidth = 1;
    for (let x = 0; x <= W; x += W / 8) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, H); ctx.stroke(); }
    ctx.beginPath(); ctx.moveTo(0, H / 2); ctx.lineTo(W, H / 2); ctx.stroke();
    if (!clip) {
      ctx.fillStyle = theme.dim; ctx.font = "13px 'JetBrains Mono', monospace";
      ctx.textAlign = "center"; ctx.fillText("-- NO CLIP -- DROP AUDIO ONTO A KEY --", W / 2, H / 2 + 4);
      return;
    }
    const p = clip.peaks, n = p.length, bw = W / n, mid = H / 2;
    ctx.fillStyle = theme.ink;
    for (let i = 0; i < n; i++) {
      const h = Math.max(1, p[i] * (H * 0.46));
      ctx.globalAlpha = 0.5 + p[i] * 0.5;
      ctx.fillRect(i * bw, mid - h, Math.max(0.8, bw - 0.7), h * 2);
    }
    ctx.globalAlpha = 1;
  }, [clip, theme]);
  return React.createElement("canvas", { ref });
}

/* ---- LCD panel ---- */
function Lcd({ bank, slot, clip, theme, loopOn, recOn, cuedName }) {
  return React.createElement("div", { className: "lcd" },
    React.createElement("div", { className: "scan" }),
    React.createElement("div", { className: "lcd-row1" },
      React.createElement("span", null, "BANK ", React.createElement("b", null, bank), "  SLOT ", React.createElement("b", null, slot + 1)),
      React.createElement("span", null, cuedName ? "NEXT ▸ " + cuedName : (recOn ? "● REC ARMED" : "READY"))
    ),
    React.createElement("div", { className: "lcd-clip" + (clip ? "" : " empty") }, clip ? clip.name : "-- NO CLIP --"),
    React.createElement("div", { className: "lcd-time" },
      React.createElement("span", { id: "lcd-elapsed" }, "0:00.0"),
      React.createElement("span", { className: "rem", id: "lcd-remain" }, clip ? "-" + fmt(clip.duration) : "-0:00.0")
    ),
    React.createElement("div", { className: "lcd-wav" },
      React.createElement(LcdWave, { clip, theme }),
      React.createElement("div", { id: "lcd-playhead", style: {
        position: "absolute", top: 0, bottom: 0, width: "2px", left: "0%",
        background: theme.ink, boxShadow: "0 0 8px " + theme.ink, opacity: 0 } }),
      React.createElement("div", { className: "lcd-flags" },
        React.createElement("span", { className: "lcd-flag" + (loopOn ? " on" : "") }, "LOOP"),
        React.createElement("span", { className: "lcd-flag" + (recOn ? " on" : "") }, "REC")
      )
    )
  );
}

function fmt(s) {
  if (s == null) return "0:00.0";
  const m = Math.floor(s / 60), sec = (s % 60);
  return m + ":" + (sec < 10 ? "0" : "") + sec.toFixed(1);
}

/* ---- generic pad button ---- */
function Pad({ label, sub, cls, on, col, onClick }) {
  return React.createElement("button", {
      className: "kbtn" + (cls ? " " + cls : "") + (on ? " on" : ""),
      style: col ? { gridColumn: col } : undefined, onClick },
    React.createElement("span", null, label),
    sub && React.createElement("small", null, sub)
  );
}

/* ---- console: soft keys, enter, keypad, transport ---- */
function Console({ bank, slot, clip, theme, loopOn, recOn, cuedName, playCount,
                   onSoft, onEnter, onPower, onNav, onLoop, onPreview, onTransport,
                   onMenu, onBankSel, onAssign, onFind, onHotList, onCancel }) {
  const soft = ["F1", "F2", "F3", "F4", "F5"];
  return React.createElement("div", { className: "console" },
    React.createElement(Lcd, { bank, slot, clip, theme, loopOn, recOn, cuedName }),
    React.createElement("div", { className: "softrow" },
      soft.map((f, i) => React.createElement("button", { key: f, className: "kbtn soft", onClick: () => onSoft(i) }, f))
    ),
    React.createElement("div", { className: "enterrow" },
      React.createElement("button", { className: "kbtn enter", onClick: onEnter }, "ENTER"),
      React.createElement("div", { className: "power" },
        React.createElement("button", { className: "pwr", onClick: onPower, title: "Standby" },
          React.createElement("span", { className: "dot" })))
    ),
    React.createElement("div", { className: "keypad" },
      React.createElement(Pad, { label: "◀◀", sub: "scan", cls: "arrow", onClick: () => onNav("scanback") }),
      React.createElement(Pad, { label: "DEL", sub: "clear", onClick: () => onNav("del") }),
      React.createElement(Pad, { label: "▶▶", sub: "scan", cls: "arrow", onClick: () => onNav("scanfwd") }),
      React.createElement(Pad, { label: "CANCEL", onClick: onCancel }),
      React.createElement(Pad, { label: "MENU", onClick: onMenu }),
      React.createElement(Pad, { label: "BANK", sub: "SEL", onClick: onBankSel }),

      React.createElement(Pad, { label: "◀", cls: "arrow", onClick: () => onNav("left") }),
      React.createElement(Pad, { label: "▲", cls: "arrow", onClick: () => onNav("up") }),
      React.createElement(Pad, { label: "▶", cls: "arrow", onClick: () => onNav("right") }),
      React.createElement(Pad, { label: "FIND", onClick: onFind }),
      React.createElement(Pad, { label: "ASSIGN", sub: "HOT KEY", onClick: onAssign }),
      React.createElement(Pad, { label: "HOT", sub: "LIST", onClick: onHotList }),

      React.createElement(Pad, { label: "▼", cls: "arrow", col: "2", onClick: () => onNav("down") }),
      React.createElement(Pad, { label: "LOOP", on: loopOn, col: "4", onClick: onLoop }),
      React.createElement(Pad, { label: "PREVIEW", col: "5", onClick: onPreview }),
    ),
    React.createElement("div", { className: "transport" },
      React.createElement("div", { className: "lab" }, "TRANSPORT"),
      React.createElement("button", { className: "tbtn", onClick: () => onTransport("stop") },
        React.createElement("span", { className: "ic" }, "■"), "STOP"),
      React.createElement("button", { className: "tbtn play" + (playCount ? " on" : ""), onClick: () => onTransport("play") },
        React.createElement("span", { className: "ic" }, "▶"), "PLAY"),
      React.createElement("button", { className: "tbtn rec" + (recOn ? " on" : ""), onClick: () => onTransport("rec") },
        React.createElement("span", { className: "ic" }, "●"), "REC"),
      React.createElement("button", { className: "tbtn", onClick: () => onTransport("rew") },
        React.createElement("span", { className: "ic" }, "◀◀"), "REW"),
      React.createElement("button", { className: "tbtn", onClick: () => onTransport("ff") },
        React.createElement("span", { className: "ic" }, "▶▶"), "FF"),
    )
  );
}

/* ---- knob ---- */
function Knob({ label, sub, value, onChange }) {
  const start = useRef(null);
  const down = (e) => {
    e.preventDefault();
    start.current = { y: e.clientY, v: value };
    const move = (ev) => {
      const dy = start.current.y - ev.clientY;
      onChange(Math.max(0, Math.min(1, start.current.v + dy / 160)));
    };
    const up = () => { window.removeEventListener("mousemove", move); window.removeEventListener("mouseup", up); };
    window.addEventListener("mousemove", move); window.addEventListener("mouseup", up);
  };
  const rot = -135 + value * 270;
  const pos = (value - 0.5) * 3;                       // map 0..1 → -1.5..+1.5
  const posStr = (pos >= 0 ? "+" : "") + pos.toFixed(1);
  return React.createElement("div", { className: "knob-wrap" },
    React.createElement("div", { className: "knob-dial" },
      React.createElement("span", { className: "ktick kmin" }, "-1.5"),
      React.createElement("span", { className: "ktick kmid" }, "0"),
      React.createElement("span", { className: "ktick kmax" }, "+1.5"),
      React.createElement("div", { className: "knob", style: { ["--rot"]: rot + "deg" }, onMouseDown: down })
    ),
    React.createElement("div", { className: "kval" }, posStr),
    React.createElement("div", { className: "cap" }, label),
    sub && React.createElement("div", { className: "cap", style: { opacity: .6 } }, sub)
  );
}

/* ---- right rail: meters + knobs ---- */
function Rail({ inL, inR, phones, onIn, onPhones }) {
  return React.createElement("div", { className: "rail" },
    React.createElement("div", { className: "cap" }, "PEAK LEVEL"),
    React.createElement("div", { className: "meters" },
      ["l", "r"].map((ch) =>
        React.createElement("div", { key: ch, className: "meter" },
          React.createElement("div", { className: "fill", id: "meter-" + ch + "-fill" }),
          React.createElement("div", { className: "peak", id: "meter-" + ch + "-peak" }),
          React.createElement("div", { className: "mask" })
        ))
    ),
    React.createElement("div", { className: "mlbl" }, React.createElement("span", null, "L"), React.createElement("span", null, "R")),
    React.createElement(Knob, { label: "INPUT", sub: "L / R", value: inL, onChange: onIn }),
    React.createElement(Knob, { label: "PHONES", value: phones, onChange: onPhones }),
  );
}

Object.assign(window, { WaveThumb, HotKey, BankBar, Deck, Lcd, LcdWave, Console, Knob, Rail, Pad, fmt });
