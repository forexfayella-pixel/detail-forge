# Plan: Milestone 0 — Vertical Slice

**Spec:** specs/2026-07-02-m0-vertical-slice.md
**Date:** 2026-07-02

## Grounding (verified 2026-07-02)
- Faust arch headers: `C:\Program Files\Faust\include\faust\` (include dir `C:/Program Files/Faust/include`).
- `faust -lang cpp -cn ClipEngine clip.dsp -o ClipEngine.h` → `class ClipEngine : public dsp` with `getNumInputs/Outputs`, `init(sr)`, `buildUserInterface(UI*)`, `compute(n,in,out)`. Params via `MapUI` path `/…/Drive`.
- Clip: `aa.hardclip2` (ADAA2) from `aanl.lib`; drive via `ba.db2linear`.
- Chassis: CMake + JUCE 8 + Catch2 via CPM.cmake.

## Tasks
1. [ ] **CMake + JUCE skeleton.** files: `CMakeLists.txt`, `CMakePresets.json`, `cmake/CPM.cmake`, `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.{h,cpp}` — a minimal JUCE 8 `AudioProcessor` (VST3 + Standalone), generic editor for now. — verify: `cmake --preset default` configures (JUCE fetched); `cmake --build --preset default` builds; Standalone binary exists.
2. [ ] **Faust clip engine.** files: `Source/dsp/clip.dsp`, generated `Source/dsp/ClipEngine.h` (CMake custom command regenerates via `faust`), `PluginProcessor.cpp` — instantiate `ClipEngine` + `MapUI`, `init(sampleRate)` in `prepareToPlay`, `compute` per channel in `processBlock`. — verify: unit test feeds a >0 dBFS sine at high drive and asserts output is bounded to ≈±1.0 (clipping happens).
3. [ ] **Drive control end-to-end (WebView).** files: `Source/ui/index.html` (+ inline css/js), `Source/PluginEditor.{h,cpp}` (WebBrowserComponent + `WebSliderRelay`; member order relays→browser→attachments), `CMakeLists.txt` (embed UI as binary data) — a Drive slider (0–36 dB) bound via `getSliderState`/relay to Faust `/clip/Drive`. — verify: moving the slider changes the MapUI param value (logged/asserted); Standalone shows the panel.
4. [ ] **Latency reporting.** files: `PluginProcessor.cpp` — measure `hardclip2` group delay (offline impulse/null alignment), `setLatencySamples(N)` in `prepareToPlay`. — verify: measured chain delay == reported latency (test asserts the null-aligned residual is minimized at N).
5. [ ] **Measurement harness.** files: `Tests/CMakeLists.txt`, `Tests/measurements.cpp` (Catch2), `scripts/analyze.py` (offline spectra) — null test, swept-sine worst-case alias level, THD+N vs freq, with thresholds from the spec. — verify: `ctest` prints alias ≥60 dB below fundamental (ADAA2) and beats a naïve hardclip control; null ≤ −120 dB.
6. [ ] **pluginval.** files: `scripts/validate.ps1` (download + run pluginval on the built VST3) — verify: pluginval strictness 5 exits 0.

## Risks / uncertain tasks
- **Task 1 (JUCE fetch)** — first CPM configure downloads JUCE 8 (large/slow); possible MSVC generator quirks. Highest-friction step; do first.
- **Task 3 (WebView)** — JUCE 8 WebView relay wiring is fiddly; the crash-on-unload member order is the classic trap.
- **Task 4** — pure ADAA2 delay is expected ~0–1 sample; confirm empirically, don't assume 0.
- **Task 5** — thresholds may need tuning to the real C++ build (research noted MATLAB-clean values misbehaving in C++).

## Definition of done
Plugin loads; `pluginval` green; reported latency == measured; harness prints alias + THD+N below spec thresholds; brain updated (Timeline + module notes for the clip engine); reviewer reports no blocking issues.
