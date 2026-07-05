---
title: Current Priorities
type: log
tags: [roadmap]
status: living
updated: 2026-07-02
---

# Current Priorities

**✅ Done — Milestone 0 (vertical slice)** and the full **UI** (all params bound, real-audio visualizers, autocorrelation scope sync, zoom-to-fit). See [[25-WebView-GUI]], [[51-Timeline]].

**✅ Done — Milestone 1 (clipper hardening):** hard↔soft **Knee** morph (`aa.hardclip2`↔`aa.softclipQuadratic2`), variable **Ceiling**, **Oversampling** 1×–16× (`dsp::Oversampling`, latency reported), **ADAA-order** Off/1st/2nd (verified −16/−21/−25 dB alias), DC blocker, gentle 2 dB default. The Clipper panel is fully live. → [[21-Clip-Engine]].

**Now (Milestone 2 — true-peak limiter → the loudness chain):**
1. Lookahead limiter after the clipper (`compressors.lib limiter_lad` derivative, or port lamb's Sanfilippo-smoothed gain) — threshold, ceiling, release. Wire GR meter.
2. True-peak / inter-sample detection; default ceiling −1.0 dBFS (−1.2 strict).
3. **Unified latency accounting** across clipper OS + limiter lookahead + true-peak → one correct reported latency (measure it).
4. Detection path phase-aligned with the audio path.

**Gate (catches phasey/wrong plugin):** reported latency sample-accurate & measured; inter-sample-peak test signal caught; detection in phase. lamb needs double precision — but our limiter path may not need lamb.

**Also worth doing:** oversampling/ADAA smoothing on switch (avoid clicks); a clipper dry/wet Mix (parallel = extra detail preservation).

**Blocked/waiting:** nothing.

Up: [[30-Roadmap-MOC]]
