---
title: Faust Libraries
type: reference
tags: [reference, faust, dsp]
status: stable
updated: 2026-07-02
---

# Faust Libraries

> **CONFIRMED against installed Faust 2.85.9** (2026-07-02): `aanl.lib` v1.4.2, `compressors.lib` v1.6.0. Stdlib at `C:\Program Files\Faust\share\faust\`. Full validation: `docs/2026-07-02-research-validate-seed.md`.

## `aanl.lib` v1.4.2 — anti-aliased nonlinearities
- **ADAA primitives:** `ADAA1(EPS, f, F1, x)` and `ADAA2(EPS, f, F1, F2, x)`. `EPS` = ill-conditioning tolerance (near-equal samples → fallback); `f` = function, `F1`/`F2` = 1st/2nd antiderivatives.
- **Hard clip:** `hardclip(x)` (ADAA1), **`hardclip2(x)` (ADAA2)** — the pre-derived example the seed cited.
- **Clipper knee-morph palette (all ADAA):** `hardclip`/`hardclip2`, `cubic1`, `parabolic`/`parabolic2`, `hyperbolic`/`hyperbolic2`, `sinarctan`/`sinarctan2`, `softclipQuadratic1`/`softclipQuadratic2`. → real hard↔soft morph without hand-deriving antiderivatives.
- **Saturator sigmoids:** `tanh1`, `arctan`/`arctan2`, `asinh1`/`asinh2`, `sine`/`sine2`, `cosine1`/`cosine2`, plus trig/hyperbolic family.
- **tanh caveat CONFIRMED:** only `tanh1` exists — **no `tanh2`** (dilogarithm). For 2nd-order tanh-flavoured sat, substitute `asinh2`/`arctan2`/`hyperbolic2`.
- Designed to combine with low-factor oversampling (96/192 kHz). Maps to [[15-DSP-Signal-Chain]] stages 1–2.

## `compressors.lib` v1.6.0 — dynamics
- **`limiter_lad_N(N, LD, ceiling, attack, hold, release)`** — N channels, `LD` = lookahead delay (s), `ceiling` linear, times in s. Body: `ba.peakholder` + attack/release smoothing; `scaling = ceiling / max(amp_profile, EPS) : min(1)`.
- Wrappers: `limiter_lad_mono/stereo/quad(LD)`; **`limiter_lad_bw`** (brickwall) = `limiter_lad_mono(.01, 1, 0.01/twopi, .1, 1/twopi)`. Quality signal: pop song, 60 dB amp, 0-dB ceiling → loudest peak ~0.38 dB.
- **`limiter_1176_R4_mono/_stereo`** = `compressor_mono(4,-6,0.0008,0.5)` — confirmed: generic compressor with hardwired params, *not* a modelled 1176.
- Bonus: `RMS_FBcompressor_peak_limiter_N_chan(...)` — feedback RMS comp+limiter, candidate for limiter character. → stage 3.

## Related
[[15-DSP-Signal-Chain]] · [[63-Reference-Repos]] · [[11-Tech-Stack]]

Up: [[60-Reference-MOC]]
