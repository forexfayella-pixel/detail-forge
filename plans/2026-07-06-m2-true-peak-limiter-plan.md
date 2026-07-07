# Plan: M2 — True-Peak Lookahead Limiter (hand-written C++)

Spec: `specs/2026-07-06-m2-true-peak-limiter.md` · Decision: C++, base-rate, outside the clip OS region, stereo-linked, single precision.
Work one task at a time; verify each before the next. Signal order becomes:
`in_gain → sat → clip (OS region) → DC blocker → out_gain → **LIMITER (final ceiling)** → scope/meter taps`.

---

### Task 1 — Limiter params + UI knobs
- **Files:** `Source/PluginProcessor.cpp` (`createLayout`), `Source/PluginProcessor.h` (raw ptrs), `Source/ui/index.html` (LIMITER panel + binding list + INIT + presets).
- Add `lim_attack` (0.05–20 ms, log-skew, default ~1 ms) and `lim_lookahead` (0–5 ms, default = attack or 1.5 ms). Keep `lim_threshold`/`lim_ceiling`/`lim_release`. Bind raw pointers.
- UI: add ATTACK + LOOKAHEAD knobs to the LIMITER panel (second row), add both IDs to `kSliderIds` and the JS state list + INIT. `hz`/`ms` formatters already exist.
- **Verify:** build DetailForge; pluginval params enumerate; standalone screenshot shows the new knobs (skew-aware helper already handles the log ranges).

### Task 2 — Limiter DSP core (self-contained, testable)
- **Files:** new `Source/dsp/Limiter.h` (header-only C++ class, JUCE-free so it can be unit-tested offline). No `juce::dsp` dependency — implement the true-peak detector as a self-contained 4× polyphase FIR upsampler (halfband ×2) so the class is portable.
- Contents: (a) per-channel **lookahead ring** (integer delay = lookahead samples); (b) **true-peak detector** — 4× upsample the lookahead window, take max-abs (inter-sample peak); (c) **gain computer** — target gain = `min(1, ceiling/peak)`; (d) **smooth envelope** — attack/release one-pole (or dual-slope) with no abrupt direction change (lamb-inspired); GR = current gain; (e) **stereo link** — one detector fed by max-abs across channels → one GR applied to all. Expose `prepare(sr, maxBlock)`, `process(buffers, n)`, `getGainReductionDb()`, `getLatencySamples()`.
- **Verify:** new `Tests/limiter_runner.cpp` (JUCE-free, like `engine_runner`) drives it with an engineered inter-sample-peak signal → assert output dBTP ≤ ceiling, GR>0 when driven, no overshoot, and reported latency == lookahead+detector delay. Compile with `cl` and run.

### Task 3 — Wire limiter into processBlock
- **Files:** `Source/PluginProcessor.h` (member `Limiter`), `Source/PluginProcessor.cpp` (`prepareToPlay`, `processBlock`, `releaseResources`).
- Instantiate/prepare in `prepareToPlay`; run **after** the DC blocker + output gain, at base rate (final stage before the scope/out-meter taps). Read the 5 params per block (RT-safe, no allocation). Bypass path unchanged.
- **Verify:** build; drive a hot signal → output true-peak (on `tpUpOut`) ≤ ceiling; pluginval s8.

### Task 4 — Unified latency accounting
- **Files:** `Source/PluginProcessor.cpp` (`prepareToPlay`, `processBlock` latency block, `preTapDelay` sizing).
- Total latency = `clip/OS delay + limiter.getLatencySamples()`; feed into the single `setLatencySamples()` and grow the pre-tap delay cap so the scope dry/wet stay aligned. Update `captureLatency` to include the limiter term.
- **Verify:** impulse null-align (dry vs wet) reports exact latency across OS factors × lookahead settings; scope pre/post still aligned.

### Task 5 — GR metering wired
- **Files:** `Source/PluginProcessor.cpp` (`processBlock` — replace `grDb.store(0.0f)` with real GR), UI already has a GR meter.
- Store `limiter.getGainReductionDb()` into `grDb`; confirm the UI GR meter + LED react.
- **Verify:** running standalone — GR meter moves under drive; screenshot.

### Task 6 — Regression test
- **Files:** `Tests/limiter_runner.cpp` (from Task 2) + `scripts/m0_gate.py` (add a limiter gate) or a dedicated `scripts/m2_gate.py` wired into `add_test`.
- Assert: ceiling held on an inter-sample signal; GR>0 when driven; latency == expected. Keep it fast (Linux CI-friendly).
- **Verify:** ctest green (m0_gate + limiter gate).

### Task 7 — Validate + document
- **Files:** brain `51-Timeline`, `52-Decision-Log` (threshold/ceiling model, lookahead length chosen), `53-Known-Issues` (close bound-but-silent lim_* + GR placeholder), `15-DSP-Signal-Chain` (stage 3 now live), `21`-style module note `22-Limiter.md` (new).
- **Verify:** build all formats; pluginval s8–10 green; full ctest green; screenshot of the live limiter + moving GR. Reconcile the M2/M3 roadmap-numbering discrepancy.

---
**Open items to settle during planning-of-tasks:** soft-knee onset (threshold<ceiling) vs ceiling-only+drive — prototype in Task 2; shortest click-free lookahead — measure in Task 2; whether 4× TP detection actually holds the ceiling or needs 8× — verify in Task 2's inter-sample test.
