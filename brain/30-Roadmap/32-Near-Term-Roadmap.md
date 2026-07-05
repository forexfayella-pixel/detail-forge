---
title: Near-Term Roadmap
type: log
tags: [roadmap]
status: living
updated: 2026-07-02
---

# Near-Term Roadmap

## Milestone 0 — Foundation & measurement
- Validate/extend seed research; confirm library signatures + live repos.
- Build the verification rig ([[62-Testing]]) with explicit pass thresholds.
- Scaffold JUCE 8 + CMake chassis; pass-through Faust engine; green build + pluginval.

## Milestone 1 — Clipper (the AA core)
- ADAA hard clip (1st + 2nd order) via `aanl.lib`; ADAA-order toggle.
- Optional oversampling selector around the nonlinearity.
- Measure: alias suppression vs oversampling-only; THD+N. Target: 2nd-order ADAA ≈ 6x oversampling for alias suppression.

## Milestone 2 — Saturator
- tube/tape/transformer modes; bias (even harmonics) + DC blocker; dry/wet Mix.

## Milestone 3 — Limiter + true-peak
- Lookahead limiter (`limiter_lad` derivative or lamb-style smoothing).
- True-peak detector; ceiling −1.0 dBFS (−1.2 strict).
- **Resolve unified latency reporting** across ADAA + OS + lookahead + true-peak.

## Milestone 4 — Integrate + voice + GUI
- Series topology wired; clip-the-tips-then-limit defaults.
- WebView GUI; parameter binding; preset(s).

See deferred items in [[34-Do-Not-Start-Yet]].

Up: [[30-Roadmap-MOC]]
