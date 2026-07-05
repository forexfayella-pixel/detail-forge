---
title: Accurate waveform + applied-effect visualization (research)
date: 2026-07-03
type: research
tags: [ui, webview, visualization, true-peak, accuracy]
status: findings
---

# Accurate waveform + applied-effect visualization

**Goal.** Make the JUCE 8 WebView display *tell the truth* about the DSP — especially
inter-sample / true-peak behaviour, which is the exact thing this plugin's clip→limit
chain exists to manage. The display must reconstruct the band-limited waveform (peaks
overshoot between samples), overlay dry vs wet with correct latency alignment, draw the
*actual* transfer curve the DSP uses, and show what got clipped. Accuracy over aesthetics.

---

## Reference — what "truthful" looks like in the field

- **"Detail Forge" is our own working title** — no shipping product by that name (verified: zero
  vendor/KVR/retailer hits). Anchor reference is **Newfangled Audio *Saturate*** (Eventide) — the
  *Elevate* spectral-clipper algorithm with a **Detail Preservation** control.
  - **Saturation Curve Display**: a transfer curve with the **live signal riding it**; the SHAPE
    control morphs hard-clip ↔ "mathematically smoothest curve" and the display redraws. Detail
    Preservation = frequency-independent processing (not a separate graph). v1.7.0 added a GR meter
    + scrolling waveform. (newfangledaudio.com/saturate; /post/saturate-1-7-0)
- **StandardCLIP (SIR Audio Tools)**: transfer plot + waveform + I/O RMS; a **red dot marks the
  current peak on the transfer curve**; a **Gain-Difference meter** shows RMS(red)/peak(green)
  difference between input and oversampled-clipped output = *what was removed*. OS up to 256×.
  (siraudiotools.com/StandardCLIP.php)
- **KClip 3 (Kazrog)**: waveform **histogram** — processed signal along the bottom, **clipped /
  reduced signal along the top**, dB scale; a **Delta** toggle to hear only the clipped-away audio;
  true-peak + LUFS jumbo meters; OpenGL up to 120 fps. (kazrog.com/products/kclip-3)
- **FabFilter Pro-L 2** — gold standard for truth: the output meter shows **inter-sample (true)
  peak** (ITU-R BS.1770 / EBU R128); **metering runs on the oversampled path so displayed peaks ==
  true peaks**; green → orange (over set level) → red (>0 dBTP). Internal OS 1×–32×.
  (fabfilter.com/help/pro-l/using/truepeaklimiting, /metering)

**Field gap = our opportunity.** Two idioms dominate: (a) transfer curve with live signal
(Saturate, StandardCLIP); (b) waveform + a *separate* clipped-portion trace + a delta listen mode
(KClip 3, bx_clipper "Ambience"). **Almost nobody highlights, per-sample on the reconstructed
true-peak waveform, exactly which tips got clipped.** That is both our differentiator and dead on
mission (true-peak truth).

---

## What already exists in our code (relevant, with paths)

**DSP latency sources — the alignment truth:**
- ADAA2 hard clip = **1 sample** (`kLatencySamples`, `Source/PluginProcessor.h`).
- Oversampling group delay = `oversamplers[..].getLatencyInSamples()` (tens of samples at 4×),
  reported to host via `setLatencySamples`; tracked in `lastReportedLatency`
  (`PluginProcessor.cpp:114-117, 257, 277-280`).
- Saturator runs **inside** the OS region (`runSat` before `runClip`) → adds no latency beyond OS.
- **Net internal latency between the pre-tap and post-tap = `lastReportedLatency` (call it L).**

**Scope capture (`PluginProcessor.cpp`):** `inRing`/`outRing` (`kScopeCap=8192`), `scopeWrite`
atomic. Input ch0 captured **pre-gain at index `w`** (`:187-192`); output ch0 captured
post-processing at the **same index `w`** (`:283-290`); `readScope()` copies the last N
(`:300-305`). Meters use `buffer.getMagnitude()` = **sample** peak (`:196, 292`); `grDb` hardcoded
`0` (`:209, 295` — limiter unbuilt).

**Editor data path (`PluginEditor.cpp`):** 30 Hz `Timer`; emits `"meters"` and `"wave"` (N=1024,
base64 of 2N floats via `Base64::toBase64`) with `emitEventIfBrowserIsVisible` (`:82-105`).
Resource provider serves `index.html` + JUCE JS. Member order relays → webView → attachments
(crash rule respected, `:27-63`).

**UI (`Source/ui/index.html`, Canvas 2D):** `drawScope` uses straight `lineTo` between the 1024
samples + autocorrelation/zero-cross trigger. `drawTransfer` **re-derives the clip curve in JS from
params** (hard = clamp, soft = `1.5u−0.5u³`, morph by knee) — *not* probed from Faust and ignores
the saturator. `drawSpectrum` = FFT of output. `prep()` handles DPR (capped at 2).

### The four ways the current display lies
1. **Latency smear.** In and out are written at the *same* ring index, but out is delayed by L →
   the pre/post overlay is misaligned by L. Fix: delay the pre-tap by L before storing so both
   rings align at one index.
2. **No band-limited reconstruction.** Straight lines between 1024 decimated samples hide
   inter-sample peaks and under-read true peak — the plugin's entire thesis, invisible.
3. **Transfer curve is a JS approximation, not the DSP.** It diverges from the real Faust
   `hardclip2`↔`softclipQuadratic2` ADAA morph and omits the saturator stage entirely.
4. **Meters show sample peak, not true peak; GR is always 0.**

---

## Constraints
- JUCE 8 WebView (HTML/Canvas/JS) + Faust DSP.
- **Never** allocate / lock / serialize on the audio thread.
- Respect reported latency (host PDC); the internal overlay must compensate by L itself — PDC
  aligns host-level dry/wet, not our two internal taps.
- WebView member-declaration-order crash rule: relays → browser → attachments.
- `emitEvent` marshals `juce::var`→JSON (slow for big arrays) → **base64-pack** float buffers.
- WebView2 throttles/suspends when hidden (already guarded by `emitEventIfBrowserIsVisible`).

## Unknowns
- Exact Faust ADAA transfer **under a morphing knee** (closed-form vs LUT) — already in
  [[53-Known-Issues]]. **Probing the live engine sidesteps this entirely.**
- Whether 4× display-oversampling suffices or we want 8× for the drawn curve.
- Whether OffscreenCanvas/worker is needed (measure first).
- End-user WebView2 runtime version (pin Evergreen).

## Techniques the research settled (apply these)
- **True peak (BS.1770/R128):** attenuate → **≥4× upsample** (polyphase LPF) → `abs` → dB. In
  **float the −12.04 dB pad is unnecessary** and dropped. 4× is the compliance floor (under-reads
  ≤~0.6 dB); 8× tightens it.
- **Drawn curve reconstruction:** **Lanczos-3** (`a=3`) at **≥8×**, `L(x)=sinc(x)·sinc(x/a)`,
  precomputed as an N×2a polyphase coefficient table.
- **Zoomed out:** **per-pixel min/max bars** (BBC peaks.js idiom) — never hides a peak. **Zoomed
  in (<1 sample/px):** sample stems **plus** the Lanczos-interpolated curve.
- **DPR:** `canvas.width = css*dpr; ctx.scale(dpr,dpr)`, redraw on resize/DPR change.
- **dB amplitude warp** applied only at the pixel-mapping step; keep peak math in **linear**.
- **No JS library does band-limited reconstruction** (wavesurfer.js / BBC peaks.js are min/max
  envelopes only) → we implement the interpolation ourselves.
- **Transport:** `juce::AbstractFifo` (wait-free, decimated min/max columns) → 30–60 Hz timer
  drains on the message thread → base64 → `emitEvent` (or `withNativeFunction` pulled on rAF) →
  `atob` → `Float32Array` → Canvas 2D. Never JSON-encode raw sample arrays; never serialize on the
  audio thread.

---

## Candidate architectures

### A — Latency-aligned Canvas 2D scope + **probed** transfer (minimal, accurate)
Keep the base64/Canvas path. (1) Delay the pre-tap by L so rings align. (2) Do true-peak
reconstruction + min/max in JS on the received window (or ship pre-oversampled peaks). (3) Replace
the JS transfer approximation by **probing the DSP**: on (re)prepare / param change, run a −1…+1
ramp (~512 pts) through the *actual* sat+clip path on the message thread and send the resulting
curve to JS — truthful by construction, and it dodges the morphing-knee unknown. (4) True-peak
meters; wire GR when the limiter lands.
*Pros:* smallest diff, reuses everything, ships fast, fully accurate for the 1-D scope + curve.
*Cons:* Canvas 2D ceiling; transfer probe needs a small offline-eval helper.

### B — WebGL waveform + curve overlay (max fidelity)
Dedicated WebGL renderer: reconstructed waveform as a vertex buffer, clipped-region highlight as a
second pass, transfer curve + live dot, glow, 60 fps at thousands of vertices.
*Pros:* headroom for stacked scopes and the per-sample "what-got-clipped" highlight at high
density. *Cons:* much more code; WebGL-in-WebView2 risk; overkill now; slower to ship.

### C — Hybrid: A's data path + clipped-region highlight, structured for later WebGL
Architecture A's accuracy and transport, but hide the renderer behind a draw interface and move
base64-decode + min/max into an **OffscreenCanvas worker** *if* the main thread janks. Add the
**clipped-region highlight** (aligned pre vs post, mark where |pre| would exceed ceiling but out is
bounded) as a Canvas overlay now.
*Pros:* accurate + shippable now, clean WebGL upgrade path, gets the differentiator cheaply.
*Cons:* a little more plumbing than A.

## Recommendation for v1 — **A, plus the clipped-region highlight from C**
Accuracy is the priority and A delivers all of it — latency-aligned overlay, band-limited
reconstruction, probed transfer, true-peak meter — with the least risk inside the WebView/Faust
constraints. Canvas 2D comfortably handles a couple thousand segments at 60 fps; defer WebGL until
we actually stack scopes or measure frame drops. Add the clipped-tip highlight because it's cheap
once pre/post are aligned and it's the on-mission differentiator no competitor ships.

**Concrete v1 changes**
1. **Processor:** add an **L-sample delay line on the pre-tap** so `inRing`/`outRing` align
   (L = `lastReportedLatency`, updates with the OS factor). Compute **true peak** (4× oversampled
   abs) for the meters. (Optional: migrate the ring to `AbstractFifo` of decimated min/max columns;
   the current N=1024 ring is adequate for now.)
2. **Transfer probe:** message-thread helper that runs a ramp through the real sat+clip and ships
   ~512 (x, y) points on param change → JS draws the true curve + a live "you-are-here" dot from
   the current output peak.
3. **JS:** replace straight `lineTo` with **Lanczos-≥8× reconstruction** (or receive
   pre-oversampled peaks); **min/max per pixel** when zoomed out; **dB amplitude** option; keep DPR
   handling; draw the **clipped-region highlight** where the aligned input tip exceeds ceiling but
   the output is bounded (the removed tips).

---

## Accuracy-verification method (must-do before calling it done)
1. **Inter-sample peak test.** Synthesize a signal engineered to have an inter-sample peak (e.g. a
   sine near `fs/4` phase-offset so samples straddle the crest; sample peak reads *below* true
   peak). Confirm the drawn reconstructed curve rises to the true peak (visibly above the sample
   dots) and the TP meter matches an independent BS.1770 4× computation (cross-check `scripts/` /
   a reference true-peak meter).
2. **Pre/post alignment test.** Send a single **impulse**. With the L-delay applied, the pre and
   post impulses must land on the **same x**. Toggle the oversampling factor (changes L) and
   confirm they stay aligned — validates dynamic L.
3. **Transfer-probe test.** Feed a slow full-scale **ramp**; the plotted output-vs-input must trace
   exactly onto the probed curve, and Knee/Drive/Ceiling/sat must bend both identically. Spot-check
   a few probed points against `Tests/engine_runner` output for the same params.
4. **dB / DPR.** Verify −48 dB detail is visible in dB mode and the waveform is crisp (no blur) at
   125/150/200% display scaling.

## Sources (key)
- ITU-R BS.1770-5; Essentia TruePeakDetector; Lanczos resampling (Wikipedia).
- FabFilter Pro-L 2 true-peak limiting / metering / oversampling docs.
- Newfangled Saturate (product + 1.7.0 post); SIR StandardCLIP; Kazrog KClip 3 manual.
- BBC peaks.js / waveform-data.js; wavesurfer.js docs (min/max envelope, no band-limiting).
- JUCE 8 WebView blog; `WebBrowserComponent::Options`, `WebSliderRelay`, `AbstractFifo`,
  `AudioProcessor` docs; forum threads: emitEvent bottleneck (#66748), lock-free visualization
  (#20659), WebView2 minimize reload (#67954), setLatency PDC (#15936).

## Related
[[15-DSP-Signal-Chain]] · [[25-WebView-GUI]] · [[21-Clip-Engine]] · [[53-Known-Issues]]
