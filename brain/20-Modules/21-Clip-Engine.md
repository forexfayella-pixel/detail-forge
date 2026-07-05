---
title: 21 · Clip Engine
type: module
tags: [module, dsp, faust]
status: stable
updated: 2026-07-02
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

## Config / params (all live in DSP, 2026-07-03)
- Faust zones `/DetailForgeClip/{Drive,Knee,Ceiling,Order}`, cached per channel and set RT-safe each block.
- `clip_drive` 0–36 dB (**default 2** — gentle), `clip_knee` 0–100% (hard↔soft morph), `clip_ceiling` −18…0 dB (clip threshold; output bounds at ±Ceiling), `adaa_order` Off/1st/2nd → Faust `Order` (select3).
- **Oversampling** (`oversampling` 1×–16×) is applied in `PluginProcessor` via `juce::dsp::Oversampling` (orders 1–4) *around* this engine — upsample → clip → downsample; latency reported to host. See [[25-WebView-GUI]] for the UI binding.

## Gotchas / open issues
- **Latency = 1 sample** (measured; ADAA2 group delay). Reported via `setLatencySamples(1)`.
- ADAA1 ill-conditioning guard is present in the generated code (`<= 0.001f` fallback branch) → [[53-Known-Issues]].
- Alias suppression at 1× is bounded (~9–15 dB over naïve); oversampling (M1) pushes further.
- `tanh` has no 2nd-order ADAA (`tanh1` only) → for M2/M3 sat use `asinh2`/`arctan2`. → [[61-Faust-Libraries]].

## Related
[[15-DSP-Signal-Chain]] · [[61-Faust-Libraries]] · [[62-Testing]]

Up: [[20-Modules-MOC]]
