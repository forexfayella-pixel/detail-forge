---
title: Git Workflow
type: runbook
tags: [ops, runbook, meta]
status: living
updated: 2026-07-02
---

# Git Workflow

## Where the git lives
detail-forge is a **folder inside the `brain-workspace` repo** (not its own repo). One history covers workspace + project.

## Guardrails
- Surgical staging: `git add` only what you changed. Never `git add -A` blind.
- No force-push, no `reset --hard`, no branch deletion.
- **No `git push` unless the user asks.**
- Commit the **brain separately** from code: `docs(brain): <what>`. Code commits use `feat: / fix: / chore: / docs: / refactor:`.
- If a commit is blocked, hand over the exact terminal commands (one per line, no inline comments).

## Notes
- Build artifacts (`build/`, `Builds/`, JUCE/CPM caches, `node_modules/`) are gitignored at the workspace root; `brain/` is intentionally tracked.
- The daily brain auto-sync task (if enabled) commits only `brain/` with `docs(brain): daily auto-sync <date>`.

Up: [[40-Operations-MOC]]
