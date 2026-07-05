---
title: DSP Signal Chain
type: reference
tags: [architecture, dsp, faust, reference]
status: living
updated: 2026-07-03
---

# DSP Signal Chain (v1)

**Single-band, three-stage series topology.** Everything serves two levers: (1) suppress aliasing, (2) choose the right peak-handling behaviour for the material. See [[64-Concepts-ADAA-and-TruePeak]].

```
in → [Saturator] → [Clipper] → [Lookahead Limiter] → out
        │              │               │
   tube/tape/       ADAA hard↔      limiter_lad / lamb-style
   transformer      soft knee       smoothing, TRUE-PEAK aware
   (aanl.lib)       morph           (ceiling ≈ −1.0 dBFS)
   bias + dry/wet   per-sample      time-based GR
   + DC blocker
```

## Stage 1 — Saturator
- Character = transfer-function shape + symmetry. Symmetric (`tanh(g·x)`) → odd harmonics; **bias** adds even harmonics.
- Modes: **tube** (asymmetric soft clip, bias sets even/odd ratio), **tape** (symmetric tanh), **transformer** (squared/polynomial, low-shelf before shaper).
- Must-dos: **DC blocker after** any biased/asymmetric shaping. Optional Chebyshev polynomials for surgical harmonic control.
- **Dry/wet (parallel) Mix** is a genuine detail-preservation feature: dry path carries detail, wet adds density. The warm↔muddy gap is often 2–3 dB of drive.
- Faust: `aanl.lib` bounded/unbounded saturators.
- **Implemented (2026-07-03, M1):** a **C++ per-sample waveshaper** `DetailForgeProcessor::runSat()` — not Faust yet. Tube `tanh`, Tape `u/(1+|u|)`, Xfmr `(2/π)atan`; each `f(g·x+bias) − f(bias)` peak-normalised by `1/f(g)`; runs **inside the oversampled block before the clip** so oversampling anti-aliases it (no dedicated ADAA on the saturator yet). Bias 0–70% → even harmonics; residual DC removed by the existing output DC blocker; mix = linear dry/wet. **Open:** migrate to `aanl.lib` ADAA saturators if aliasing at low oversampling proves audible.

## Stage 2 — Clipper
- **Per-sample**, no attack/release — only affects samples above the ceiling → preserves transient punch.
- ADAA anti-aliasing (1st/2nd order). `aanl.lib` ships an ADAA2 hard-clip example with antiderivatives pre-derived.
- Hard↔soft **knee morph** → open question on ADAA under a morphing knee (closed-form per shape vs LUT-ADAA). See [[53-Known-Issues]].

## Stage 3 — Lookahead Limiter
- **Time-based** gain reduction (attack/release) — transparent for tonal overshoots.
- Faust `compressors.lib`: `limiter_lad_N` / `limiter_lad_bw` (brickwall). Or **lamb**-style sine-shaped gain smoothing ([[63-Reference-Repos]]).
- **True-peak detection** via oversampling/interpolation; default ceiling **−1.0 dBFS** (−1.2 strict). lamb has no true-peak yet — a gap we fill.

## Why clip *then* limit
Clipper preserves transients (distortion mostly inaudible on fast transients); limiter is optimal for tonal overshoot. Pro workflow: clip the tips (1–3 dB of tallest transients), then limit the rest → more punch than either alone.

## User controls (CPU/fidelity is the user's choice)
- **Oversampling selector** (off / 2x / 4x / 8x…)
- **ADAA-order toggle** (standard / 1st / 2nd)

## Cross-cutting risk
Delay alignment across ADAA + oversampling + lookahead + true-peak → **one correct reported host latency**. Highest risk of a subtly phasey plugin. See [[53-Known-Issues]] and [[52-Decision-Log]].

## Related
[[64-Concepts-ADAA-and-TruePeak]] · [[61-Faust-Libraries]] · [[63-Reference-Repos]] · [[62-Testing]] · [[20-Modules-MOC]]

Up: [[10-Architecture-MOC]]
