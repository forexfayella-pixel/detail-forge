# Spec: Milestone 0 — Vertical Slice (the spike)

**Date:** 2026-07-02
**Goal (one line):** Prove the whole Faust→faust2api→JUCE 8→WebView→pluginval toolchain with the least possible DSP, and stand up the measurement rig that makes every later "detail-preserving" claim falsifiable.

## Problem
The scary risk in this plugin is **integration, not DSP theory** — the Faust→faust2api→JUCE binding, latency bookkeeping, WebView UI hosting, and whether "detail-preserving" can actually be *measured*. We retire the most risk per hour by building one thin vertical slice end-to-end before layering on saturator/limiter/oversampling.

## Outcome
A plugin that: loads in a host, shows a WebView UI shell, runs a single Faust ADAA hard-clip stage in `processBlock`, exposes one real end-to-end control (Drive, dB), reports its latency correctly to the host, passes `pluginval`, and comes with a measurement harness that prints null / aliasing / THD+N numbers **under explicit thresholds**. The real output is *confidence* + a reusable measurement rig.

## Scope
- **In:**
  - (a) A loadable **JUCE 8 plugin** (VST3 + Standalone) with a **WebView UI host** (blank/minimal panel is fine).
  - (b) **ONE Faust ADAA hard-clipper** stage (`aanl.lib` `hardclip2`, ADAA2) generated via **faust2api** and wired into `processBlock`.
  - (c) **ONE real control — Drive (dB)** — bound end-to-end via `WebSliderRelay`/`getSliderState`.
  - (d) **Correct latency reporting** (`setLatencySamples`) for whatever delay the clip stage introduces.
  - (e) A **measurement harness** (Catch2 Tests target + offline analysis) running: **null test**, **aliasing check** (swept-sine, worst-case alias level), **THD+N vs frequency**, each with explicit pass thresholds.
- **Out:** saturator, limiter, oversampling selector, ADAA-order toggle, real metering, final UI.

## Key decisions
- **Chassis:** hand-rolled Pamplejuce-style CMake project pulling **JUCE 8 + Catch2 via CPM.cmake** (full control, matches the audio-vst profile without inheriting a template's machinery).
- **Clip primitive:** `aanl.lib` **`hardclip2`** (ADAA2) — confirmed present in Faust 2.85.9. Drive applied pre-clip, compensating gain post-clip is out of M0 scope.
- **faust2api target:** generate a `DspFaust`/MapUI C++ class; set params by Faust path via `setParamValue`.
- **Precision:** double (profile rule; also future-proofs for lamb in M2).
- **WebView member order (crash guard):** relays → `WebBrowserComponent` → attachments.

## Open questions
- Exact latency (group delay) of `hardclip2` — expected 0–1 samples for pure ADAA (no oversampling in M0); **measure it**, don't assume.
- Whether faust2api's generated class name/entry differs on this Faust 2.85.9 install — confirm at generation.

## Verification
- Plugin **loads** in a host (Standalone launches; VST3 scanned).
- **`pluginval`** passes (strictness 5+).
- **Latency** reported to the host equals the measured clip-stage delay (null-test alignment).
- Harness prints **alias** and **THD+N** numbers below thresholds.

### Thresholds — RECALIBRATED to the real C++ build (2026-07-02)
The original targets below were written on paper; measuring the actual ADAA2 engine at **1× oversampling** showed they were unrealistic *for 1×* — ADAA alone gives a bounded win, and oversampling (M1) is what pushes further (research: "2nd-order ADAA ≈ 6× oversampling" is the *combination*, not ADAA alone). Recalibrated, data-backed gates:
- **Anti-aliasing works:** ADAA2 must beat a naïve (instantaneous) hard-clip control by **≥ 6 dB** worst-case-alias across a 2–9 kHz × 1–6 dB-drive sweep, measured with coherent sampling + rectangular window. *Measured: 9–15 dB improvement — PASS.*
- **Latency correct:** reported host latency == measured group delay. *Measured: 1 sample (null minimises at shift=1) — PASS.*
- **pluginval:** strictness 10 green. *PASS.*

Superseded original targets (kept for M1, where oversampling is added):
- ~~Worst-case alias ≥ 60 dB below fundamental at 1×~~ → revisit at 4×/8× in M1.
- ~~Null ≤ −120 dB~~ → passthrough through the ADAA nonlinearity has an inherent ~−60 dB linear-region floor (ADAA is not identity even for |x|≤1); true plumbing transparency belongs in a *bypass* null test, not a drive-0 test.
