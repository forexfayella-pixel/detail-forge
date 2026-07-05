---
title: Home — detail-forge Master MOC
type: moc
tags: [moc, meta]
status: living
updated: 2026-07-02
---

# 🏠 detail-forge — Home

Detail-preserving **saturator → clipper → limiter** plugin. Faust DSP + JUCE 8 WebView GUI. The north star: a true-peak-aware clip→limit chain with ADAA anti-aliasing throughout that stays clean when pushed.

## Start-of-session checklist
1. Read **[[31-Current-Priorities]]** — what to work on now.
2. Skim the top of **[[51-Timeline]]** — what happened last.
3. Check **[[53-Known-Issues]]** — open risks/bugs.
4. Open only the notes the task needs (below). Don't re-read the whole codebase.

## Maps of Content
- 🏗️ [[10-Architecture-MOC]] — tech stack, repo structure, the DSP signal chain
- 🧩 [[20-Modules-MOC]] — per-subsystem deep notes (grow as we build)
- 🗺️ [[30-Roadmap-MOC]] — priorities, near-term plan, deferred work
- ⚙️ [[40-Operations-MOC]] — build/validate runbook, git workflow
- 📓 [[50-Audit-Log-MOC]] — timeline, decisions, known issues
- 📚 [[60-Reference-MOC]] — Faust libs, reference repos, DSP concepts, the verification rig

## Index notes
- [[01-How-To-Use-This-Vault]] · [[02-Glossary]] · [[detail-forge-Operating-Rules]] · [[Repo-Docs-Index]]

## The one-paragraph project
A single-band, three-stage series topology: **saturator** (tube/tape/transformer via `aanl.lib`, bias + dry/wet + DC blocker) → **clipper** (ADAA, hard↔soft knee morph) → **lookahead limiter** (`limiter_lad` derivative or lamb-style smoothing, true-peak aware). User-facing oversampling selector + ADAA-order toggle so the CPU/fidelity trade is the user's. See [[15-DSP-Signal-Chain]].
