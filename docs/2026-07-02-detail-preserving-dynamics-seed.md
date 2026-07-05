# Research: Detail-Preserving Clipper / Limiter / Saturator

**Date:** 2026-07-02
**Type:** seed findings (external research, pre-code)
**Goal:** identify the correct method to build a detail-preserving saturator → clipper → limiter plugin in Faust + JUCE 8, and the primitives/references that shortcut the build.

> Drop this file into `docs/` as the research baseline. It was produced from external web research, so `/research` should *validate and extend* it (confirm current library signatures and live repos), not re-derive it.

---

## The unifying principle

"Detail-preserving" is not one feature across the three tools — it reduces to **two levers**:

1. **Suppress aliasing.** Every nonlinearity (a clipper corner, a saturator curve, a hard ceiling) generates harmonics above Nyquist that fold back into the audible band as inharmonic energy unrelated to the signal. That folded energy *is* what listeners hear as "harsh / brittle / digital / smeared detail." Killing it is the single biggest fidelity lever.
2. **Choose the right peak-handling behaviour** for the material so you don't throw away transients or dynamics you meant to keep.

Everything below serves those two.

---

## Anti-aliasing (the core engine)

Two techniques; best practice is to **combine** them.

- **Oversampling** — run the nonlinearity at 4x/8x/16x, filter, downsample. Works, but multiplies CPU by the factor, and downsampling can nudge peaks and eat headroom (bad for a peak-control plugin).
- **ADAA (antiderivative anti-aliasing)** — the differentiator. Introduced Parker/Zavalishin/Le Bivic, DAFx 2016 (Native Instruments). Instead of applying `f(x)`, apply its antiderivative and take a discrete difference; aliasing is suppressed at the source. Caveats to design for:
  - **1st-order ADAA is ill-conditioned** when consecutive samples are nearly equal (divide-by-near-zero) → needs a tolerance/fallback branch. Values that behave in MATLAB can misbehave in C++; tune in the real build.
  - **2nd-order** needs more fallback branches but suppresses more.
  - **tanh** specifically: the 2nd antiderivative needs the dilogarithm, so some libs offer tanh at 1st-order only. Accept and document.
  - **Key finding:** ADAA + modest oversampling beats heavy oversampling alone at a fraction of the CPU (2nd-order ADAA ≈ 6x oversampling for alias suppression). This combination is the detail-preservation core.

---

## Clipper vs limiter (they preserve detail in opposite ways)

This shapes the architecture — they are not interchangeable, and pros use both in series.

- **Clipper** = per-sample. No attack/release, no gain bleed into neighbours; only affects samples that exceed the ceiling. Preserves transient punch. Trade = distortion, but distortion is mostly *inaudible on fast transients* and audible on sustained/tonal content — so clipping drum transients is nearly free while clipping tonal content colours it.
- **Limiter** = time-based gain reduction (attack/release). Pulls down surrounding audio with the peak → the "squashed" sound when pushed, but transparent for tonal overshoots.
- **The pro workflow / our topology:** clip the tips (1–3 dB of the tallest transients), then limit the rest. Preserves more punch than either alone. (Tokyo Dawn's framing: limiter is optimal for tonal overshoot; clipper is optimal for transients in noisy spectra.)

**True-peak / inter-sample peaks:** between samples, the reconstructed waveform can exceed any sample value. Detect via oversampling/interpolation. Default ceiling ≈ **-1.0 dBFS**, **-1.2 dBFS** for strict true-peak delivery.

---

## Saturation (transfer function is the whole game)

Character comes entirely from the curve's shape and symmetry.

- **Symmetric curve (e.g. `tanh(g·x)`) → odd harmonics only.** Asymmetry — in the curve or the input — unlocks **even harmonics**.
- **Archetypes / modes:** *tube* = asymmetric soft clip, a **bias** param sets the even/odd ratio; *tape* = symmetric tanh sigmoid; *transformer* = squared/polynomial with both terms, usually a low-shelf before the shaper.
- **Implementation must-dos:** put a **DC blocker after** any asymmetric/biased shaping (bias = sliding the curve on the y-axis = DC offset). For surgical harmonic control, **Chebyshev polynomials** let you dial exactly which harmonics appear.
- **Voicing rule:** at low drive harmonics enhance; at high drive they mask detail and add mud — the gap between "warm/full" and "muddy" is often **2–3 dB of drive**. A **dry/wet (parallel) Mix** is therefore a genuine detail-preservation feature: the untouched path carries the detail while the wet path adds density.

---

## What this maps to in Faust (the shortcut)

Faust ships most of the hard parts; verify current signatures during `/research`.

- **`aanl.lib`** — aliasing-suppressed nonlinearities via 1st- and 2nd-order antiderivative approximations; designed to combine with low-factor oversampling (96/192 kHz); includes bounded/unbounded saturators and a worked ADAA2 hard-clip example with the antiderivatives pre-derived. → the saturator/clipper backbone; the clipper's anti-aliasing is nearly off-the-shelf.
- **`compressors.lib`** — `limiter_lad_N` (lookahead, N-channel) and ready-made `limiter_lad_bw` (brickwall). Documented quality signal: pop song, 60 dB amplification, 0-dB ceiling → loudest peak ~0.38 dB. Note `limiter_1176` is just the generic compressor with hardwired params (an example, not a distinct algorithm).

**Reference repos to read before writing DSP:**
- **magnetophon/lamb** (Bart Brouns — author of the Faust std-lib limiter functions). Lookahead limiter using Dario Sanfilippo's sine-shaped gain smoothing (gain never abruptly changes direction → smooth even at short times). Directly relevant: at minimum attack/release/hold it **becomes a waveshaper/clipper** — it already embodies the clipper↔limiter continuum we're building. Constraints: needs **double precision (Faust 2.72.14+)**; **no true-peak yet** → a gap we can fill as a differentiator.
- **magnetophon/LazyLimiter** — brute-force, very low distortion (zero on sines to low bass), heavy CPU, ~186 ms latency. Reference for the fidelity ceiling.
- **jatinchowdhury18 ADAA repo** — C++/JUCE, the clearest practical ADAA reference and a good model for a mode-switch UI (standard / 1st / 2nd / oversampling) to mirror.

---

## The perceptual frontier (north star, out of MVP scope)

**Newfangled Elevate** splits audio into up to **26 critical/Mel bands** and limits each independently, adapting gain/speed/transient per band in real time, with a spectral clipper at the end — which is *why* it stays transparent when pushed. But it's extremely CPU-heavy (multiple instances can be infeasible). **Not a monthly build.** Flag as the v2/v3 ceiling, not the MVP.

---

## Recommended architecture (v1)

A **single-band, three-stage series topology**: **saturator** (tube/tape/transformer via `aanl.lib`, bias + dry/wet + DC blocker) → **clipper** (ADAA, hard↔soft knee morph) → **lookahead limiter** (`limiter_lad` derivative or lamb-style smoothing, with true-peak detection). Ship with a **user oversampling selector** and an **ADAA-order toggle** so users pick their own CPU/fidelity point. This is genuinely differentiated — a true-peak-aware clip→limit chain with ADAA throughout is cleaner than much of the market, and achievable because the hard parts are library primitives.

---

## Open questions (resolve empirically, not on paper)

1. **Delay alignment** across ADAA + oversampling + lookahead + true-peak → one correct reported host latency. *Highest risk of a subtly-wrong/phasey plugin.*
2. **ADAA under a morphing knee** — closed-form antiderivatives per shape, or lookup-table ADAA?
3. **Oversampling placement** — nonlinear stages only, but the true-peak detector also wants oversampling; resolve the tension.
4. **Primitive trust** — verify each Faust primitive in *our* faust2api→JUCE path (double precision, version quirks, host stability).
5. **Binding layer** — faust2api C++ into Pamplejuce, and where latency is reported to the JUCE host (this is where #1 surfaces).

## Verification requirement

"Detail-preserving" is unfalsifiable without measurement. Build the rig in the first milestone: **null test, aliasing (swept-sine spectrogram / worst-case alias level), THD+N vs frequency, matched-loudness A/B**, each with explicit pass thresholds. Every later fidelity claim is checked against it.
