/* ============================================================
   IR3 AUDIO ENGINE  (plain JS · window.IR3Audio)
   Web Audio playback, level metering, peak analysis,
   mic recording, and procedurally-generated demo clips.
   ============================================================ */
(function () {
  const TAGS = ["#e23b2e", "#ffb454", "#54ff9e", "#62b6ff", "#c77dff",
                "#ff6fa5", "#4dd0c4", "#f7d046", "#7bd45a", "#ff8a3d"];

  class Engine {
    constructor() {
      this.ctx = null;
      this.master = null;
      this.splitter = null;
      this.anL = null; this.anR = null;
      this._bufL = null; this._bufR = null;
      this.voices = [];        // {slot, src, gain, start, duration, loop, clip}
      this.tagIndex = 0;
      this.phones = 0.85;
    }

    /* lazily create the graph on first user gesture */
    ensure() {
      if (this.ctx) { if (this.ctx.state === "suspended") this.ctx.resume(); return; }
      const AC = window.AudioContext || window.webkitAudioContext;
      this.ctx = new AC();
      this.master = this.ctx.createGain();
      this.master.gain.value = this.phones;
      this.splitter = this.ctx.createChannelSplitter(2);
      this.anL = this.ctx.createAnalyser(); this.anL.fftSize = 1024; this.anL.smoothingTimeConstant = 0.2;
      this.anR = this.ctx.createAnalyser(); this.anR.fftSize = 1024; this.anR.smoothingTimeConstant = 0.2;
      this.master.connect(this.splitter);
      this.splitter.connect(this.anL, 0);
      this.splitter.connect(this.anR, 1);
      this.master.connect(this.ctx.destination);
      this._bufL = new Float32Array(this.anL.fftSize);
      this._bufR = new Float32Array(this.anR.fftSize);
    }

    nextTag() { const t = TAGS[this.tagIndex % TAGS.length]; this.tagIndex++; return t; }

    setPhones(v) { this.phones = v; if (this.master) this.master.gain.value = v; }

    /* ---- peaks for waveform thumbnails ---- */
    computePeaks(buffer, n) {
      const ch = buffer.getChannelData(0);
      const block = Math.floor(ch.length / n) || 1;
      const out = new Float32Array(n);
      let max = 0.0001;
      for (let i = 0; i < n; i++) {
        let p = 0;
        const s = i * block, e = Math.min(s + block, ch.length);
        for (let j = s; j < e; j++) { const a = Math.abs(ch[j]); if (a > p) p = a; }
        out[i] = p; if (p > max) max = p;
      }
      for (let i = 0; i < n; i++) out[i] = out[i] / max;   // normalize
      return out;
    }

    makeClip(name, buffer) {
      return { name, buffer, duration: buffer.duration,
               peaks: this.computePeaks(buffer, 96), tag: this.nextTag() };
    }

    /* ---- decode a dropped/loaded file ---- */
    async decodeFile(file) {
      this.ensure();
      const arr = await file.arrayBuffer();
      const buffer = await this.ctx.decodeAudioData(arr);
      let name = file.name.replace(/\.[^.]+$/, "").toUpperCase().slice(0, 22);
      return this.makeClip(name, buffer);
    }

    /* ---- playback ---- */
    play(clip, slot, { loop = false } = {}) {
      this.ensure();
      const src = this.ctx.createBufferSource();
      src.buffer = clip.buffer; src.loop = loop;
      const g = this.ctx.createGain();
      src.connect(g); g.connect(this.master);
      const voice = { slot, src, gain: g, start: this.ctx.currentTime, duration: clip.buffer.duration, loop, clip };
      src.onended = () => {
        this.voices = this.voices.filter(v => v !== voice);
        if (this._onChange) this._onChange();
      };
      src.start();
      this.voices.push(voice);
      if (this._onChange) this._onChange();
      return voice;
    }

    stopSlot(slot) {
      this.voices.filter(v => v.slot === slot).forEach(v => { try { v.src.stop(); } catch (e) {} });
    }
    stopAll() { this.voices.slice().forEach(v => { try { v.src.stop(); } catch (e) {} }); }

    /* fade-pause: broadcast units pause = stop on this simple model; we duck+stop */
    pauseAll() { this.stopAll(); }

    isPlaying(slot) { return this.voices.some(v => v.slot === slot); }
    playingCount() { return this.voices.length; }

    /* elapsed time on the most-recent voice of a slot */
    slotElapsed(slot) {
      const v = this.voices.filter(x => x.slot === slot).pop();
      if (!v) return null;
      let t = this.ctx.currentTime - v.start;
      if (v.loop && v.duration) t = t % v.duration;
      return { elapsed: Math.min(t, v.duration), duration: v.duration, loop: v.loop };
    }

    onChange(fn) { this._onChange = fn; }

    /* ---- L/R metering (RMS, 0..1) ---- */
    levels() {
      if (!this.anL) return { l: 0, r: 0 };
      this.anL.getFloatTimeDomainData(this._bufL);
      this.anR.getFloatTimeDomainData(this._bufR);
      const rms = (b) => { let s = 0; for (let i = 0; i < b.length; i++) s += b[i] * b[i]; return Math.sqrt(s / b.length); };
      const map = (x) => Math.min(1, Math.pow(x * 3.2, 0.8));
      return { l: map(rms(this._bufL)), r: map(rms(this._bufR)) };
    }

    /* ---- mic recording ---- */
    async startRecording() {
      this.ensure();
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      this._recStream = stream;
      this._recChunks = [];
      const mime = MediaRecorder.isTypeSupported("audio/webm") ? "audio/webm" : "";
      this._rec = new MediaRecorder(stream, mime ? { mimeType: mime } : undefined);
      this._rec.ondataavailable = (e) => { if (e.data.size) this._recChunks.push(e.data); };
      this._recStart = performance.now();
      this._rec.start();
      // live-monitor level off the mic too
      const srcNode = this.ctx.createMediaStreamSource(stream);
      this._recMonitor = this.ctx.createGain(); this._recMonitor.gain.value = 0;
      srcNode.connect(this._recMonitor); this._recMonitor.connect(this.master);
      this._recSrcNode = srcNode;
      return true;
    }

    async stopRecording(name) {
      if (!this._rec) return null;
      const blob = await new Promise((res) => {
        this._rec.onstop = () => res(new Blob(this._recChunks, { type: this._recChunks[0]?.type || "audio/webm" }));
        this._rec.stop();
      });
      this._recStream.getTracks().forEach(t => t.stop());
      try { this._recSrcNode.disconnect(); this._recMonitor.disconnect(); } catch (e) {}
      const arr = await blob.arrayBuffer();
      const buffer = await this.ctx.decodeAudioData(arr);
      this._rec = null;
      return this.makeClip(name || "REC CLIP", buffer);
    }

    /* ============================================================
       DEMO CLIP SYNTHESIS — so the deck is alive on first open
       ============================================================ */
    _env(data, sr, attack, release, dur) {
      const n = data.length, aN = attack * sr, rN = release * sr, dN = dur * sr;
      for (let i = 0; i < n; i++) {
        let e = 1;
        if (i < aN) e = i / aN;
        else if (i > dN - rN) e = Math.max(0, (dN - i) / rN);
        data[i] *= e;
      }
    }
    _buf(dur) { const sr = this.ctx.sampleRate; return this.ctx.createBuffer(1, Math.floor(sr * dur), sr); }

    genDemos() {
      this.ensure();
      const sr = this.ctx.sampleRate, mk = (d) => this._buf(d);
      const out = [];
      const push = (name, buf) => out.push({ name, buf });

      // CROWD CHEER — band-limited noise swell
      (() => { const d = 2.6, b = mk(d), x = b.getChannelData(0); let lp = 0;
        for (let i = 0; i < x.length; i++) { const t = i / sr; const n = Math.random() * 2 - 1;
          lp += (n - lp) * 0.04; let v = lp * 0.9; v *= 0.4 + 0.6 * Math.min(1, t / 1.4);
          v += 0.04 * Math.sin(2 * Math.PI * (220 + 40 * Math.sin(t * 3)) * t); x[i] = v; }
        this._env(x, sr, 0.25, 0.7, d); push("CROWD CHEER", b); })();

      // BUZZER — harsh square
      (() => { const d = 1.1, b = mk(d), x = b.getChannelData(0);
        for (let i = 0; i < x.length; i++) { const t = i / sr;
          x[i] = (Math.sign(Math.sin(2 * Math.PI * 233 * t)) * 0.45 + Math.sign(Math.sin(2 * Math.PI * 466 * t)) * 0.18); }
        this._env(x, sr, 0.005, 0.05, d); push("BUZZER", b); })();

      // APPLAUSE — random claps
      (() => { const d = 2.4, b = mk(d), x = b.getChannelData(0);
        for (let i = 0; i < x.length; i++) { const t = i / sr;
          let v = (Math.random() * 2 - 1) * (Math.random() < 0.06 ? 1 : 0.12);
          v *= 0.5 + 0.5 * Math.min(1, t); x[i] = v * 0.8; }
        this._env(x, sr, 0.1, 0.6, d); push("APPLAUSE", b); })();

      // AIR HORN — detuned saws
      (() => { const d = 1.7, b = mk(d), x = b.getChannelData(0);
        const saw = (f, t) => 2 * (f * t - Math.floor(0.5 + f * t));
        for (let i = 0; i < x.length; i++) { const t = i / sr;
          x[i] = (saw(146, t) + saw(146 * 1.5, t) * 0.7 + saw(147, t) * 0.5) * 0.16; }
        this._env(x, sr, 0.02, 0.18, d); push("AIR HORN", b); })();

      // STINGER 01 — bright tone burst with decay
      (() => { const d = 1.5, b = mk(d), x = b.getChannelData(0);
        for (let i = 0; i < x.length; i++) { const t = i / sr; const dec = Math.exp(-t * 3.2);
          x[i] = (Math.sin(2 * Math.PI * 523 * t) + Math.sin(2 * Math.PI * 784 * t) * 0.6
                + Math.sin(2 * Math.PI * 1046 * t) * 0.3) * 0.22 * dec; }
        this._env(x, sr, 0.004, 0.2, d); push("STINGER 01", b); })();

      // RIMSHOT — short snap
      (() => { const d = 0.4, b = mk(d), x = b.getChannelData(0);
        for (let i = 0; i < x.length; i++) { const t = i / sr; const dec = Math.exp(-t * 26);
          x[i] = ((Math.random() * 2 - 1) * 0.6 + Math.sin(2 * Math.PI * 320 * t) * 0.5) * dec; }
        push("RIMSHOT", b); })();

      // LASER ZAP — descending sweep
      (() => { const d = 0.7, b = mk(d), x = b.getChannelData(0);
        for (let i = 0; i < x.length; i++) { const t = i / sr; const f = 1400 * Math.exp(-t * 5) + 120;
          x[i] = Math.sin(2 * Math.PI * f * t) * 0.3 * Math.exp(-t * 2.6); }
        push("LASER ZAP", b); })();

      // WHOOSH — noise sweep
      (() => { const d = 1.2, b = mk(d), x = b.getChannelData(0); let lp = 0;
        for (let i = 0; i < x.length; i++) { const t = i / sr; const n = Math.random() * 2 - 1;
          const k = 0.02 + 0.3 * Math.sin(Math.min(Math.PI, t / d * Math.PI)); lp += (n - lp) * k;
          x[i] = lp * 0.7 * Math.sin(Math.min(Math.PI, t / d * Math.PI)); }
        this._env(x, sr, 0.1, 0.3, d); push("WHOOSH", b); })();

      return out.map(o => this.makeClip(o.name, o.buf));
    }
  }

  window.IR3Audio = new Engine();
})();
