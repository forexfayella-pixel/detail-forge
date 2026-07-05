---
title: Do Not Start Yet
type: log
tags: [roadmap]
status: living
updated: 2026-07-02
---

# Do Not Start Yet (gated / deferred)

- **Multiband / per-band limiting (Newfangled Elevate-style)** — up to 26 critical/Mel bands limited independently with a spectral clipper. This is the perceptual north star and *why* Elevate stays transparent when pushed, but it's extremely CPU-heavy (multiple instances can be infeasible). **v2/v3 ceiling, not the MVP.** Do not build until the single-band chain is measured and shipped.
- **AU/AAX packaging** — after VST3 is validated.
- **LazyLimiter-grade brute-force fidelity** (~186 ms latency) — reference for the fidelity ceiling, not a shipping mode.
- **LUT-based ADAA for arbitrary morphing knees** — only if closed-form per-shape antiderivatives prove impractical (see [[53-Known-Issues]]).

Up: [[30-Roadmap-MOC]]
