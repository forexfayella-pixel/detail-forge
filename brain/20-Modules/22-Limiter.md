---
title: 22 · True-Peak Limiter
type: module
tags: [module, dsp, limiter]
status: living
updated: 2026-07-07
---

# True-Peak Limiter

## Purpose
Stage 3 of the [[15-DSP-Signal-Chain|saturator → clipper → limiter]] chain: a transparent, true-peak-aware **lookahead brickwall limiter** that enforces a guaranteed inter-sample output ceiling while the clipper keeps transient punch. Hand-written C++ (not Faust, not lamb — see [[52-Decision-Log]]). Landed 2026-07-07 (v0.7); the [[25-WebView-GUI|GR meter]] went live with it.

## Key files
- `Source/dsp/Limiter.h` — `TruePeakLimiter`, header-only, **JUCE-free** (unit-testable offline). Base-rate, stereo-linked.
- `Source/PluginProcessor.cpp` — instantiates `limiter` (member), `prepare()`s it, runs it in `processBlock` **after out-gain** (final stage), folds its lookahead into the reported latency, and stores `getGainReductionDb()` into `grDb`.
- `Source/PluginProcessor.h` — the `TruePeakLimiter limiter;` member + `lim_*` raw param pointers.

## Data flow
`in → sat → clip (OS region) → DC blocker → out-gain → **limiter** → scope/meter taps`. Per block: read `lim_threshold / lim_ceiling / lim_release / lim_lookahead` from APVTS → `limiter.setParams(...)` → `limiter.process(ch, numCh, n)` in-place → `grDb.store(limiter.getGainReductionDb())`. Gated on `lim_on`.

## How it works
- **Threshold = drive.** Peaks are pulled down to `threshold`, then an auto-makeup of `ceiling/threshold` brings the result to the ceiling → lower threshold = louder.
- **Guaranteed ceiling.** A **sliding-minimum of the required gain** over the lookahead window (monotonic deque, `pushMin`) is applied to audio **delayed** by that window — the gain is already down when the peak lands, so no overshoot. One-pole handles **release**; the lookahead window itself is the attack ramp (so there is no `lim_attack` param).
- **True-peak detection.** Internal **8× polyphase** windowed-sinc detector (`buildDetector`) finds the inter-sample peak, **linked** across channels (max-abs) → one GR curve → no image shift.

## Config / params
| Param | Range | Default | Role |
|---|---|---|---|
| `lim_threshold` | −24…0 dB | −6 | GR onset / drive |
| `lim_ceiling` | −12…0 dB | −1 | true-peak output bound |
| `lim_release` | 1…500 ms | 60 | recovery time |
| `lim_lookahead` | 0…12 ms | 1.5 | window length; adds to reported latency |
| `lim_on` | bool | true | section enable ([[21-Clip-Engine|like sat/clip]]) |

> [!warning] RT-safety — never re-init on the audio thread
> `prepare()` **preallocates** all buffers to the 12 ms maximum. `setParams()` (called every block) only changes the **active window length `L`**, never resizes — so automating lookahead cannot allocate on the audio thread. If you ever make the buffers grow with the live lookahead, you reintroduce a realtime heap allocation. Same discipline as the [[21-Clip-Engine|clip engine]].

> [!bug] True-peak detector must be ≥ 8× near Nyquist
> A 4×/8-tap detector **under-reads** inter-sample peaks for content near Nyquist — a 21 kHz test overshot the ceiling to −0.62 dB. Fixed with **8×/12-tap** detection **plus a −0.2 dB internal safety margin** (output sits at −1.2 dB for a −1.0 ceiling). Don't drop the margin or the oversampling. → [[53-Known-Issues]].

## Gotchas / open issues
- **[OPEN, 1.0]** Regression is scratch-only (`limiter_check`, 4 cases passed) — not yet a committed `Tests/limiter_runner.cpp` + ctest gate. → [[53-Known-Issues]].
- **[OPEN, 1.0]** In-DAW latency null across OS × lookahead not yet confirmed (the [[53-Known-Issues|[HIGH] unified-latency]] risk).
- Lookahead / `lim_on` changes retrigger `setLatencySamples` from `processBlock` (rare; acceptable, same pattern as OS-factor changes).

## Related
[[21-Clip-Engine]] · [[15-DSP-Signal-Chain]] · [[52-Decision-Log]] · [[53-Known-Issues]] · [[25-WebView-GUI]]

Up: [[20-Modules-MOC]]
