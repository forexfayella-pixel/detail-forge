---
title: 21 · Clip Engine
type: module
tags: [module, dsp, faust]
status: living
updated: 2026-07-06
---

# Clip Engine

## Purpose
The M0 nonlinear core: one **ADAA2 hard-clip** stage per channel with a Drive control. Proves the Faust→JUCE binding and is the seed the M1 clipper (knee morph + oversampling) grows from.

## Key files
- `Source/dsp/clip.dsp` — Faust source. `process = *(drive) : aa.hardclip2;`, Drive is `hslider("Drive[unit:dB]") : ba.db2linear`. Box name `DetailForgeClip` → MapUI path **`/DetailForgeClip/Drive`**.
- `Source/dsp/ClipEngine.h` — **generated** (gitignored) by `faust -lang cpp -cn ClipEngine`. CMake `faust_codegen` target regenerates it from `clip.dsp`. `class ClipEngine : public dsp` — 1-in/1-out (mono); one instance per channel.
- `Source/PluginProcessor.cpp` — instantiates one `ClipEngine` + `MapUI` per channel in `prepareToPlay`, sets Drive per block, calls `compute` in-place. `setLatencySamples(1)`.

## Data flow
`processBlock` → per channel: read `drive` (dB) from APVTS → `MapUI::setParamValue("/DetailForgeClip/Drive", driveDb)` → `ClipEngine::compute(n, &d, &d)` in-place. Drive→linear and the ADAA2 clip happen inside the Faust engine.

## Binding decision (important)
We compile `.dsp` with the **faust compiler directly** (`-lang cpp -cn`) + **MapUI**, not `faust2api`. `faust2api -juce` generates a standalone JUCE *architecture* (bundles an audio driver) meant to *be* the app; for embedding in our own processor the direct-compile + MapUI path is the clean contract (Faust param paths as source of truth). → [[52-Decision-Log]].

## Config / params (all live in DSP)
- Faust zones `/DetailForgeClip/{Drive,Knee,Ceiling,Order,Detail,DetailFreq}`, cached per channel and set RT-safe each block.
- `clip_drive` 0–36 dB (**default 2** — gentle), `clip_knee` 0–100% (hard↔soft morph), `clip_ceiling` −18…0 dB (clip threshold; output bounds at ±Ceiling), `adaa_order` Off/1st/2nd → Faust `Order` (select3).
- **`clip_detail` 0–200 % → Faust `Detail` 0–2** (HF fold-back amount, **default 0 = off**), **`clip_detail_freq` 300 Hz–12 kHz** (log-skew, default 3 kHz) → `DetailFreq`. (2026-07-06)

## Detail-preserving fold-back (2026-07-06)
The Au5 "detail-preserving clipper" technique, built in-band. In `clip.dsp`:
`clipCore(x) = x : /(ceiling) : shaped : *(ceiling)` is the plain clip; then
`dpClip(x) = clipped + Detail·hf` where `residual = x − clipped` and
`hf = residual − lowpass(2, DetailFreq, residual)` (**complementary** HF, phase-coherent — see [[52-Decision-Log]]).
`Detail=0` ⇒ `dpClip == clipCore` bit-exact. Runs inside the oversampled region (HF is anti-aliased);
the JUCE layer pre-divides `DetailFreq` by the OS factor (`1<<osIndex`) because the engine is `init()`'d
at base rate. Verified: Detail=1 restores ~47× HF energy above the split while LF body RMS moves +2.8%.
**UI:** DETAIL % + D-FREQ kHz knobs live in the CLIPPER panel (bound via `kSliderIds`); the JS knob
helpers are skew-aware so the log-skewed D-FREQ recalls correctly. pluginval s8 SUCCESS. → [[25-WebView-GUI]].
**Behavior:** restores HF from *transients* (broadband residual), not steady-tone harmonics — on a sine
it's `out = x − LP(residual)` and *cleans up* upper clip harmonics. Regression: `m0_gate` drives an impulse
(Detail=0 bit-exact vs plain clip; 17.15× HF restored at Detail=1). OS-cutoff compensation verified
bit-exact 2×–16× — see the [[53-Known-Issues|192 kHz SR-clamp gotcha]] (don't re-init at the oversampled rate).
- **Oversampling** (`oversampling` 1×–16×) is applied in `PluginProcessor` via `juce::dsp::Oversampling` (orders 1–4) *around* this engine — upsample → clip → downsample; latency reported to host. See [[25-WebView-GUI]] for the UI binding.

## Gotchas / open issues
- **Latency = 1 sample** (measured; ADAA2 group delay). Reported via `setLatencySamples(1)`.
- ADAA1 ill-conditioning guard is present in the generated code (`<= 0.001f` fallback branch) → [[53-Known-Issues]].
- Alias suppression at 1× is bounded (~9–15 dB over naïve); oversampling (M1) pushes further.
- `tanh` has no 2nd-order ADAA (`tanh1` only) → for M2/M3 sat use `asinh2`/`arctan2`. → [[61-Faust-Libraries]].

## Related
[[15-DSP-Signal-Chain]] · [[61-Faust-Libraries]] · [[62-Testing]]

Up: [[20-Modules-MOC]]
