---
title: Glossary
type: reference
tags: [meta, reference, dsp]
status: living
updated: 2026-07-02
---

# Glossary

Domain terms and easily-confused names for detail-forge.

- **ADAA** — Antiderivative Anti-Aliasing. Apply a nonlinearity's antiderivative and take a discrete difference to suppress aliasing at the source. 1st-order and 2nd-order variants. See [[64-Concepts-ADAA-and-TruePeak]].
- **Aliasing** — harmonics generated above Nyquist by a nonlinearity folding back into the audible band as inharmonic energy; the main cause of "harsh/brittle/digital" sound.
- **Oversampling** — running a nonlinear stage at 4x/8x/16x sample rate, then filtering + downsampling to reduce aliasing. Costs CPU proportional to the factor.
- **Clipper** — per-sample ceiling. No attack/release; only affects samples above the ceiling. Preserves transient punch, adds distortion. Contrast **Limiter**.
- **Limiter** — time-based (attack/release) gain reduction that pulls down surrounding audio with the peak. Transparent for tonal overshoots; "squashes" when pushed.
- **True peak / inter-sample peak** — the reconstructed analog waveform can exceed any digital sample value; detected via oversampling/interpolation. Default ceiling ≈ −1.0 dBFS (−1.2 for strict delivery).
- **Saturation** — nonlinear transfer function adding harmonics. Symmetric curve → odd harmonics; asymmetry/bias → even harmonics.
- **Bias** — vertical offset of a saturation curve; introduces even harmonics and a DC offset (needs a DC blocker after).
- **DC blocker** — high-pass at ~DC that removes the offset introduced by asymmetric/biased shaping.
- **Faust** — functional DSP language; `.dsp` files compiled to C++ via **faust2api**. Parameter paths are the single source of truth for the plugin's parameter list.
- **aanl.lib** — Faust anti-aliased-nonlinearities library (ADAA saturators/clippers). See [[61-Faust-Libraries]].
- **lamb** — magnetophon's Faust lookahead limiter (sine-shaped gain smoothing); collapses to a waveshaper/clipper at min times. See [[63-Reference-Repos]].
- **pluginval** — plugin validation tool; a build isn't "done" until it passes.
- **THD+N** — Total Harmonic Distortion + Noise; a fidelity measurement in the verification rig ([[62-Testing]]).

Up: [[00-Home]]
