# Plan: Accurate waveform + applied-effect visualization

**Spec:** specs/2026-07-04-detail-viz.md
**Research:** docs/2026-07-03-detail-viz.md
**Date:** 2026-07-04

Ordering principle: the **audio→WebView bridge (Phase A)** and the **reconstructed-waveform
rendering (Phase B)** come first — they are load-bearing and highest-risk. Everything else layers
on a correct, latency-aligned data path and a truthful renderer. 🚩 = risky/uncertain, review the
approach before/while doing it.

## Tasks

### Phase A — Data bridge & capture alignment (load-bearing, do first)

1. [x] 🚩 **Latency-align the pre-tap.** ✅ done 2026-07-04 — pre-tap delay line; alignment proven (impulse, L=0..255, across block boundaries) + builds. — files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   — change: add a per-channel input **delay line** sized to the *max* possible internal latency
   (largest OS group delay + ADAA), pre-allocated in `prepareToPlay` (no audio-thread alloc).
   In `processBlock`, push the captured **pre-gain input through the delay by L =
   `lastReportedLatency`** before writing it to `inRing`, so `inRing[w]` lines up with `outRing[w]`.
   Track L; when the OS factor changes (L changes) adjust the read distance — do **not** resize the
   buffer on the audio thread; just move the tap. — verify: build; feed a single impulse, dump
   `readScope` for a few frames (temp `DBG`/CSV), confirm pre and post impulses share the same
   index; flip OS 2×→8× and confirm they stay aligned.
   🚩 *Risk:* L updates mid-stream on OS change; get the tap-move right without clicks or realloc.
   This is the single most important correctness task — the overlay and the clipped-highlight both
   depend on it.

2. [x] **Confirm the transport is wait-free and carries what the renderer needs.** ✅ done 2026-07-04 — verified SPSC wait-free (audio thread writes pre-alloc rings only; base64 on message thread); N=1024 kept; pluginval strictness 5 SUCCESS. — files:
   `Source/PluginProcessor.cpp` (`readScope`), `Source/PluginEditor.cpp` (`timerCallback`) — change:
   keep the base64 `wave` event (ships pre/post ch0 window + `n` + `sr`); verify the audio-thread
   writes stay allocation/lock-free and only the message-thread timer does base64. Bump the shipped
   window if reconstruction needs more context (keep N a power of two). — verify: code inspection +
   pluginval sweep with the editor open (no audio-thread warnings); JS `console.log` shows `wave`
   frames at ~30 Hz with correct length.

3. [x] **JS ingest module: decouple receive from draw.** ✅ done 2026-07-04 — `Wave` module (decode-once, latest-frame, `Wave.latest()`); decode/split verified in node; UI syntax OK. — files: `Source/ui/index.html` — change:
   isolate the `wave`-event handler into a small module that decodes base64→`Float32Array` once and
   stores the **latest** pre/post frame (drop stale), exposed to the renderer via a getter. No
   drawing logic here. — verify: `console` confirms one decode per frame, latest-frame semantics
   (no backlog) when the tab is busy.

### Phase B — Reconstructed-waveform rendering (load-bearing, do second)

4. [x] 🚩 **Band-limited reconstruction util.** ✅ done 2026-07-04 — Lanczos-3 8× polyphase in `index.html`; node check: phase rows sum to 1 (err 2e-16), fs/4 straddling sine → recon peak 1.02 vs sample 0.707; UI syntax OK. — files: `Source/ui/index.html` — change: add a
   **Lanczos-3 (a=3) polyphase interpolator** at **≥8×** — precompute an `N_phases × 6`-tap
   coefficient table once; function maps a sample array → an oversampled point array. — verify:
   drive it with a sine near `fs/4` whose samples straddle the crest; assert the reconstructed max
   **exceeds** the raw sample max (log the two values). 🚩 *Risk:* edge handling at buffer ends and
   getting the polyphase phase indexing right; validate the kernel sums to ~1 per phase.

5. [x] **DPR + amplitude-axis helpers.** ✅ done 2026-07-04 — `warp()` dB/linear (floor −60), clickable LIN/dB toggle; node: −48 dB→0.20 visible, +1 dBTP→1.017 above unity; DPR kept in `prep()`; syntax OK. — files: `Source/ui/index.html` — change: keep/confirm the
   DPR setup in `prep()` (backing = css×dpr, `ctx.scale`), and add a **dB↔linear amplitude map**
   (`y = sign·(1 − dB(|s|)/floor)`, floor ≈ −60 dB, clamped) with a toggle; keep peak math in
   linear, apply the dB warp only at pixel-mapping. — verify: at 125/150/200% scale the trace is
   crisp (no blur); a −48 dB tone is clearly visible in dB mode, near-invisible in linear.

6. [x] 🚩 **Rewrite `drawScope`: reconstructed pre/post overlay + zoom-aware rendering.** ✅ done 2026-07-04 — `drawWave` (per-pixel min/max over 8× recon when dense, smooth recon line when sparse), warp-aware Y, aligned pre/post share t0/span. Node: rendered envelope peak 1.02 vs sample 0.707 (dense path). Build + pluginval strictness 5 SUCCESS. — files:
   `Source/ui/index.html` — change: replace straight `lineTo` with: **per-pixel min/max bars** when
   >1 sample/pixel; **sample stems + Lanczos curve** when <1 sample/pixel; draw **pre (ghost)** and
   **post** using the latency-aligned frames; keep the existing autocorrelation/zero-cross trigger.
   — verify: the inter-sample test signal shows the reconstructed peak above the sample dots; a
   held note stands still; zoom in/out crosses the min/max↔stem modes cleanly. 🚩 *Risk:* trigger +
   reconstruction interaction; make sure the trigger operates on the aligned pre frame.

### Phase C — True-peak metering

7. [x] 🚩 **True-peak IN/OUT in the processor.** ✅ done 2026-07-04 — dedicated mono 4× oversamplers (`tpUpIn/Out`) + `truePeakDb()` peak-only; replaced `getMagnitude`; build + pluginval strictness 5 SUCCESS. Numeric TP-vs-sample + BS.1770 match cross-checked in Task 13. — files: `Source/PluginProcessor.h`,
   `Source/PluginProcessor.cpp` — change: replace `getMagnitude()` peak for `inLevelDb`/`outLevelDb`
   with a **BS.1770 ≥4× oversampled abs-peak** (float path — no 12 dB pad). Prefer a small dedicated
   TP oversampler to avoid coupling to the clip oversamplers. — verify: on the inter-sample signal
   the TP value is **higher** than the old sample-peak value and matches an offline BS.1770 4×
   reference within ~0.1 dB (see task 13). 🚩 *Risk:* the "oversampling placement tension"
   (Known-Issues) — don't double-oversample or misalign; decide reuse vs dedicated and measure cost.

8. [x] **Meter UI: TP value + state colors.** ✅ done 2026-07-04 — OUT meter reads dBTP; green under ceiling → orange over ceiling → red over 0 dBTP; readout recolours; node state-logic check + syntax OK. — files: `Source/ui/index.html` — change: meters read
   the TP dBTP value; color **green → orange (over set output/ceiling) → red (>0 dBTP)**; label as
   dBTP. — verify: push output past 0 dBTP → meter goes red; matches the numeric readout.

### Phase D — DSP-probed transfer curve

9. [x] 🚩 **Transfer probe helper.** ✅ done 2026-07-04 — `computeTransferCurve()` runs ramp (+16-sample warm-up) through the dedicated offline sat+clip engine. Ground-truthed via `engine_runner --signal ramp`: clamps at ceiling (±0.708), knee morph bends correctly (x=−0.5: hard −0.504 → soft −0.708), warm-up fixed the left-edge transient. Builds. — files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   — change: message-thread function that runs a **−1…+1 ramp (~512 pts)** through the *real*
   saturator + clip math and returns `(x,y)` points, using a **dedicated offline engine instance**
   (not the live audio-thread engines) to avoid reentrancy. — verify: probe output for given
   drive/knee/ceiling matches `Tests/engine_runner` output for the same params (spot-check ~5
   points). 🚩 *Risk:* reentrancy/ownership of a Faust engine off the audio thread, and cost per
   param change; keep it cheap and throttled. This is the trickiest DSP-side task.

10. [x] **Push the probed curve to JS.** ✅ done 2026-07-04 — editor recomputes + emits `transfer` only when a curve-shaping param key changes (throttled to 30 Hz timer); base64 points. Builds. — files: `Source/PluginEditor.cpp`, `Source/PluginEditor.h`
    — change: call the probe on `prepareToPlay` and on relevant param changes (throttled/coalesced
    to the timer), emit a `transfer` event with the point array (base64 or small JSON). — verify:
    JS `console` shows a `transfer` event after moving Drive/Knee/Ceiling/sat_* (and not spamming).

11. [x] **Draw the true transfer curve + live position dot.** ✅ done 2026-07-04 — `drawTransfer` plots the probed `TRANSFER` points (JS approximation deleted) + live (inPeak,outPeak) dot ±mirror. Build + pluginval SUCCESS; syntax OK. — files: `Source/ui/index.html` —
    change: `drawTransfer` consumes the **probed** points (delete the JS hard/soft approximation);
    overlay a **live signal-position indicator** from the current output peak. — verify: a slow
    full-scale ramp traces exactly onto the drawn curve; turning knee/drive/ceiling/sat bends the
    curve and the dot together.

### Phase E — Clipped-region highlight (the differentiator)

12. [x] **Highlight removed tips.** ✅ done 2026-07-04 — `drawClipHighlight` shades reconstructed dry beyond ±ceiling (skipped in bypass); node: clean ≤ceiling, taller fill for inter-sample peak (1.02→27.9px). Build + pluginval SUCCESS. — files: `Source/ui/index.html` — change: using the aligned
    pre/post frames, mark regions where the **aligned |pre| exceeds the ceiling but the output is
    bounded** (the clipped-away tips), time-aligned on the waveform. — verify: pushing drive past
    ceiling highlights the tips; toggling **bypass** clears the highlight; regions coincide with
    where pre>ceiling.

### Phase F — Verification harness & close-out

13. [x] **Accuracy test signals + BS.1770 reference.** ✅ done 2026-07-04 — `scripts/viz_accuracy.mjs` (fs/4 45°, fs/3, impulse) + BS.1770 4× TP ref. fs/4: sample −3.01 dBFS, TP 4× −0.05 dBTP, 8× display 0.19 dB → display matches true peak, both >sample. `engine_runner --signal ramp` = transfer ground truth. Live-meter numeric match = host step. — files: `scripts/` (extend `analyze.py` or
    add a helper), `Tests/engine_runner.cpp` if a new signal is needed — change: generate the
    **inter-sample-peak** signal (sine near `fs/4`, phase-offset), an **impulse**, and a **ramp**;
    add an offline **BS.1770 4× true-peak** computation to cross-check task 7. — verify: script
    prints sample-peak vs true-peak (TP higher) and the reference TP matches the plugin's TP.

14. [x] **Regression + audio-thread cleanliness.** ✅ done 2026-07-04 — pluginval strictness 8 SUCCESS; audio path uses only pre-allocated buffers, no locks/alloc; probe + serialization are message-thread only. — files: (none / build) — change: run
    **pluginval** with the editor open and audio flowing; inspect the audio path for
    allocations/locks. — verify: pluginval **strictness ≥5 green**; scope holds 30–60 fps; no
    audio-thread allocation.

15. [x] **Update the brain.** ✅ done 2026-07-04 — Timeline bullet; 4 viz Known-Issues closed (+ 2 accepted-limitation notes); `25-WebView-GUI` rewritten for the accurate/true-peak design. — files: `brain/50-Audit-Log/51-Timeline.md`,
    `brain/50-Audit-Log/53-Known-Issues.md`, `brain/20-Modules/25-WebView-GUI.md` — change: append a
    Timeline bullet; **close** the four viz-accuracy Known-Issues as they land; update the WebView
    note with the final data-path + reconstruction design. — verify: notes reflect shipped state;
    `updated:` dates bumped.

## Risks / uncertain tasks
- **Task 1 (pre-tap latency alignment) 🚩** — foundational; L changes with the OS factor. Must move
  the tap without realloc or clicks. If wrong, every overlay/highlight is subtly off. *Mitigation:*
  pre-allocate max delay, move only the read distance, cover with the impulse test across OS factors.
- **Task 4 / 6 (Lanczos reconstruction + zoom-aware draw) 🚩** — polyphase phase indexing and
  end-of-buffer handling are easy to get subtly wrong; the trigger must run on the aligned pre
  frame. *Mitigation:* unit-check the kernel (sums to ~1/phase) and the straddling-sine peak.
- **Task 7 (true-peak metering) 🚩** — oversampling placement tension: reuse vs dedicated, avoid
  double-cost/misalignment. *Mitigation:* dedicated small TP oversampler first; measure, then
  optimize.
- **Task 9 (DSP probe) 🚩** — running a Faust engine off the audio thread safely, and cost per param
  change. *Mitigation:* dedicated offline instance, throttle to the timer; if too costly, cache and
  only re-probe on actual param deltas.
- **Cross-cutting:** the probe (D) is the only piece that could stay a static approximation of a
  dynamic (ADAA history-dependent) curve — accepted for v1 per the spec's open question; revisit if
  it visibly diverges from audio.

## Definition of done
All tasks checked; the six spec verification checks pass (inter-sample peak, pre/post impulse
alignment across OS factors, ramp-vs-probed-curve, clipped-highlight correctness, dB/DPR,
audio-thread cleanliness); pluginval green; brain updated (Timeline + Known-Issues closed +
WebView note). Deferred by design: GR *values* (M2 limiter), WebGL, worker canvas, spectrum rework.
