---
title: Reference Repos
type: reference
tags: [reference, dsp, faust]
status: snapshot
updated: 2026-07-02
---

# Reference Repos

> Read these before writing DSP. `status: snapshot` — **`/research` should confirm each is live** and note current commit/version.

## magnetophon/lamb (Bart Brouns)
- Faust lookahead limiter by the author of the std-lib limiter functions. Uses **Dario Sanfilippo's sine-shaped gain smoothing** (gain never abruptly changes direction → smooth even at short times).
- **Directly relevant:** at minimum attack/release/hold it **becomes a waveshaper/clipper** — it already embodies the clipper↔limiter continuum we're building.
- Constraints: needs **double precision (Faust 2.72.14+)**; **no true-peak yet** → the gap we fill. → [[53-Known-Issues]].

## magnetophon/LazyLimiter
- Brute-force, very low distortion (zero on sines down to low bass); heavy CPU; ~186 ms latency. Reference for the **fidelity ceiling**, not a shipping mode → [[34-Do-Not-Start-Yet]].

## jatinchowdhury18 ADAA repo
- C++/JUCE; clearest practical **ADAA reference**. Good model for a **mode-switch UI** (standard / 1st / 2nd / oversampling) to mirror in our GUI.

## Related
[[64-Concepts-ADAA-and-TruePeak]] · [[61-Faust-Libraries]] · [[15-DSP-Signal-Chain]]

Up: [[60-Reference-MOC]]
