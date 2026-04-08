'use strict';
// ============================================================
//  SHORTCUT PRO — Weird Al Universe Edition
//  Full-keyboard broadcast cart machine.
//  47 trigger keys × 4 banks = 188 assignable slots.
//  Modeled on the 360 Systems Shortcut workflow.
// ============================================================

// ── KEYBOARD LAYOUT ──────────────────────────────────────
// Key IDs match e.key (lowercased where needed)
const KB_ROWS = [
  { id: 'num', keys: ['`','1','2','3','4','5','6','7','8','9','0','-','='] },
  { id: 'q',   keys: ['q','w','e','r','t','y','u','i','o','p','[',']'] },
  { id: 'a',   keys: ['a','s','d','f','g','h','j','k','l',';',"'"] },
  { id: 'z',   keys: ['z','x','c','v','b','n','m',',','.','/'] },
  { id: 'sp',  keys: [' '] },
];

// Display labels for keys (what shows on the button face)
const KEY_DISPLAY = {
  '`':'`', '-':'-', '=':'=', '[':'[', ']':']',
  ';':';', "'":`'`, ',':',', '.':'.', '/':'/',
  ' ':'SPC',
};
function keyDisplay(k) { return KEY_DISPLAY[k] || k.toUpperCase(); }

// ── PRESET NAMES (per bank, per key) ─────────────────────
// Bank A: UHF film / VHS content
// Bank B: Classic song hooks
// Bank C: Deep cuts / Al TV / interview
// Bank D: Specialty / rarities
const PRESETS = {
  A: {
    '`':'UHF STING',   '1':'NO STINK BADGR','2':'SUPPLIES!',   '3':'SPATULA CITY', '4':'CONAN LIBRARYN',
    '5':'WHEEL O FISH', '6':'GANDHI II',     '7':'SPADOWSKI',   '8':'BEVERAGE CART','9':'YOU SO STUPID',
    '0':'UHF THEME',   '-':'RAUL INTRO',    '=':'TOWN TALK',
    'q':'QUICK CHANGE', 'w':'WILD KINGDOM',  'e':'EVEL K JR',   'r':"RAUL'S SHOW",  't':'TOOTHPICK TRK',
    'y':'YODA IMPRSN',  'u':'UNCLE NUTSY',   'i':'I LUV PLACE', 'o':'ONE DOLLAR!',  'p':"PAULY'S DEBUT",
    '[':'POOL TRICK',  ']':'PUPPET SHOW',
    'a':"AL'S SPEECH",  's':'SPATULA PITCH', 'd':'DOROTHY SHOES','f':'FLYING GANDHI','g':'GANDHI TRLR',
    'h':'HOTEL LOBBY',  'j':'JANITOR PALCE', 'k':'KIWI SLAM',   'l':'LARRY LAUNDRY',';':'SQUEAK FX',
    "'":'BADGER QUOTE',
    'z':'ZERO DOLLARS', 'x':'XMAS VHS',      'c':'COLONIC IRGTN','v':'VHS TRACKING', 'b':'BEV SMASH',
    'n':'NOOGIE NUTSY', 'm':'MY SPLEEN!',    ',':'SLIGHT RETURN','.':'THATS ALL',    '/':'FADE OUT',
    ' ':'BIG FIRE',
  },
  B: {
    '`':'ACCORDION RFF', '1':'EAT IT HOOK',  '2':'FAT HOOK',    '3':'AMISH PARADSE','4':'LIKE A SURGN',
    '5':'WHITE&NERDY',  '6':'SMELLS NIRVNA', '7':'JURASSIC PRK','8':'BEDROCK ANTM', '9':'ALBUQUERQUE',
    '0':'HARDWARE STOR','-':'POLKA PARTY',   '=':'DARE STUPID',
    'q':'QUEEN SUEDE',  'w':'WORD CRIMES',   'e':'EAT IT VERSE', 'r':'RUN W SCISSRS','t':'TACKY HOOK',
    'y':'YODA SONG',    'u':'UGLY GIRL',     'i':'ILL SUE YA',  'o':'ODE FAMILY',   'p':'PRFRM THIS WY',
    '[':'POLKA FACE',  ']':'PRINCESS BRIDE',
    'a':'ANOTHER RIDES','s':'SAGA BEGINS',   'd':'DONT DOWNLOAD','f':'FIRST WLD PRBS','g':'GENERIC BLUES',
    'h':'HEADLINE NEWS','j':'JACKSON PARK',  'k':'KING OF SUEDE','l':'LASAGNA',      ';':'SRGN FX',
    "'":'AL QUOTE',
    'z':'ZERO HOUR',    'x':'X AMISH',       'c':'CNR HOOK',    'v':'VIRUS ALERT',  'b':'BOHEMIAN LIKE',
    'n':'NATURE TRAIL', 'm':'MY BOLOGNA',    ',':'COMMA KARMA',  '.':'POINT BREAK',  '/':'JUST EAT IT',
    ' ':'ACCORDION STAB',
  },
  C: {
    '`':'RADIO CALL IN', '1':'AL TV MJ',     '2':'AL TV MADONNA','3':'AL TV DIRE ST','4':'AL TV COOLIO',
    '5':'AL TV HAMMER',  '6':'AL TV NIRVANA','7':'AL TV LYNYRD', '8':'AL TV AEROSMTH','9':'AL TV VAN HLN',
    '0':'AL TV MEATLOAF','-':'BEHIND SCENES','=':'VLOG INTRO',
    'q':'QUESTION AL',  'w':'WENDY INTRVIEW','e':'EARLY KIDSHOW', 'r':'RUNNING GAGS', 't':'TOUR DIARY',
    'y':'YEARBOOK PIC',  'u':'UNDER PRESSURE','i':'IN 3D SHOW',   'o':'OPEN CHEESE',  'p':'POODLE BONUS',
    '[':'POLKA FACE BNS',']':'POLKA FACE 2',
    'a':'AL FAN MAIL',  's':'SCOTTI BROS',   'd':'DIRECT HELL',  'f':'FAN LTR READ', 'g':'GRAMMY SPEECH',
    'h':'HIT ME BABY',  'j':'JURASSIC FX',   'k':'KARAOKE SCENE','l':'LIVE KNOTTS',  ';':'SATURDAY NGT',
    "'":'TOUR BLOOPER',
    'z':'ZIP IT GOOD',  'x':'XYLOPHONE GAG', 'c':'CLOSE ENCOUNT','v':'VHS STATIC',   'b':'BONUS FEATURE',
    'n':'NETWORK CLIP',  'm':'MY LIFE',       ',':'LITTLE MOMENT','.'  :'PERIOD CORCT', '/':'FLIP SIDE',
    ' ':'CROWD NOISE',
  },
  D: {
    '`':'WEASEL STOMP',  '1':'FOIL INTRO',   '2':'TACKY VERSE',  '3':'MISSION STMT', '4':'SPORTS SONG',
    '5':'EBAY HOOK',     '6':'VIRUS FX',     '7':'CNR VERSE',    '8':'WORD CRIMES V','9':'1ST WLD PRBS',
    '0':'JACKSON PARK E','-':'RINGTONE PRD', '=':'VELVET ELVIS',
    'q':'QUEEN SUEDE V', 'w':'WEASEL STOMP V','e':'EVERYTHING U', 'r':'RICKY HOOK',   't':'TRPD DRV-THRU',
    'y':'U DONT LOVE ME','u':'UNDEF HERO',   'i':'STILL BILL',   'o':'OPEN MIC NGT', 'p':'PARTY GROUND',
    '[':'LAME CLAIM FAME',']':'JUST A JOKE',
    'a':'AMISH PARDSE V','s':'SMELLS VERSE',  'd':'DONT WR WHITE','f':'FRANK 2000 TV', 'g':'GENIUS FRANCE',
    'h':'HANDY HOOK',    'j':'JACKSON ALT',   'k':'KILLIN TIME',  'l':'SURGN LIVE',   ';':'SPORTS REF',
    "'":'OTHER DEEP CUT',
    'z':'ZELDA POLKA',  'x':'XMAS AT GRND',  'c':'CAVITY SEARCH','v':'VELVET VERSE', 'b':'BOB DYLAN STL',
    'n':'NATURE TRAIL V','m':'MIDNIGHT STAR', ',':'AL MINUTE',    '.':'ALBUQ END',    '/':'HARDWARE F',
    ' ':'ACCORDION CHRD',
  },
};

// ── STATE ────────────────────────────────────────────────
const state = {
  audioCtx:     null,
  analyser:     null,
  masterGain:   null,
  banks:        { A:{}, B:{}, C:{}, D:{} },
  currentBank:  'A',
  playback:     {},         // key -> { source, gainNode, startWall, trimIn, trimOut, segDur }
  selectedKey:  null,
  playMode:     'fire-stop',
  loopAll:      false,
  scrubAngle:   0,
  timerInterval: null,
  vuAnim:       null,
};

// ── SLOT CONSTRUCTOR ─────────────────────────────────────
function makeSlot(bank, key) {
  return {
    bank, key,
    name:         PRESETS[bank][key] || key.toUpperCase(),
    audioBuffer:  null,
    waveformData: null,
    gain:         1.0,
    trimIn:       0,
    trimOut:      1,
  };
}

// ── INIT ─────────────────────────────────────────────────
function init() {
  for (const bank of ['A','B','C','D']) {
    for (const row of KB_ROWS) {
      for (const k of row.keys) {
        state.banks[bank][k] = makeSlot(bank, k);
      }
    }
  }
  buildKeyboard();
  bindTransport();
  bindBankButtons();
  bindModeButtons();
  bindKeyboard();
  bindMasterVol();
  bindSlotControls();
  bindScrubWheel();
  bindContextMenu();
  bindZoom();
  buildVUMeters();
  startClock();
  setStatus('READY — Drag audio files onto keys to assign. Right-click for options.');
}

// ── AUDIO CONTEXT ────────────────────────────────────────
function ensureAudioCtx() {
  if (state.audioCtx) return;
  state.audioCtx   = new (window.AudioContext || window.webkitAudioContext)();
  state.masterGain = state.audioCtx.createGain();
  state.analyser   = state.audioCtx.createAnalyser();
  state.analyser.fftSize = 256;
  state.masterGain.connect(state.analyser);
  state.analyser.connect(state.audioCtx.destination);
  state.masterGain.gain.value = 0.85;
  startVUMeter();
}

// ── BUILD KEYBOARD ───────────────────────────────────────
function buildKeyboard() {
  for (const row of KB_ROWS) {
    const rowEl = document.getElementById(`kb-row-${row.id}`);
    rowEl.innerHTML = '';

    // Add modifier placeholder at start of each row
    if (row.id === 'q') {
      const mod = modKey('TAB', calc(1.5));
      rowEl.appendChild(mod);
    } else if (row.id === 'a') {
      const mod = modKey('CAPS', calc(1.75));
      rowEl.appendChild(mod);
    } else if (row.id === 'z') {
      const mod = modKey('SHIFT', calc(2.25));
      rowEl.appendChild(mod);
    }

    for (const k of row.keys) {
      const btn = createKeyBtn(k);
      rowEl.appendChild(btn);
    }

    // Right-side modifier placeholders
    if (row.id === 'q') {
      const mod = modKey('\\', calc(1.5));
      rowEl.appendChild(mod);
    } else if (row.id === 'a') {
      const mod = modKey('ENTER', calc(2.25));
      rowEl.appendChild(mod);
    } else if (row.id === 'z') {
      const mod = modKey('SHIFT', calc(2.75));
      rowEl.appendChild(mod);
    }
  }
  document.getElementById('kb-bank-label').textContent =
    `BANK ${state.currentBank} — FIRE KEYBOARD · RIGHT-CLICK ANY KEY TO ASSIGN`;
}

function calc(units) {
  return `calc(var(--ku) * ${units} - var(--kg))`;
}

function modKey(label, width) {
  const div = document.createElement('div');
  div.className = 'kb-mod';
  div.style.width = width;
  div.innerHTML = `<span>${label}</span>`;
  return div;
}

function createKeyBtn(k) {
  const slot  = state.banks[state.currentBank][k];
  const btn   = document.createElement('button');
  btn.className   = 'kb-key';
  btn.dataset.key = k;
  if (k === ' ') btn.id = 'key-space';

  const waveId = `wave-${k === ' ' ? 'space' : k.replace(/[^a-z0-9]/g, c => c.charCodeAt(0))}`;

  btn.innerHTML = `
    <div class="kb-led"></div>
    <canvas class="kb-wave" id="${waveId}" height="28"></canvas>
    <div class="kb-labels">
      <span class="kb-keychar">${keyDisplay(k)}</span>
      <span class="kb-name">${slot.name}</span>
    </div>
  `;

  btn.addEventListener('click', e => {
    if (state.playMode !== 'momentary') fireKey(k);
  });
  btn.addEventListener('contextmenu', e => { e.preventDefault(); showContextMenu(e, k); });
  btn.addEventListener('dragover',  e => { e.preventDefault(); btn.classList.add('drag-over'); });
  btn.addEventListener('dragleave', ()  => btn.classList.remove('drag-over'));
  btn.addEventListener('drop',      e  => {
    e.preventDefault(); btn.classList.remove('drag-over');
    const f = e.dataTransfer.files[0];
    if (f) loadAudioFile(k, f);
  });
  btn.addEventListener('mousedown', () => { if (state.playMode === 'momentary') fireKey(k); });
  btn.addEventListener('mouseup',   () => { if (state.playMode === 'momentary') stopKey(k); });
  btn.addEventListener('mouseleave',() => { if (state.playMode === 'momentary' && isKeyPlaying(k)) stopKey(k); });

  refreshKeyEl(k, btn);
  if (slot.audioBuffer) drawMiniWave(btn.querySelector('.kb-wave'), slot.waveformData);
  return btn;
}

// ── REFRESH SINGLE KEY ELEMENT ───────────────────────────
function refreshKeyEl(k, btn) {
  if (!btn) btn = document.querySelector(`.kb-key[data-key="${CSS.escape(k)}"]`);
  if (!btn) return;
  const slot    = state.banks[state.currentBank][k];
  const playing = isKeyPlaying(k);
  btn.classList.toggle('loaded',   !!slot.audioBuffer);
  btn.classList.toggle('playing',  playing);
  btn.classList.toggle('selected', k === state.selectedKey);
  btn.querySelector('.kb-name').textContent = slot.name;
  if (slot.audioBuffer) drawMiniWave(btn.querySelector('.kb-wave'), slot.waveformData);
}

function refreshAllKeys() {
  document.querySelectorAll('.kb-key').forEach(btn => refreshKeyEl(btn.dataset.key, btn));
  updatePolyCount();
}

// ── FIRE / STOP ──────────────────────────────────────────
function fireKey(k) {
  ensureAudioCtx();
  if (state.audioCtx.state === 'suspended') state.audioCtx.resume();

  const slot = state.banks[state.currentBank][k];

  if (!slot.audioBuffer) {
    openFilePicker(k);
    return;
  }

  switch (state.playMode) {
    case 'fire-stop':
      if (isKeyPlaying(k)) { stopKey(k); return; }
      startKey(k, false);
      break;
    case 'play-end':
      startKey(k, false);
      break;
    case 'loop-fire':
      if (isKeyPlaying(k)) { stopKey(k); return; }
      startKey(k, true);
      break;
    case 'restart':
      startKey(k, false);
      break;
    // momentary handled by mousedown/up
  }
}

function startKey(k, forceLoop) {
  stopKey(k);

  const slot = state.banks[state.currentBank][k];
  if (!slot.audioBuffer) return;

  const dur    = slot.audioBuffer.duration;
  const trimIn = slot.trimIn  * dur;
  const trimOut= slot.trimOut * dur;
  const segDur = trimOut - trimIn;

  const gainNode = state.audioCtx.createGain();
  gainNode.gain.value = slot.gain;
  gainNode.connect(state.masterGain);

  const source = state.audioCtx.createBufferSource();
  source.buffer    = slot.audioBuffer;
  source.loop      = forceLoop || state.loopAll;
  source.loopStart = trimIn;
  source.loopEnd   = trimOut;
  source.connect(gainNode);
  source.start(0, trimIn, source.loop ? undefined : segDur);

  state.playback[k] = { source, gainNode, startWall: Date.now(), trimIn, trimOut, segDur };

  source.onended = () => {
    if (state.playback[k]?.source === source) {
      delete state.playback[k];
      refreshKeyEl(k);
      updatePolyCount();
      updateLamps();
      if (state.selectedKey === k) {
        document.getElementById('clip-timer').textContent = '–:––.–';
        document.getElementById('playhead-line').style.left = '0%';
      }
    }
  };

  selectKey(k);
  refreshKeyEl(k);
  updateLamps();
  updatePolyCount();
  if (state.selectedKey === k) startClipTimer(k);
  setStatus(`▶  ${slot.name}`);
}

function stopKey(k) {
  const pb = state.playback[k];
  if (!pb) return;
  try { pb.source.stop(); } catch(e){}
  try { pb.gainNode.disconnect(); } catch(e){}
  delete state.playback[k];
  refreshKeyEl(k);
  updatePolyCount();
  updateLamps();
}

function stopAll() {
  Object.keys(state.playback).forEach(k => stopKey(k));
  document.getElementById('lcd-clip-name').textContent = '– STOPPED –';
  document.getElementById('clip-timer').textContent   = '–:––.–';
  document.getElementById('playhead-line').style.left = '0%';
  setStatus('STOP — all voices killed');
}

function isKeyPlaying(k) { return !!state.playback[k]; }

// ── SELECT KEY ───────────────────────────────────────────
function selectKey(k) {
  const prev = state.selectedKey;
  state.selectedKey = k;
  if (prev) refreshKeyEl(prev);
  refreshKeyEl(k);
  const slot = state.banks[state.currentBank][k];
  document.getElementById('lcd-clip-name').textContent =
    slot.audioBuffer ? slot.name : '– NO CLIP LOADED –';
  document.getElementById('lcd-duration').textContent =
    slot.audioBuffer ? formatTime(slot.audioBuffer.duration) : '0:00.0';
  document.getElementById('lcd-bank-label').textContent = `BANK ${state.currentBank}`;
  // Sync slot controls
  const g = Math.round(slot.gain * 100);
  document.getElementById('slot-gain').value      = g;
  document.getElementById('slot-gain-val').textContent = gainLabel(slot.gain);
  document.getElementById('slot-trim-in').value   = Math.round(slot.trimIn  * 100);
  document.getElementById('slot-trim-out').value  = Math.round(slot.trimOut * 100);
  document.getElementById('slot-trim-in-val').textContent  = `${Math.round(slot.trimIn *100)}%`;
  document.getElementById('slot-trim-out-val').textContent = `${Math.round(slot.trimOut*100)}%`;
  drawMainWaveform();
  if (isKeyPlaying(k)) startClipTimer(k);
}

// ── CLIP TIMER ───────────────────────────────────────────
function startClipTimer(k) {
  if (state.timerInterval) clearInterval(state.timerInterval);
  const slot = state.banks[state.currentBank][k];
  state.timerInterval = setInterval(() => {
    const pb = state.playback[k];
    if (!pb || !slot.audioBuffer) {
      clearInterval(state.timerInterval);
      return;
    }
    const elapsed = (Date.now() - pb.startWall) / 1000;
    const pos     = pb.trimIn + elapsed;
    const pct     = Math.min((elapsed / pb.segDur) * 100, 100);
    document.getElementById('clip-timer').textContent = formatTime(pos);
    document.getElementById('lcd-pos').textContent    = `▶ ${formatTime(pos)}`;
    document.getElementById('playhead-line').style.left = `${pct}%`;
    document.getElementById('lcd-duration').textContent = formatTime(slot.audioBuffer.duration);
  }, 80);
}

// ── LOAD AUDIO ───────────────────────────────────────────
async function loadAudioFile(k, file) {
  ensureAudioCtx();
  const slot = state.banks[state.currentBank][k];
  setStatus(`Loading ${file.name}…`);
  try {
    const ab  = await file.arrayBuffer();
    const buf = await state.audioCtx.decodeAudioData(ab);
    slot.audioBuffer  = buf;
    slot.name         = file.name.replace(/\.[^.]+$/, '').toUpperCase().substring(0, 14);
    slot.waveformData = buildWaveformData(buf);
    selectKey(k);
    refreshKeyEl(k);
    setStatus(`Loaded: ${slot.name}  (${formatTime(buf.duration)})`);
  } catch(err) {
    setStatus(`ERROR: ${err.message}`);
  }
}

function buildWaveformData(buf, samples = 200) {
  const ch   = buf.getChannelData(0);
  const step = Math.floor(ch.length / samples);
  const data = new Float32Array(samples);
  for (let i = 0; i < samples; i++) {
    let max = 0;
    const start = i * step;
    for (let j = 0; j < step; j++) {
      const v = Math.abs(ch[start + j] || 0);
      if (v > max) max = v;
    }
    data[i] = max;
  }
  return data;
}

// ── WAVEFORM DRAWING ─────────────────────────────────────
function drawMiniWave(canvas, data) {
  if (!data || !canvas) return;
  const W = canvas.offsetWidth  || parseInt(getComputedStyle(canvas).width)  || 54;
  const H = canvas.offsetHeight || parseInt(getComputedStyle(canvas).height) || 28;
  canvas.width  = W;
  canvas.height = H;
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, W, H);
  ctx.fillStyle = 'rgba(0,255,68,0.75)';
  const step = data.length / W;
  for (let x = 0; x < W; x++) {
    const v = data[Math.floor(x * step)] || 0;
    const h = Math.max(1, v * H * 0.9);
    ctx.fillRect(x, (H - h) / 2, 1, h);
  }
}

function drawMainWaveform() {
  const canvas = document.getElementById('waveform-canvas');
  const W = canvas.width, H = canvas.height;
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, W, H);

  // Grid
  ctx.strokeStyle = '#0c240c'; ctx.lineWidth = 1;
  for (let x = 0; x < W; x += 50) {
    ctx.beginPath(); ctx.moveTo(x,0); ctx.lineTo(x,H); ctx.stroke();
  }
  ctx.beginPath(); ctx.moveTo(0,H/2); ctx.lineTo(W,H/2); ctx.stroke();

  const k    = state.selectedKey;
  const slot = k ? state.banks[state.currentBank][k] : null;

  if (!slot?.waveformData) {
    ctx.font = '10px Courier New'; ctx.fillStyle = '#163a16';
    ctx.textAlign = 'center';
    ctx.fillText('NO AUDIO — DROP FILE ONTO KEY OR RIGHT-CLICK TO ASSIGN', W/2, H/2+4);
    return;
  }

  const data   = slot.waveformData;
  const inX    = Math.floor(slot.trimIn  * W);
  const outX   = Math.floor(slot.trimOut * W);

  ctx.fillStyle = 'rgba(0,0,0,0.55)';
  ctx.fillRect(0, 0, inX, H);
  ctx.fillRect(outX, 0, W - outX, H);

  const step = data.length / W;
  for (let x = 0; x < W; x++) {
    const v = data[Math.floor(x * step)] || 0;
    const h = Math.max(1, v * H * 0.86);
    ctx.fillStyle = (x >= inX && x <= outX) ? '#00cc44' : '#004422';
    ctx.fillRect(x, (H - h) / 2, 1, h);
  }

  ctx.strokeStyle = '#00ff44'; ctx.lineWidth = 1.5; ctx.setLineDash([3,3]);
  if (inX > 0) { ctx.beginPath(); ctx.moveTo(inX,0); ctx.lineTo(inX,H); ctx.stroke(); }
  if (outX < W){ ctx.beginPath(); ctx.moveTo(outX,0);ctx.lineTo(outX,H);ctx.stroke(); }
  ctx.setLineDash([]);
}

// ── VU METERS ────────────────────────────────────────────
const VU_SEGS = 16;
function buildVUMeters() {
  ['vu-left-segs','vu-right-segs'].forEach(id => {
    const wrap = document.getElementById(id);
    wrap.innerHTML = '';
    for (let i = 0; i < VU_SEGS; i++) {
      const seg = document.createElement('div');
      seg.className = 'vu-seg off';
      wrap.appendChild(seg);
    }
  });
}

function startVUMeter() {
  const lSegs = document.querySelectorAll('#vu-left-segs  .vu-seg');
  const rSegs = document.querySelectorAll('#vu-right-segs .vu-seg');
  const bufLen = state.analyser.frequencyBinCount;
  const buf    = new Uint8Array(bufLen);

  (function tick() {
    state.analyser.getByteTimeDomainData(buf);
    let pL = 0, pR = 0;
    for (let i = 0; i < bufLen; i++) {
      const v = Math.abs((buf[i] - 128) / 128);
      if (i % 2 === 0) { if (v > pL) pL = v; }
      else              { if (v > pR) pR = v; }
    }
    if (pR === 0) pR = pL;
    paintVU(lSegs, pL);
    paintVU(rSegs, pR);
    state.vuAnim = requestAnimationFrame(tick);
  })();
}

function paintVU(segs, peak) {
  const lit = Math.round(Math.min(peak * 2.8, 1) * VU_SEGS);
  segs.forEach((seg, i) => {
    if (i < lit) {
      seg.className = i < 10 ? 'vu-seg green' : i < 13 ? 'vu-seg amber' : 'vu-seg red';
    } else {
      seg.className = 'vu-seg off';
    }
  });
}

// ── POLY COUNT ───────────────────────────────────────────
function updatePolyCount() {
  const n = Object.keys(state.playback).length;
  document.getElementById('poly-count').textContent = n;
  document.getElementById('status-playing').textContent = `${n} PLAYING`;
}

// ── LAMPS ────────────────────────────────────────────────
function updateLamps() {
  const any = Object.keys(state.playback).length > 0;
  document.getElementById('lamp-play').classList.toggle('lit', any);
  document.getElementById('lamp-loop').classList.toggle('lit', state.loopAll);
}

// ── CLOCK ────────────────────────────────────────────────
function startClock() {
  (function tick() {
    document.getElementById('clock').textContent = new Date().toTimeString().slice(0,8);
    setTimeout(tick, 1000);
  })();
}

// ── TRANSPORT BINDINGS ───────────────────────────────────
function bindTransport() {
  document.getElementById('btn-play').addEventListener('click', () => {
    if (state.selectedKey) startKey(state.selectedKey, false);
  });
  document.getElementById('btn-pause').addEventListener('click', () => {
    if (!state.audioCtx) return;
    if (state.audioCtx.state === 'running') {
      state.audioCtx.suspend();
      document.getElementById('btn-pause').classList.add('pressed');
    } else {
      state.audioCtx.resume();
      document.getElementById('btn-pause').classList.remove('pressed');
    }
  });
  document.getElementById('btn-stop').addEventListener('click', stopAll);
  document.getElementById('btn-stop-all').addEventListener('click', stopAll);
  document.getElementById('btn-rew').addEventListener('click', () => {
    stopAll();
    document.getElementById('clip-timer').textContent   = '–:––.–';
    document.getElementById('playhead-line').style.left = '0%';
    if (state.selectedKey) selectKey(state.selectedKey);
  });
  document.getElementById('btn-loop').addEventListener('click', () => {
    state.loopAll = !state.loopAll;
    document.getElementById('btn-loop').classList.toggle('lit', state.loopAll);
    updateLamps();
    setStatus(`LOOP ALL: ${state.loopAll ? 'ON' : 'OFF'}`);
  });
  document.getElementById('btn-clear').addEventListener('click', clearSelectedKey);
}

function clearSelectedKey() {
  const k = state.selectedKey;
  if (!k) return;
  stopKey(k);
  const slot = state.banks[state.currentBank][k];
  slot.audioBuffer  = null;
  slot.waveformData = null;
  slot.gain         = 1.0;
  slot.trimIn       = 0;
  slot.trimOut      = 1;
  slot.name         = PRESETS[state.currentBank][k] || k.toUpperCase();
  refreshKeyEl(k);
  drawMainWaveform();
  setStatus(`Key [${keyDisplay(k)}] cleared.`);
}

// ── BANK SWITCHING ───────────────────────────────────────
function bindBankButtons() {
  document.querySelectorAll('.bank-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const bank = btn.dataset.bank;
      if (bank === state.currentBank) return;
      state.currentBank = bank;
      state.selectedKey = null;
      document.querySelectorAll('.bank-btn').forEach(b =>
        b.classList.toggle('active', b.dataset.bank === bank));
      buildKeyboard();
      drawMainWaveform();
      setStatus(`Bank ${bank} — ${countLoaded(bank)} clips loaded`);
    });
  });
}

function countLoaded(bank) {
  return Object.values(state.banks[bank]).filter(s => s.audioBuffer).length;
}

// ── MODE BUTTONS ─────────────────────────────────────────
function bindModeButtons() {
  document.querySelectorAll('.mode-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      state.playMode = btn.dataset.mode;
      document.querySelectorAll('.mode-btn').forEach(b =>
        b.classList.toggle('active', b === btn));
      document.getElementById('status-mode').innerHTML =
        `MODE: ${btn.textContent.trim()} &nbsp;|&nbsp; <span id="status-playing">${Object.keys(state.playback).length} PLAYING</span>`;
    });
  });
}

// ── KEYBOARD HANDLER ─────────────────────────────────────
function bindKeyboard() {
  // Build a flat set of all trigger keys
  const allKeys = new Set(KB_ROWS.flatMap(r => r.keys));

  document.addEventListener('keydown', e => {
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

    // Bank switching: Ctrl+A/B/C/D
    if (e.ctrlKey || e.metaKey) {
      const lk = e.key.toLowerCase();
      if (['a','b','c','d'].includes(lk)) {
        e.preventDefault();
        document.querySelector(`.bank-btn[data-bank="${lk.toUpperCase()}"]`)?.click();
        return;
      }
      if (lk === 'l')   { e.preventDefault(); document.getElementById('btn-loop').click(); return; }
      if (lk === '.')   { e.preventDefault(); stopAll(); return; }
      if (e.key === 'Delete' || e.key === 'Backspace') {
        e.preventDefault(); clearSelectedKey(); return;
      }
      return;
    }

    const k = e.key === ' ' ? ' ' : (e.key.length === 1 ? e.key.toLowerCase() : null);
    if (!k || !allKeys.has(k)) return;
    e.preventDefault();

    if (e.repeat) return; // no key-repeat triggers

    if (state.playMode === 'momentary') {
      startKey(k, false);
    } else {
      fireKey(k);
    }
  });

  document.addEventListener('keyup', e => {
    if (state.playMode !== 'momentary') return;
    const k = e.key === ' ' ? ' ' : (e.key.length === 1 ? e.key.toLowerCase() : null);
    if (!k) return;
    stopKey(k);
  });
}

// ── MASTER VOLUME ────────────────────────────────────────
function bindMasterVol() {
  const slider = document.getElementById('master-vol');
  slider.addEventListener('input', () => {
    const v = slider.value / 100;
    if (state.masterGain) state.masterGain.gain.value = v;
    document.getElementById('master-vol-val').textContent = slider.value;
  });
}

// ── SLOT CONTROLS ────────────────────────────────────────
function bindSlotControls() {
  document.getElementById('slot-gain').addEventListener('input', function() {
    const k = state.selectedKey; if (!k) return;
    const slot = state.banks[state.currentBank][k];
    slot.gain = this.value / 100;
    document.getElementById('slot-gain-val').textContent = gainLabel(slot.gain);
    const pb = state.playback[k];
    if (pb) pb.gainNode.gain.value = slot.gain;
  });
  document.getElementById('slot-trim-in').addEventListener('input', function() {
    const k = state.selectedKey; if (!k) return;
    state.banks[state.currentBank][k].trimIn = this.value / 100;
    document.getElementById('slot-trim-in-val').textContent = `${this.value}%`;
    drawMainWaveform();
  });
  document.getElementById('slot-trim-out').addEventListener('input', function() {
    const k = state.selectedKey; if (!k) return;
    state.banks[state.currentBank][k].trimOut = this.value / 100;
    document.getElementById('slot-trim-out-val').textContent = `${this.value}%`;
    drawMainWaveform();
  });
}

// ── SCRUB WHEEL ──────────────────────────────────────────
function bindScrubWheel() {
  const wheel = document.getElementById('scrub-wheel');
  let dragging = false, lastY = 0, angle = 0;
  wheel.addEventListener('mousedown', e => { dragging = true; lastY = e.clientY; e.preventDefault(); });
  document.addEventListener('mousemove', e => {
    if (!dragging) return;
    const dy = e.clientY - lastY; lastY = e.clientY;
    angle = (angle + dy * 2) % 360;
    wheel.style.transform = `rotate(${angle}deg)`;
    document.getElementById('scrub-tick').style.transform = `translateX(-50%) rotate(${-angle}deg)`;
    const k = state.selectedKey;
    if (!k) return;
    const slot = state.banks[state.currentBank][k];
    if (!slot.audioBuffer) return;
    const frac = (((angle % 360) + 360) % 360) / 360;
    const pos  = frac * slot.audioBuffer.duration;
    document.getElementById('clip-timer').textContent = formatTime(pos);
    document.getElementById('playhead-line').style.left = `${frac * 100}%`;
  });
  document.addEventListener('mouseup', () => { dragging = false; });
}

// ── CONTEXT MENU ─────────────────────────────────────────
let ctxKey = null;
function showContextMenu(e, k) {
  ctxKey = k;
  selectKey(k);
  const menu = document.getElementById('ctx-menu');
  menu.classList.remove('hidden');
  let x = e.clientX, y = e.clientY;
  if (x + 185 > window.innerWidth)  x = window.innerWidth  - 190;
  if (y + 160 > window.innerHeight) y = window.innerHeight - 165;
  menu.style.left = `${x}px`;
  menu.style.top  = `${y}px`;
}
function hideContextMenu() {
  document.getElementById('ctx-menu').classList.add('hidden');
  ctxKey = null;
}
function bindContextMenu() {
  document.addEventListener('click', hideContextMenu);
  document.getElementById('ctx-load').addEventListener('click', () => {
    if (ctxKey != null) openFilePicker(ctxKey);
    hideContextMenu();
  });
  document.getElementById('ctx-rename').addEventListener('click', () => {
    if (ctxKey == null) { hideContextMenu(); return; }
    const slot = state.banks[state.currentBank][ctxKey];
    const name = prompt('Rename key:', slot.name);
    if (name?.trim()) {
      slot.name = name.trim().toUpperCase().substring(0, 14);
      refreshKeyEl(ctxKey);
    }
    hideContextMenu();
  });
  document.getElementById('ctx-clear').addEventListener('click', () => {
    clearSelectedKey(); hideContextMenu();
  });
  document.getElementById('ctx-stop-this').addEventListener('click', () => {
    if (ctxKey) stopKey(ctxKey); hideContextMenu();
  });
  document.getElementById('ctx-copy').addEventListener('click', () => {
    if (!ctxKey) { hideContextMenu(); return; }
    const src = state.banks[state.currentBank][ctxKey];
    if (!src.audioBuffer) { hideContextMenu(); return; }
    const dest = prompt('Copy to — Bank (A/B/C/D) + Key (letter/number/symbol):\nExample:  B q   or   A 3');
    if (!dest) { hideContextMenu(); return; }
    const parts = dest.trim().split(/\s+/);
    const bank  = parts[0]?.toUpperCase();
    const dk    = parts[1] === 'space' ? ' ' : (parts[1] || '');
    if (!['A','B','C','D'].includes(bank) || !state.banks[bank]?.[dk]) {
      alert('Invalid destination. Use format: B q'); hideContextMenu(); return;
    }
    const t = state.banks[bank][dk];
    Object.assign(t, { audioBuffer: src.audioBuffer, waveformData: src.waveformData,
      name: src.name, gain: src.gain, trimIn: src.trimIn, trimOut: src.trimOut });
    if (bank === state.currentBank) refreshKeyEl(dk);
    setStatus(`Copied "${src.name}" → Bank ${bank} [${keyDisplay(dk)}]`);
    hideContextMenu();
  });
}

// ── FILE PICKER ──────────────────────────────────────────
let filePickerKey = null;
function openFilePicker(k) {
  filePickerKey = k;
  const inp = document.getElementById('file-input');
  inp.value = ''; inp.click();
}
document.getElementById('file-input').addEventListener('change', function() {
  if (this.files[0] && filePickerKey != null) loadAudioFile(filePickerKey, this.files[0]);
  filePickerKey = null;
});

// ── ZOOM ────────────────────────────────────────────────
function bindZoom() {
  document.querySelectorAll('.zoom-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.zoom-btn').forEach(b => b.classList.toggle('active', b === btn));
      drawMainWaveform();
    });
  });
}

// ── GLOBAL DROP ──────────────────────────────────────────
window.addEventListener('dragover', e => e.preventDefault());
window.addEventListener('drop', e => {
  e.preventDefault();
  if (e.target.closest('.kb-key')) return;
  const f = e.dataTransfer.files[0];
  if (f && state.selectedKey) loadAudioFile(state.selectedKey, f);
  else setStatus('Drop audio onto a key, or select a key first.');
});

// ── HELPERS ─────────────────────────────────────────────
function gainLabel(g) {
  if (g <= 0) return '-∞ dB';
  const db = 20 * Math.log10(g);
  return `${db >= 0 ? '+' : ''}${db.toFixed(1)} dB`;
}
function formatTime(sec) {
  if (!isFinite(sec) || sec < 0) return '0:00.0';
  const m = Math.floor(sec / 60);
  const s = (sec % 60).toFixed(1).padStart(4, '0');
  return `${m}:${s}`;
}
function setStatus(msg) {
  document.getElementById('status-msg').textContent = msg;
}

// ── BOOT ────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', init);
