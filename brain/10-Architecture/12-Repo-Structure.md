---
title: Repo Structure
type: snapshot
tags: [architecture, reference]
status: snapshot
updated: 2026-07-02
---

# Repo Structure

> `status: snapshot` — reflects the pre-code scaffold. Update when the JUCE/Faust chassis is generated.

## Current (pre-code)
```
projects/detail-forge/
├── CLAUDE.md          # project manual (workspace summary + audio-vst profile + brain block)
├── brain/             # this vault
├── docs/              # research notes (holds the seed)
├── specs/             # specs (empty)
└── plans/             # plans (empty)
```

## Planned (after chassis scaffold) — expected Pamplejuce-style shape
```
projects/detail-forge/
├── CMakeLists.txt
├── CMakePresets.json
├── Source/
│   ├── PluginProcessor.{h,cpp}   # JUCE processor; hosts the Faust engine; reports latency
│   ├── PluginEditor.{h,cpp}      # WebView editor (member order: relays, browser, attachments)
│   ├── dsp/                      # Faust .dsp sources + generated C++ engine
│   └── ui/                       # HTML/CSS/JS WebView GUI
├── Tests/                        # Catch2 target
└── libs/ (or CPM cache)          # JUCE, etc.
```
Update the tree here (and bump `updated:`) once the chassis exists. Add per-subsystem detail in [[20-Modules-MOC]].

Up: [[10-Architecture-MOC]]
