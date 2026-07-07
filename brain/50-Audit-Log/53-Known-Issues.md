---
title: Known Issues
type: log
tags: [audit]
status: living
updated: 2026-07-06
---

# Known Issues

Bugs, risks, open questions, tech debt. These are **anticipated** risks from research (pre-code) — confirm/close as we build.

## UI (2026-07-07)
- **[FIXED, painful] `const` TDZ in the UI module killed the whole UI (v0.10).** The delta-control block called `syncDelta()` at load, reading `const SCOPE` **before** its declaration → `ReferenceError: Cannot access 'SCOPE' before initialization`. That aborts the rest of the `<script type=module>`, so the meter/`wave` listeners never registered and `fit()`/`drawFrame` never ran → **audio worked but every visualizer was dead AND the rack overflowed the window (top/bottom cut off)** because `fit()` (defined after the throw) never scaled it. The knobs (built earlier) still rendered, masking it. **Lesson:** declare `const`/`let` used at load-time before the code that reads them; verify UI JS with the mock-harness (`/tmp/df_test.mjs` pattern: mock document/window/Juce, `await import` the module) — it catches load-time throws without a WebView. Fixed v0.11; editor size now persists (state `editorW`/`editorH`).
- **[TEST ARTIFACT] Blank standalone when the DAW is also open.** Standalone + VST3 (and rapid relaunch) share the WebView2 user-data folder (`Fayella/DetailForge/WebView2`) → lock → blank white UI. Not a plugin bug; don't screenshot the standalone while a DAW instance is loaded. → [[52-Decision-Log|2026-07-03 WebView2 folder]].

## Detail-preserving clipper (2026-07-06)
- **[GOTCHA — do not "fix"] The Faust clip engine MUST stay init()'d at the BASE rate; compensate `DetailFreq` by `/factor` instead of re-initing at the oversampled rate.** Generated `ClipEngine.h` clamps the filter-design rate: `fConst0 = π / min(192000, max(1, SR))` (line ~143). The engine is fed oversampled blocks but init'd at base rate, so `fConst0` uses the (unclamped) base rate and our `DetailFreq/factor` makes the detail cutoff land exactly at `fc` for the true rate — **verified bit-exact 2×–16×** (`os_comp_check`, equivalence of `init@base·f, fc` vs `init@base, fc/f` when unclamped). If anyone re-inits the engine at the oversampled rate "to be tidy," 8×/16× from a 48 kHz base = 384/768 kHz **exceeds the 192 kHz clamp** → the detail band silently mistunes (measured 13–25 % output divergence at 8×/16×). Same caution applies to **any future fs-dependent Faust DSP** run oversampled. → [[52-Decision-Log|2026-07-06 DetailFreq pre-divide]], [[21-Clip-Engine]].
- **[EDGE, LOW] Host base sample rate > 192 kHz would clamp even the base-rate init**, mistuning the detail cutoff. Exotic (base > 192 k is rare) and unrelated to our oversampling; note only. If ever supported, scale `DetailFreq` by `SR/min(192000,SR)` too.
- **[RESOLVED] Detail-path verification is now a committed regression.** `engine_runner` gained `--detail`/`--detail-freq` (default off) and `m0_gate.py` asserts: Detail=0 == plain clip bit-exact, and an **impulse** transient gains **>1.3× HF** above the split with Detail=1 (measured 17.15×). Runs in the ctest `m0_gate`. NOTE the probe is an **impulse (transient)**, not a sine — see below.
- **[BEHAVIOR, by-design] Detail restores TRANSIENT HF, not tonal harmonics.** On a steady sine the fold-back collapses to `out = x − LP(residual)`, which *removes* upper clip harmonics (cleaner, duller), so a sine HF-energy test reads <1× — that fooled the first gate attempt. The technique targets transient/broadband material (drums, mixes); the amount knob is the user's control. Not a bug; the gate uses an impulse accordingly.

## Visualization accuracy (implemented 2026-07-04; spec `docs/2026-07-03-detail-viz.md`)
- **[RESOLVED] Pre/post overlay latency smear.** Pre-tap delay line delays the captured dry by L=`lastReportedLatency` so `inRing`/`outRing` align at one index; impulse-verified across L=0..255 and block boundaries. `PluginProcessor.cpp` processBlock.
- **[RESOLVED] No inter-sample / true-peak reconstruction.** Display: JS Lanczos-3 8× polyphase (`reconstruct`/`drawWave`), per-pixel min/max when dense — rendered envelope reaches the true peak (1.02 vs 0.707 sample). Meters: dedicated mono 4× oversamplers + `truePeakDb()` (dBTP). Ref harness `scripts/viz_accuracy.mjs`.
- **[RESOLVED] Transfer curve was a JS approximation.** Now probed from the real engine: `computeTransferCurve()` runs a ramp (+16-sample warm-up) through a dedicated offline sat+clip instance; editor emits `transfer` on param change; JS plots the true curve + live dot. Ground-truthed via `engine_runner --signal ramp`. This also mooted the "ADAA morphing-knee" display concern.
- **[OPEN, LOW] GR viz still a placeholder.** `grDb` ≡ 0 until the limiter lands (M2); clipped-tip highlight now shipped (`drawClipHighlight`, dry beyond ±ceiling).
- **[OPEN, LOW] Live-DAW visual confirmation pending.** All accuracy claims verified numerically at the algorithm level + pluginval; a real in-host visual/audible pass (inter-sample signal shows the reconstructed peak; impulse pre/post alignment) is the remaining check.
- **[NOTE] Clipped-highlight uses dry-vs-ceiling.** Shades where the reconstructed **dry** exceeds ±clip-ceiling; with heavy in-gain/drive the true clipper input differs, so it can under-report. Accepted per spec; revisit if it misleads.
- **[NOTE] Transfer probe is static.** Ramp-through-engine captures the steady-state curve, not ADAA's sample-history dependence. Accepted for v1 per spec open question; revisit if it visibly diverges from audio.

## Resolved / open (UI, 2026-07-03)
- **[RESOLVED] Blank/white WebView.** Two causes: resource provider didn't serve `check_native_interop.js` (imported by `index.js`), and WebView2 fell back to IE for lack of `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. Both fixed; UI verified rendering. → [[25-WebView-GUI]].
- **[RESOLVED] Visualizers are now real audio.** Scope (triggered in→out), transfer (real scatter), harmonics (real FFT) all driven by captured audio via the `wave` event. → [[25-WebView-GUI]].
- **[RESOLVED] Meter row squish.** Variable-width dB readouts reflowed the flex row 30×/s; fixed with fixed-width meter columns + readouts (tabular nums).
- **[GOTCHA — support] "Section does nothing" is usually a STALE INSTALL.** 2026-07-07: user on installed **v0.4** (July-3 binary) reported the saturator/detail-clipper doing nothing — they predate M1 sat / the detail work. Always check the installed `C:\Program Files\Common Files\VST3\Detail Forge.vst3` binary date vs the build, and confirm the on-screen **version badge** (now v0.6). Also: DETAIL defaults to 0 (silent by design) — turn it up or load a preset. Fix = reinstall the fresh VST3 + DAW rescan.
- **[RESOLVED] Sections can now be enabled/disabled.** `sat_on`/`clip_on`/`lim_on` power toggles (lit/unlit UI) gate the DSP.
- **[RESOLVED 2026-07-07] M2 limiter built + GR meter live.** `TruePeakLimiter` (`Source/dsp/Limiter.h`) wired after out-gain; `grDb` now reflects real gain reduction; `lim_*` params are audible; latency accounting includes the lookahead.
- **[RESOLVED 2026-07-07] Limiter regression committed.** `Tests/limiter_runner.cpp` (JUCE-free) + ctest **`m2_gate`**: output true-peak ≤ ceiling on 4 cases incl. a near-Nyquist inter-sample signal, GR>0. `ctest -L measurement` → m0_gate + m2_gate both green.
- **[FIXED 2026-07-07, reviewer-found] Limiter `nowIdx` 32-bit overflow.** The sliding-min index counter overflowed (signed UB) after ~12 h continuous playback → could corrupt the min-window and breach the ceiling. Now 64-bit (`Limiter.h`). Caught by the `reviewer` subagent.
- **[FIXED 2026-07-07, reviewer-found] Lookahead-change deque gap.** `setParams` zeroed the monotonic deque on an `L` change → momentarily under-populated min-window (a peak could slip). Now it just updates `L` and lets `pushMin`'s front-eviction adapt (no gap).
- **[OPEN, 1.0] In-DAW latency null not yet confirmed.** Unified latency (clip/OS + limiter lookahead) is implemented and pluginval-clean, but a real host null-alignment test across OS×lookahead is the remaining [HIGH]-risk check.
- **[OPEN, expected] Bound-but-silent params.** clip_knee, clip_ceiling, oversampling, adaa_order, lim_* move real params but don't affect audio until their DSP lands (M1–M3) — so turning them won't change the (real) viz yet. GR reads 0 until the limiter. This is the top source of "why is nothing happening" confusion → prioritize M1 (clipper knee/ceiling/oversampling) so most of the clipper panel becomes audible.

## Resolved / confirmed (M0, 2026-07-02)
- **[CONFIRMED] ADAA1 ill-conditioning guard present.** The generated `ClipEngine.h` has the `<= 0.001f` fallback branch for near-equal consecutive samples — the divide-by-near-zero guard the research flagged is built into `aanl.lib`. Tune the tolerance if artifacts appear at higher orders.
- **[MEASURED] Clip-stage latency = 1 sample** (ADAA2 group delay), reported via `setLatencySamples(1)`. Confirmed by null minimising at shift=1.
- **[NOTED] ADAA linear-region floor ≈ −60 dB.** Passthrough (drive 0) through the ADAA clip does not null to silence — ADAA is not identity even for |x|≤1. A true plumbing-transparency check needs a *bypass* null, not a drive-0 null. Not a bug.

## Risks & open questions (from research)
- **[HIGH] Unified latency reporting.** Delay must be aligned across ADAA + oversampling + lookahead + true-peak into one correct host-reported latency. Highest risk of a subtly wrong / phasey plugin. M0 established the baseline (1 sample); M1 adds oversampling delay to the accounting. → [[15-DSP-Signal-Chain]].
- **[MED] ADAA under a morphing knee.** Do we derive closed-form antiderivatives per knee shape, or use lookup-table ADAA? **Path found (2026-07-02):** `aanl.lib` ships a ready ADAA knee palette (`hardclip2`, `cubic1`, `parabolic2`, `hyperbolic2`, `softclipQuadratic2`) we can crossfade; and the jatinchowdhury18/ADAA repo demonstrates **LUT+ADAA** working end-to-end as the fallback. Still resolve empirically. → deferred fallback in [[34-Do-Not-Start-Yet]].
- **[MED] Oversampling placement tension.** Nonlinear stages want oversampling; the true-peak detector *also* wants oversampling — resolve without double-cost or misalignment.
- **[MED] Primitive trust.** Verify each Faust primitive in *our* faust2api → JUCE path (double precision, version quirks, host stability) — MATLAB-clean values can misbehave in C++.
- **[MED] 1st-order ADAA ill-conditioning.** Near-equal consecutive samples → divide-by-near-zero; needs a tolerance/fallback branch. 2nd-order needs more branches.
- **[LOW] tanh 2nd antiderivative needs the dilogarithm** — some libs offer tanh ADAA at 1st order only; accept and document.
- **[LOW] lamb has no true-peak** — a gap we intend to fill.

## Open (M0 review, low priority)
- **[LOW] Drive has no smoothing → zipper on fast automation.** `clip.dsp` computes drive once per block; no per-sample interpolation. Fine for M0. Fix in M1: `si.smoo` on the drive line in Faust, or a JUCE `SmoothedValue`. → review finding #3.
- **[LOW/process] M0 gate is now automated via ctest** (`scripts/m0_gate.py`, `add_test(m0_gate)`) — asserts ADAA2≥6 dB over naive + latency==1. The Catch2 `Tests/` target the plan named was not created; the standalone `engine_runner` + python gate covers the same measurements with real pass/fail. → review finding #4 (closed).

## Standing gotchas (audio-vst)
- **[CRITICAL] WebView crash-on-unload** if member declaration order is wrong: relays FIRST, WebBrowserComponent SECOND, parameter attachments LAST. Destruction reverses declaration. → [[detail-forge-Operating-Rules]].

Up: [[50-Audit-Log-MOC]]
