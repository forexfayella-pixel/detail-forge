---
title: 25 · WebView GUI
type: module
tags: [module, juce, ui]
status: stable
updated: 2026-07-04
---

# WebView GUI

## Purpose
The plugin's UI: a JUCE 8 WebView (WebView2 on Windows) rendering a self-contained HTML/CSS/JS control surface bound to the APVTS parameters. Rebuilt from a design-tool export into JUCE-native code.

## Key files
- `Source/ui/index.html` — the whole UI: SVG-arc knobs, segmented selectors, bypass, preset browser, canvas visualizers (oscilloscope/transfer/harmonics), meter bridge. Self-contained (no CDNs/web fonts). Imports the JUCE frontend via `./juce/index.js`.
- `Source/PluginEditor.{h,cpp}` — registers a relay per param (11 `WebSliderRelay`, 3 `WebComboBoxRelay`, 1 `WebToggleButtonRelay`) + attachments; pushes live meter levels to JS on a 30 Hz timer via `emitEventIfBrowserIsVisible("meters", …)`. Member order relays→browser→attachments.
- `CMakeLists.txt` — embeds `index.html` + JUCE's `index.js` + **`check_native_interop.js`** as binary data; WebView2 defines.

## Binding contract
- Knobs → `Juce.getSliderState(id)` (getNormalisedValue/setNormalisedValue/getScaledValue). Vertical drag, shift = fine, double-click = default.
- Selectors → `Juce.getComboBoxState(id)` (getChoiceIndex/setChoiceIndex, `.properties.choices`).
- Bypass → `Juce.getToggleState("bypass")`.
- Meters → `window.__JUCE__.backend.addEventListener("meters", …)`.
Control ids match APVTS param ids exactly (`clip_drive`, `sat_voicing`, …).

## Visualizers — accurate / true-peak (2026-07-04)
The display is built to tell the truth about the DSP, especially inter-sample/true-peak behaviour. Full spec `docs/2026-07-03-detail-viz.md`; plan `plans/2026-07-04-detail-viz-plan.md`.

**Data path.** Processor captures channel-0 pre/post into lock-free rings (`inRing`/`outRing`, atomic `scopeWrite`); `readScope(n)` copies the last n on the message thread. Editor 30 Hz timer packs 1024 in + 1024 out floats → base64 → `wave` `{n,sr,data}`; JS `Wave` module decodes once, keeps only the latest frame (`Wave.latest()`), no drawing. Audio thread is wait-free (no alloc/lock/serialize).

- **Latency alignment (critical).** A per-channel **pre-tap delay line** delays the captured dry by **L = `lastReportedLatency`** (integer OS group delay + ADAA) before writing `inRing`, so `inRing[i]`/`outRing[i]` are the same instant. An impulse lines up pre/post and stays aligned when the OS factor (hence L) changes. Without this the overlay smears by L.
- **Oscilloscope** — **band-limited reconstruction**, not straight lines. JS Lanczos-3 8× polyphase (`makeLanczos`/`reconstruct`); `drawWave` draws **per-pixel min/max** over the reconstructed signal when dense (never hides an inter-sample peak) and a smooth recon line when sparse. Dry (ghost) + wet share the aligned `t0/span`; trigger (`risingCross`) runs on the aligned dry. **LIN/dB** amplitude toggle (`warp`, floor −60; overshoot maps above unity). Clipped tips (`drawClipHighlight`) shade reconstructed dry beyond ±clip-ceiling.
- **Transfer** — the **actual DSP curve, probed** (not a JS approximation). `DetailForgeProcessor::computeTransferCurve()` runs a ramp (+16-sample warm-up) through a **dedicated offline** sat+clip engine on the message thread; editor emits a `transfer` event only when a curve-shaping param key changes; JS plots the points + a live (inPeak,outPeak) operating dot. Ground-truth: `engine_runner --signal ramp`.
- **Harmonics** — real **FFT** (radix-2, Hann) of the output; 28 log bands (unchanged).
- **Meters** — **true-peak (dBTP)**: dedicated mono 4× oversamplers (`tpUpIn/Out`) + `truePeakDb()` (float, no headroom pad). OUT: green → orange (>limiter ceiling) → red (>0 dBTP). GR = 0 until the limiter (M2). Ref harness `scripts/viz_accuracy.mjs` (BS.1770 4×).

## Real vs. not-yet
- **Affects audio today:** sat_* (M1 saturator), clip_drive/clip_knee/clip_ceiling, oversampling, adaa_order, in_gain, out_gain, bypass.
- **Bound but silent:** lim_* — limiter DSP lands M2 (lim_ceiling still draws a reference line + drives the OUT meter's orange state; GR meter reads 0 until then).
- **Events:** `wave` (scope frames), `transfer` (probed curve, on param change), `meters` (dBTP IN/OUT + GR).

## Gotchas
- **WebView2 static linking (critical).** `NEEDS_WEBVIEW2 TRUE` only *links* the static loader; you MUST also define `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (plus `JUCE_USE_WIN_WEBVIEW2=1`). Without it JUCE tries `LoadLibrary("WebView2Loader.dll")` at runtime, fails, and **falls back to the IE backend** → "Navigation to the webpage was canceled" / blank UI. → [[52-Decision-Log]], [[53-Known-Issues]].
- Serve **`check_native_interop.js`** from the resource provider — `index.js` imports it; missing it 404s the whole module → blank page.
- Member order relays→browser→attachments (crash-on-unload). → [[detail-forge-Operating-Rules]].

## Related
[[21-Clip-Engine]] · [[15-DSP-Signal-Chain]]

Up: [[20-Modules-MOC]]
