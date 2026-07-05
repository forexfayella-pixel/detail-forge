---
title: detail-forge Operating Rules
type: guide
tags: [meta, guide, ops]
status: stable
updated: 2026-07-02
---

# detail-forge — Operating Rules

Mirror of the guardrails in the project `CLAUDE.md` and the workspace manual. Read before making changes.

## Pipeline
Research → Spec → Plan → Implement → Review. Each stage writes a file the next reads (`docs/` → `specs/` → `plans/`). Durable facts land in this vault. Pause when requirements are unclear — research + a recommendation is cheaper than wrong edits.

## DSP / audio-vst rules
- DSP in Faust `.dsp`; generate C++ with **faust2api**; set params by Faust path via `setParamValue`/MapUI. **Faust parameter names are the single source of truth** for the parameter list.
- GUI: HTML/CSS/JS in `Source/ui/`, bound with JUCE `WebSliderRelay` / `getSliderState`.
- **CRITICAL (crash-on-unload):** declare WebView members in order — relays FIRST, WebBrowserComponent SECOND, parameter attachments LAST. Destruction is the reverse of declaration. See [[53-Known-Issues]].
- **lamb needs double precision** (Faust 2.72.14+). We have 2.85.9 ✓.
- Validate every build with **pluginval** before calling it done.

## Verification
"Detail-preserving" is unfalsifiable without measurement. The verification rig ([[62-Testing]]) — null test, aliasing spectrogram, THD+N vs frequency, matched-loudness A/B — is a first-milestone deliverable. Every fidelity claim is checked against it.

## Git
Surgical: stage only what changed; no force-push/reset/branch-delete; no `git push` unless asked. Commit the brain separately with `docs(brain):`. This project is a folder inside the `brain-workspace` repo. See [[47-Git-Workflow]].

Up: [[00-Home]]
