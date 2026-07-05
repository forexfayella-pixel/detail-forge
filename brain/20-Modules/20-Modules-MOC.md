---
title: Modules MOC
type: moc
tags: [moc, module]
status: living
updated: 2026-07-02
---

# 🧩 Modules MOC

One deep note per subsystem. Create each from [[Module-Note-Template]] as it's built. Mapped to [[15-DSP-Signal-Chain]]:

| Note | Subsystem | Status |
|--------------|-----------|--------|
| [[21-Clip-Engine]] | ADAA2 hard-clip engine + Drive (M0) | **built (M0)** |
| `22-Saturator` | Faust saturator (tube/tape/transformer, bias, dry/wet, DC blocker) | not started (M3) |
| `23-Limiter` | Lookahead limiter (limiter_lad / lamb-style), true-peak | not started (M2) |
| `24-Oversampling-and-Latency` | oversampling + true-peak detector + unified latency reporting | not started (M1/M2) |
| [[25-WebView-GUI]] | JUCE 8 WebView UI, all params bound via relays | **built (2026-07-03)** |

> Create a note when the subsystem gets real code — not before. Each links back here and to its architecture/reference notes.

Up: [[00-Home]]
