---
title: Architecture MOC
type: moc
tags: [moc, architecture]
status: living
updated: 2026-07-02
---

# 🏗️ Architecture MOC

How detail-forge is put together.

## Leaves
- [[11-Tech-Stack]] — languages, libraries, toolchain, versions
- [[12-Repo-Structure]] — directory layout (grows as scaffolded)
- [[15-DSP-Signal-Chain]] — the saturator → clipper → limiter topology (the heart of the plugin)

## The shape in one line
Faust `.dsp` DSP → faust2api → C++ engine embedded in a Pamplejuce-style JUCE 8 plugin; JUCE 8 WebView (HTML/CSS/JS) GUI bound to Faust parameters.

Up: [[00-Home]]
