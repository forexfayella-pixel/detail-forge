---
title: Concepts — ADAA, Clipper vs Limiter, Saturation
type: reference
tags: [reference, dsp]
status: stable
updated: 2026-07-02
---

# Concepts — the DSP theory behind detail-forge

"Detail-preserving" reduces to **two levers**: (1) suppress aliasing, (2) choose the right peak-handling behaviour for the material. Everything below serves those.

## 1. Anti-aliasing (the core engine)
Every nonlinearity (clipper corner, saturator curve, hard ceiling) generates harmonics above Nyquist that fold back as inharmonic energy → the "harsh/brittle/digital/smeared" sound. Killing it is the single biggest fidelity lever. Two techniques, best combined:
- **Oversampling** — run the nonlinearity at 4x/8x/16x, filter, downsample. Works but multiplies CPU by the factor, and downsampling can nudge peaks / eat headroom (bad for a peak-control plugin).
- **ADAA (antiderivative anti-aliasing)** — Parker/Zavalishin/Le Bivic, DAFx 2016 (Native Instruments). Apply the antiderivative of `f(x)` and take a discrete difference; aliasing is suppressed at the source. Caveats:
  - **1st-order is ill-conditioned** when consecutive samples are near-equal (divide-by-near-zero) → needs a tolerance/fallback branch.
  - **2nd-order** suppresses more, needs more fallback branches.
  - **tanh** specifically: 2nd antiderivative needs the **dilogarithm** → some libs offer tanh ADAA at 1st order only.
- **Key finding:** ADAA + modest oversampling beats heavy oversampling alone at a fraction of the CPU — **2nd-order ADAA ≈ 6x oversampling** for alias suppression. This combination is the detail-preservation core.

## 2. Clipper vs Limiter (opposite ways to preserve detail)
- **Clipper** = per-sample, no attack/release. Only affects samples above the ceiling → preserves transient punch. Distortion is mostly *inaudible on fast transients*, audible on sustained/tonal content. So clipping drum transients is nearly free; clipping tonal content colours it.
- **Limiter** = time-based gain reduction (attack/release) → pulls down surrounding audio with the peak; "squashed" when pushed, but transparent for tonal overshoots.
- **Pro topology:** clip the tips (1–3 dB of the tallest transients), then limit the rest → more punch than either alone. (Tokyo Dawn framing: limiter optimal for tonal overshoot; clipper optimal for transients in noisy spectra.)
- **True/inter-sample peaks:** the reconstructed waveform can exceed any sample value; detect via oversampling/interpolation. Default ceiling **−1.0 dBFS**, **−1.2** for strict true-peak delivery.

## 3. Saturation (the transfer function is the whole game)
- **Symmetric curve (`tanh(g·x)`) → odd harmonics only.** Asymmetry (curve or input) unlocks **even harmonics**.
- Archetypes: **tube** = asymmetric soft clip, a **bias** param sets even/odd ratio; **tape** = symmetric tanh sigmoid; **transformer** = squared/polynomial with both terms, usually a low-shelf before the shaper.
- Must-dos: **DC blocker after** any asymmetric/biased shaping (bias = y-axis slide = DC offset). **Chebyshev polynomials** dial exactly which harmonics appear.
- Voicing rule: low drive enhances, high drive masks detail + adds mud; the warm↔muddy gap is often **2–3 dB of drive**. A **dry/wet (parallel) Mix** is therefore a genuine detail-preservation feature.

## The perceptual frontier (north star, out of MVP)
**Newfangled Elevate** splits into up to **26 critical/Mel bands**, limits each independently (adaptive gain/speed/transient per band) with a spectral clipper at the end — *why* it stays transparent when pushed. Extremely CPU-heavy → [[34-Do-Not-Start-Yet]].

## Related
[[15-DSP-Signal-Chain]] · [[61-Faust-Libraries]] · [[63-Reference-Repos]] · [[62-Testing]]

Up: [[60-Reference-MOC]]
