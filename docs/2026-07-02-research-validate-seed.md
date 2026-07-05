# Research: Validate & Extend the Detail-Preserving Dynamics Seed

**Date:** 2026-07-02
**Stage:** Research (validates `docs/2026-07-02-detail-preserving-dynamics-seed.md`)
**Goal:** Confirm the seed's Faust primitives against the *installed* Faust 2.85.9 stdlib, and confirm the reference repos are live. Extend with concrete signatures.

## Method
- Inspected the local stdlib at `C:\Program Files\Faust\share\faust\` (`aanl.lib` v1.4.2, `compressors.lib` v1.6.0).
- Web-confirmed the reference repos.

## Findings — `aanl.lib` (v1.4.2) — CONFIRMED + concrete signatures
The seed's ADAA claims are accurate. Real signatures:
- **`ADAA1(EPS, f, F1, x)`** and **`ADAA2(EPS, f, F1, F2, x)`** — the antiderivative AA primitives. `EPS` is the ill-conditioning tolerance the seed warned about (near-equal consecutive samples → fallback branch). `f` = function, `F1`/`F2` = 1st/2nd antiderivatives.
- **Hard clip:** `hardclip(x)` (ADAA1) and **`hardclip2(x)` (ADAA2)** — the "worked ADAA2 hard-clip example" the seed referenced.
- **Clipper knee-morph palette (all ADAA):** `hardclip`/`hardclip2`, `cubic1`, `parabolic`/`parabolic2`, `hyperbolic`/`hyperbolic2`, `sinarctan`/`sinarctan2`, `softclipQuadratic1`/`softclipQuadratic2`. → gives us a hard↔soft morph from real primitives instead of hand-derived antiderivatives.
- **tanh caveat CONFIRMED exactly:** only **`tanh1`** exists (ADAA1); there is **no `tanh2`** — matches the seed's dilogarithm note. If we want tanh-flavoured saturation at 2nd order, use `asinh2`/`arctan2`/`hyperbolic2` as sigmoid substitutes.
- Saturator sigmoids available: `tanh1`, `arctan`/`arctan2`, `asinh1`/`asinh2`, `sine`/`sine2`, `cosine1`/`cosine2`, plus the trig/hyperbolic family.

## Findings — `compressors.lib` (v1.6.0) — CONFIRMED + concrete signatures
- **`limiter_lad_N(N, LD, ceiling, attack, hold, release)`** — arg order confirmed (N channels, `LD` = lookahead delay in seconds, `ceiling` linear, attack/hold/release in seconds). Body uses `ba.peakholder` + `att_smooth`/`rel_smooth`, scaling = `ceiling / max(amp_profile, EPS) : min(1)`.
- Wrappers: `limiter_lad_mono/stereo/quad(LD)`, and **`limiter_lad_bw`** (brickwall) = `limiter_lad_mono(.01, 1, 0.01/twopi, .1, 1/twopi)`.
- **`limiter_1176_R4_mono/_stereo`** = `compressor_mono(4,-6,0.0008,0.5)` — confirms the seed: it's the generic compressor with hardwired params, not a modelled 1176.
- Bonus: `RMS_FBcompressor_peak_limiter_N_chan(...)` exists — a feedback RMS comp+limiter, worth a look for the limiter stage character.

## Findings — reference repos (web-confirmed 2026-07-02)
- **magnetophon/lamb** — LIVE (215 commits, AGPL-3.0). Self-describes as "lookahead compressor/limiter soft as a lamb." **Needs double precision** (`-double` flag) ✓. Sine-shaped attack/release smoothing ✓. `release hold` adds latency. **No true-peak** ✓ → confirmed gap for us to fill. Users set `MaxSampleRate` manually above 48 kHz (note for our build).
- **jatinchowdhury18/ADAA** — LIVE (122★, C++/JUCE + Python, CMake). VST3/AU + standalone. Hard-clip, tanh, waveguide, **each with SIX variants: standard / ADAA1 / ADAA2 / LUT / LUT+ADAA1 / LUT+ADAA2**, plus 2x/4x/8x oversampling. → **directly answers open question #2**: LUT-ADAA is a proven approach for the morphing knee. Good model for our mode-switch UI.
- **magnetophon/LazyLimiter** — same author as lamb; not re-verified this pass (high confidence live). Keep as fidelity-ceiling reference only.

## What changed in the brain
- `61-Faust-Libraries` → upgraded from snapshot to confirmed: real signatures + lib versions + tanh1-only.
- `53-Known-Issues` → open question #2 (morphing knee) now has a concrete path (LUT-ADAA, per jatin repo).
- `51-Timeline` → dated bullet.

## Recommendation (unchanged direction, now grounded)
Proceed with the v1 single-band saturator → clipper → limiter. Build the clipper first on `hardclip2` + the ADAA knee-morph palette; limiter on `limiter_lad_N`; add true-peak ourselves (lamb has none). Stand up the [[62-Testing|verification rig]] before/alongside. No blockers surfaced.
