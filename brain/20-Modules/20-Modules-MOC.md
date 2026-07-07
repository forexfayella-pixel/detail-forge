---
title: Modules MOC
type: moc
tags: [moc, module]
status: living
updated: 2026-07-07
---

# 🧩 Modules MOC

One deep note per subsystem. Create each from [[Module-Note-Template]] as it's built. Mapped to [[15-DSP-Signal-Chain]]:

| Note | Subsystem | Status |
|--------------|-----------|--------|
| [[21-Clip-Engine]] | ADAA2 clip + Drive + detail fold-back | **built** |
| [[22-Limiter]] | Hand-written true-peak lookahead brickwall limiter | **built (2026-07-07)** |
| `23-Saturator` | C++ waveshaper (tube/tape/xfmr, bias, dry/wet, DC blocker) — currently `runSat` in the processor, not yet its own note | **built, note pending** |
| `24-Oversampling-and-Latency` | oversampling + true-peak detector + unified latency reporting | note pending |
| [[25-WebView-GUI]] | JUCE 8 WebView UI, all params bound via relays | **built (2026-07-03)** |

> Create a note when the subsystem gets real code — not before. Each links back here and to its architecture/reference notes.

Up: [[00-Home]]
