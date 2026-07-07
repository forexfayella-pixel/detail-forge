# Spec: Milestone 2 — True-Peak Lookahead Limiter (the loudness chain)

**Date:** 2026-07-06
**Goal (one line):** Add stage 3 — a transparent, true-peak-aware lookahead limiter after the clipper — so the chain hits a guaranteed inter-sample ceiling while the clipper keeps transient punch, with one correct host-reported latency.

## Problem
The saturator (stage 1, live) and detail-preserving clipper (stage 2, live) shape and densify the signal, but nothing guarantees the **output true-peak ceiling**. Clipping controls the tips; a limiter transparently catches everything else and enforces a real inter-sample bound (−1.0 dBFS default). Today `lim_threshold`/`lim_ceiling`/`lim_release` are declared but **bound-but-silent**, and the **GR meter reads 0** (`PluginProcessor.cpp` `grDb ≡ 0`). This is also the project's **top flagged risk — [HIGH] unified latency reporting** (`53-Known-Issues`): the limiter adds a lookahead delay that must be folded into one correct reported latency alongside the clip/oversampling delay, or the plugin goes subtly phasey.

## Outcome
Push the chain hard and the **output true-peak never exceeds the limiter ceiling** (verified with an engineered inter-sample-peak signal on an oversampled meter), gain reduction is **audibly transparent** on tonal overshoots and **stereo-image-stable** (linked GR), the **GR meter moves**, and the host-reported latency is **sample-accurate and measured** (null-aligned). Turning the three (or more) limiter knobs now changes sound. Defaults stay gentle/transparent (consistent with the clip drive 2 dB philosophy).

## Scope
- **In:**
  - (a) A **lookahead limiter** stage running **at base rate, after (outside) the clipper's oversampling region**, before the output-gain/DC-blocker tail.
  - (b) **True-peak detection**: an internal inter-sample peak detector (oversampled) driving the gain computer, so the ceiling bounds the reconstructed peak, not just sample peaks.
  - (c) **Stereo-linked gain reduction** (shared GR across channels → no image shift).
  - (d) **Smooth gain envelope** — attack/release with no abrupt direction changes (lamb/Sanfilippo-inspired), lookahead-aligned so attack is click-free.
  - (e) **Unified latency accounting**: `clip/OS delay + limiter lookahead (+ any detector delay)` summed into a single `setLatencySamples()`, with the detection/sidechain phase-aligned to the audio path. Extend the existing pre-tap delay pattern.
  - (f) **GR metering wired**: replace the `grDb ≡ 0` placeholder with real gain-reduction dB to the UI meter.
  - (g) Any **new params** the design needs (see decisions): likely `lim_attack` and/or `lim_lookahead`.
- **Out (deferred, do NOT pull in — `34-Do-Not-Start-Yet`):** multiband / per-band limiting; LazyLimiter-grade brute-force fidelity (~186 ms); AU/AAX packaging; oversampling *of the limiter itself* beyond what true-peak detection needs.

## Key decisions
Each has a **recommendation**; the ⚑ ones genuinely change the build and want your sign-off before planning.

- **⚑ Implementation approach — DECIDED 2026-07-06: hand-written C++ limiter at base rate.** Options weighed:
  - *lamb (Faust, Sanfilippo-smoothed):* best-in-class smoothing, but needs **double precision** (would force the engine/zone types off `float`, tripping the `static_assert` and the RT zone-cache pattern) and **has no true-peak** — we'd add TP anyway.
  - *`limiter_lad` derivative (Faust stdlib, single-precision OK):* simple peak-hold + attack/release; also not true-peak natively; another generated engine + zone plumbing.
  - *Hand-written C++ (like the existing `runSat` waveshaper):* single precision, **owns the lookahead delay and TP detector cleanly** (mirrors the existing `truePeakDb` oversampler pattern), makes the [HIGH] latency accounting trivial (our own integer delay), decoupled from Faust version quirks. Can borrow lamb's *smoothing idea* without its double-precision constraint.
  - **Recommendation: C++.** It's the lowest-risk path to the two hardest requirements (true-peak + unified latency) and keeps the whole audio path single-precision. Revisit lamb only if C++ smoothing proves audibly inferior.
- **⚑ Placement — RECOMMEND: outside/after the clip oversampling region, at base rate.** A TP limiter must bound the *final downsampled* output and add its lookahead to base-rate latency. Running it inside the 2×–16× region would scale its lookahead/ceiling by the OS factor and force GR back across the downsampler — exactly the "[MED] oversampling-placement" misalignment risk. Outside = clean. (Log as a Decision-Log entry once confirmed.)
- **True-peak detection — RECOMMEND: dedicated internal 4× oversampled peak detector in the gain-computer path**, reusing the `truePeakDb`/`juce::dsp::Oversampling` pattern (BS.1770-style, `64-Concepts §2`). Detector delay (if any) folded into latency. Default ceiling **−1.0 dBFS**, strict mode −1.2.
- **Stereo linking — RECOMMEND: yes, linked** (max of per-channel detectors → one GR curve). A mastering limiter must not shift the image. Mono path unaffected.
- **Threshold vs ceiling semantics — RECOMMEND:** `lim_ceiling` = the hard true-peak output bound (make-up-independent brickwall target); `lim_threshold` = where gain reduction *begins* (soft-ish onset above threshold, brickwall at ceiling). If that dual-knob model proves redundant, collapse to ceiling-only + drive. Pin this in planning.
- **New params — RECOMMEND:** add `lim_attack` (e.g. 0.05–20 ms, short default) and `lim_lookahead` (e.g. 0–5 ms, or fixed = attack). Keep existing `lim_threshold`/`lim_ceiling`/`lim_release`. Bind in `createLayout` + UI (LIMITER panel already exists with 3 knobs).
- **Precision — RECOMMEND: stay single (`float`).** Contingent on the C++ decision; keeps the `static_assert` valid and zone caches as `float*`. (The 2026-07-02 "double precision at M2" note was conditional on adopting lamb.)

## Open questions
- Does a soft-knee onset (threshold < ceiling) add value over a pure brickwall-at-ceiling + drive, for this clip→limit chain? Decide in planning; prototype both.
- Lookahead length vs transparency vs latency — what's the shortest lookahead that stays click-free with the chosen smoothing? Measure.
- Is the existing `truePeakDb` 4× enough for the *limiting* path, or does the gain computer need higher TP oversampling to actually hold the ceiling? Verify against an engineered inter-sample signal.
- Roadmap note discrepancy: `31-Current-Priorities` calls this **M2**, `32-Near-Term-Roadmap` numbers it M3 (M2=saturator, now folded in). Reconcile the two notes.

## Verification
- **Ceiling held (the core gate):** an engineered **inter-sample-peak** signal (true peak ≫ sample peak) pushed into the chain → measured **output dBTP ≤ ceiling** on the 4× true-peak meter, across drive/threshold sweeps. This is the M2 gate that catches a fake/sample-only limiter.
- **Latency sample-accurate + measured:** null-align dry vs wet (impulse) → reported `setLatencySamples()` equals the measured delay across all OS factors × lookahead settings. Detection path in phase.
- **Transparency:** on tonal overshoots, THD+N stays low and no zipper/pumping at default release; GR envelope has no abrupt direction changes (inspect GR trace).
- **Image stability:** a hard-panned transient does not shift with linked GR.
- **Regression:** extend `engine_runner` + `m0_gate.py` with a limiter case (ceiling-hold on an inter-sample signal; GR > 0 when driven; latency check) — same pattern as the detail-path regression just added.
- **pluginval** strictness 8–10 green; GR meter visibly moves in the running plugin (screenshot/host).
