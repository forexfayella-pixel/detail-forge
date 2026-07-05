---
title: Build Runbook
type: runbook
tags: [ops, runbook, dsp]
status: snapshot
updated: 2026-07-02
---

# Build Runbook

> **Pending detail.** The user has a dedicated build runbook at `C:\Users\Fayella\Downloads\plugin-build-runbook.md` to consult **when staging the plugin build**. Fold its exact steps into this note at that point (and bump `updated:`). Until then, the audio-vst profile defaults below apply.

## Default flow (audio-vst profile)
1. **Faust → C++:** generate the engine from `.dsp` with **faust2api** (double precision). Faust param paths are the parameter source of truth.
2. **Configure:** `cmake --preset default`
3. **Build:** `cmake --build --preset default`
4. **Test:** build + run the Tests target (Catch2).
5. **Validate:** run **pluginval** against the built plugin. Not "done" until it passes.

## Toolchain (verified 2026-07-02)
cmake 4.3.3 · MSVC VS Build Tools 2026 (18.4) · Faust 2.85.9 · faust2api · node v26.3.0. See [[11-Tech-Stack]].

## Watch-outs
- **WebView member order** (relays → browser → attachments) or crash-on-unload. See [[53-Known-Issues]].
- **Latency reporting** must be unified across ADAA/OS/lookahead/true-peak — verify what's reported to the host. See [[15-DSP-Signal-Chain]].
- CI: run tests/pluginval on **Linux** (GitHub bills Windows 2x, macOS 10x on private repos).

Up: [[40-Operations-MOC]]
