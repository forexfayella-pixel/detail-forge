---
title: Testing — Verification Rig
type: reference
tags: [reference, testing, dsp]
status: living
updated: 2026-07-02
---

# Testing — the Verification Rig

**"Detail-preserving" is unfalsifiable without measurement.** Build this rig in the first milestone; every later fidelity claim is checked against it. Each measure needs an **explicit pass threshold**.

## Measurements
1. **Null test** — process vs bypass/ref; residual should be only the intended nonlinear energy. Catches accidental gain/phase/latency errors. *Pass: null residual ≤ threshold on a linear passthrough config.*
2. **Aliasing** — swept-sine spectrogram / worst-case alias level. Compare ADAA vs oversampling-only vs off. *Pass: worst-case alias ≤ target dB below fundamental; target: 2nd-order ADAA ≈ 6x oversampling.*
3. **THD+N vs frequency** — harmonic + noise distortion across the band at fixed drive levels. *Pass: within target curve per mode.*
4. **Matched-loudness A/B** — loudness-matched before/after so "better" isn't just "louder." *Pass: subjective A/B holds at matched LUFS.*

## Plugin validation
- **pluginval** must pass before any build is "done" (audio-vst rule). → [[41-Build-Runbook]].
- Unit tests via the **Catch2** Tests target.

## Latency correctness
- Verify the host-reported latency matches the true group delay of ADAA + oversampling + lookahead + true-peak (via null test alignment). This is the [[53-Known-Issues|highest risk]].

## Related
[[64-Concepts-ADAA-and-TruePeak]] · [[15-DSP-Signal-Chain]] · [[53-Known-Issues]]

Up: [[60-Reference-MOC]]
