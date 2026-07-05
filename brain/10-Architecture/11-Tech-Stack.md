---
title: Tech Stack
type: reference
tags: [architecture, reference, dsp, faust, juce]
status: living
updated: 2026-07-02
---

# Tech Stack

## Layers
- **DSP:** Faust (`.dsp`) → compiled to C++ via **faust2api**. Parameter paths are the single source of truth. Libraries: `aanl.lib` (anti-aliased nonlinearities), `compressors.lib` (`limiter_lad_N`, `limiter_lad_bw`). See [[61-Faust-Libraries]].
- **Plugin chassis:** Pamplejuce-style **JUCE 8 + CMake** project. JUCE pulled in by the template (submodule/CPM).
- **GUI:** JUCE 8 **WebView** — HTML/CSS/JS in `Source/ui/`, bound via `WebSliderRelay` / `getSliderState`.
- **Formats:** VST3 (+ AU/CLAP as the chassis allows).

## Installed toolchain (verified 2026-07-02)
| Tool | Version | Notes |
|------|---------|-------|
| cmake | 4.3.3 | ≥ 3.25 required ✓ |
| MSVC | VS Build Tools 2026 (18.4) | VC.Tools.x86.x64 present |
| Faust | 2.85.9 | ≥ 2.72.14 → double precision OK for lamb ✓ |
| faust2api | present | Faust → C++ engine generator |
| node | v26.3.0 | WebView UI build |
| git | 2.40.0 | — |

## Precision
Build in **double precision** — required by lamb-style limiters. Faust 2.85.9 supports it.

## Related
[[15-DSP-Signal-Chain]] · [[61-Faust-Libraries]] · [[41-Build-Runbook]]

Up: [[10-Architecture-MOC]]
