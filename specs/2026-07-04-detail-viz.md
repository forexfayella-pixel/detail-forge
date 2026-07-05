# Spec: Accurate waveform + applied-effect visualization

**Date:** 2026-07-04
**Goal (one line):** Make the WebView display tell the truth about the DSP — reconstructed
band-limited waveform, latency-aligned dry/wet overlay, the *actual* transfer curve, and true-peak
levels — so what you see matches what the clip→limit chain is really doing.

Source research: `docs/2026-07-03-detail-viz.md`.

## Problem
The current display lies in four ways (all verified in code — see the research note for file:line):
1. **Latency smear.** `inRing` (pre) and `outRing` (post) are written at the *same* ring index, but
   the output is delayed by **L = `lastReportedLatency`** (ADAA 1 sample + oversampling group
   delay). The dry/wet overlay is misaligned by L.
2. **No inter-sample reconstruction.** The scope draws straight lines between 1024 decimated
   samples, so the inter-sample / true peaks this plugin exists to manage are invisible; meters use
   `getMagnitude()` = sample peak, not true peak.
3. **Fake transfer curve.** `drawTransfer` re-approximates hard/soft in JS, ignores the saturator,
   and diverges from the real Faust ADAA `hardclip2`↔`softclipQuadratic2` morph.
4. **No GR / "what got clipped".** `grDb` is hardcoded 0; nothing shows which tips were removed.

For a plugin whose entire thesis is true-peak-aware detail preservation, an inaccurate display
undermines trust in the DSP itself.

## Outcome
Success = a viewer can trust the display as an instrument:
- The waveform shows the **reconstructed** curve; an engineered inter-sample peak visibly rises
  above the sample dots, and the true-peak meter agrees with an independent BS.1770 computation.
- Dry vs wet overlay is **sample-aligned**: a single impulse lands at the same x pre and post, and
  stays aligned when the oversampling factor (and thus L) changes.
- The transfer curve is the **DSP's actual curve** (saturator + clip, current knee/drive/ceiling);
  turning any of those bends the drawn curve and the live signal-position indicator identically.
- The **clipped tips are highlighted** on the waveform (the removed portion), time-aligned.
- All of the above at 30–60 fps with **zero added load on the audio thread**.

## Scope
**In:**
- Latency-aligned pre/post capture (delay the pre-tap by L in the processor).
- Band-limited waveform reconstruction for display (Lanczos ≥8×); per-pixel min/max when zoomed
  out; dB/linear amplitude toggle; DPR-correct canvas.
- True-peak metering (BS.1770 ≥4× oversampled) for IN/OUT.
- DSP-probed transfer curve + live signal-position indicator.
- Clipped-region highlight (aligned pre exceeds ceiling but out is bounded).
- Lock-free audio→message transport sized for the frame rate.

**Out (deferred):**
- GR meter *values* — placeholder until the limiter lands (M2); wire the lane now, feed it later.
- WebGL renderer (revisit only if Canvas 2D drops frames or we stack scopes).
- OffscreenCanvas/worker (adopt only if the main thread janks — measure first).
- Spectrum/harmonics accuracy rework (separate concern; leave as-is this pass).
- Goniometer / correlation / loudness history.

## Key decisions
- **Architecture: A + clipped-highlight from C** (per research). Latency-aligned **Canvas 2D**
  scope + **DSP-probed** transfer + true-peak meter + clipped-tip highlight. Canvas 2D is
  sufficient for a couple thousand segments at 60 fps; WebGL is deferred, not adopted.
- **Audio → WebView, non-blocking.** Audio thread only does **wait-free writes** into a
  pre-allocated lock-free buffer (`juce::AbstractFifo` or the existing SPSC ring — a scope may drop
  stale frames). **All serialization happens on the message thread**: a 30–60 Hz editor `Timer`
  drains, decimates to ~display-width **min/max columns**, **base64-packs** the float buffer, and
  pushes one `emitEventIfBrowserIsVisible("wave", …)`. JS decodes via `atob` → `Float32Array`.
  Never JSON-encode raw sample arrays; never allocate/lock/serialize on the audio thread. Member
  order stays relays → browser → attachments (crash rule).
- **Latency alignment (ADAA + oversampling + lookahead).** One number governs it:
  **L = `lastReportedLatency`**, already computed and reported to the host via `setLatencySamples`
  (ADAA 1 + oversampling group delay; the saturator runs inside the OS region and adds none; the
  future limiter's lookahead will add into this same L). The fix is internal: **delay the pre-tap
  by L** before writing it to the ring so both taps align at one index. L updates when the OS
  factor changes, so the delay line length tracks it. Host PDC handles host-level dry/wet; it does
  **not** align our two internal taps — we do.
- **Transfer curve in sync with the DSP.** **Probe, don't approximate.** On (re)prepare and on
  relevant param change, run a −1…+1 ramp (~512 points) through the *real* saturator + clip path on
  the message thread and ship the resulting (x, y) curve to JS. Truthful by construction, and it
  sidesteps the unresolved "ADAA under a morphing knee" question. The live signal-position
  indicator is driven by the current output peak against that curve.

## Open questions
- **Probe fidelity vs cost.** Is a static per-param-change ramp probe enough, or do we need the
  curve to reflect ADAA's *dynamic* (sample-history-dependent) behaviour? Start static; revisit if
  it visibly diverges from audio.
- **Where the probe runs.** Reuse the live per-channel Faust engine (guard against reentrancy with
  the audio thread) vs a dedicated offline engine instance for probing. Offline instance is safer;
  confirm cost.
- **Display oversampling factor.** 8× Lanczos assumed sufficient for the drawn curve — confirm
  against the inter-sample test; bump to 16× only if needed.
- **Decimation vs raw ship.** Send pre-computed min/max columns (smaller, honest when zoomed out)
  vs raw samples reconstructed in JS. Leaning min/max columns + a raw window at sample-level zoom.
- **True-peak meter reuse.** Can the existing clip-stage oversamplers feed the TP meter without
  double cost / misalignment (the "oversampling placement tension" in Known-Issues)?
- **dB warp default.** Ship linear or dB amplitude by default? (Affects perceived "detail.")

## Verification
Accuracy is the acceptance bar. Before calling it done:
1. **Inter-sample peak.** Feed a signal engineered to have an inter-sample peak (e.g. a sine near
   `fs/4`, phase-offset so samples straddle the crest; sample peak reads *below* true peak). The
   drawn reconstructed curve must rise to the true peak (visibly above the sample dots), and the TP
   meter must match an independent BS.1770 4× computation (`scripts/` or a reference TP meter).
2. **Pre/post alignment.** Send a single **impulse**; pre and post must land on the same x. Toggle
   the oversampling factor (changes L) and confirm they stay aligned — validates dynamic L.
3. **Transfer probe.** Feed a slow full-scale **ramp**; the plotted output-vs-input must trace
   exactly onto the probed curve, and Knee/Drive/Ceiling/sat_* must bend both identically.
   Spot-check probed points against `Tests/engine_runner` for the same params.
4. **Clipped highlight.** With drive pushed past the ceiling, the highlighted regions must coincide
   with samples where aligned |in| > ceiling and out is bounded — and vanish when bypassed.
5. **dB / DPR.** −48 dB detail is visible in dB mode; the waveform is crisp (no blur) at
   125/150/200% display scaling.
6. **Audio-thread cleanliness.** No allocations/locks on the audio path (inspect + a scope-under-
   load pass in pluginval); frame rate holds 30–60 fps.

## Related
`docs/2026-07-03-detail-viz.md` · [[15-DSP-Signal-Chain]] · [[25-WebView-GUI]] · [[21-Clip-Engine]]
· [[53-Known-Issues]]
