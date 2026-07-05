# detail-forge

**Goal:** A detail-preserving **saturator → clipper → limiter** plugin (Faust + JUCE 8) — ADAA anti-aliasing throughout and a true-peak-aware clip→limit chain that stays clean when pushed.

## Project Brain — READ FIRST
This repo has an Obsidian knowledge vault at **`brain/`** holding all durable project context
(architecture, modules, roadmap, operations, known issues, decisions, and history).

**At the start of every session:** read `brain/00-Index/00-Home.md`. It is the hub that links to
everything — follow its start-of-session checklist and open only the notes relevant to the task
instead of re-reading the whole codebase.

**Keep the brain current.** After any meaningful work:
- Append a dated bullet to `brain/50-Audit-Log/51-Timeline.md`.
- If a decision was made, add it to `brain/50-Audit-Log/52-Decision-Log.md`.
- If a bug/risk was found or fixed, update `brain/50-Audit-Log/53-Known-Issues.md`.
- When code changes a documented fact, update the relevant note and bump its `updated:` date.
- When I say "update the brain," refresh the affected notes.

Conventions are in `brain/00-Index/01-How-To-Use-This-Vault.md`.

## Workspace conventions (summary)
This project lives inside a Brain-Edition workspace. Work the staged pipeline — **Research → Spec → Plan → Implement → Review** — where each stage writes a file the next reads (`docs/`, `specs/`, `plans/`), and each durable fact is written to this project's `brain/` vault. Push heavy exploration into the `explorer` subagent (it reads the brain first); keep implementation in the main agent; never mark work done without verification (a test, build, screenshot, or measurement). Git is surgical: stage only what changed, no force-push/reset, no `git push` unless asked; commit the brain separately with a `docs(brain):` prefix. When reading config/env, record secret **names** only. Full manual: workspace root `CLAUDE.md`.

---

# Stack profile: audio-vst

For audio plugins (VST3/AU/CLAP) and audio apps. Chassis: a Pamplejuce-style JUCE 8 + CMake project. DSP in Faust (via faust2api) where practical. GUI via JUCE 8 WebView (HTML/CSS/JS).

## Conventions
- DSP: write Faust `.dsp` files; generate the C++ engine with faust2api and set parameters by their Faust path via `setParamValue` / MapUI. Faust parameter names are the single source of truth for the parameter list.
- GUI: HTML/CSS/JS in `Source/ui/`, bound with JUCE `WebSliderRelay` / `getSliderState`.
- CRITICAL (prevents crash-on-unload): declare WebView members in this order — relays FIRST, WebBrowserComponent SECOND, parameter attachments LAST. Destruction is the reverse of declaration.
- Validate every build with pluginval before calling it done. Run tests/pluginval on Linux in CI (GitHub bills Windows 2x and macOS 10x minutes on private repos).

## Prerequisites
- git, cmake (3.25+), a C++ toolchain (Xcode CLT on macOS / MSVC on Windows / build-essential on Linux), node (for the WebView UI build), and Faust. JUCE is pulled in by the template (submodule/CPM).

## Build / test / run
- Configure: `cmake --preset default`  •  Build: `cmake --build --preset default`
- Test: build and run the Tests target (Catch2).  •  Validate: run pluginval against the built plugin.
