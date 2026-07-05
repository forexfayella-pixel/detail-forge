---
title: How To Use This Vault
type: guide
tags: [meta, guide]
status: stable
updated: 2026-07-02
---

# How To Use This Vault

## Numbering (MOC system)
- Tens for top-level folders (`10-`, `20-`, …) so categories sort and new ones slot between.
- Within a folder: `N0` = the MOC hub; `N1+` = leaves. Past 9, hex-style suffixes (`2A`, `2B`).

## Frontmatter (every note)
```yaml
---
title: <human title>
type: moc | module | guide | runbook | reference | log | template
tags: [ ... ]
status: living | stable | snapshot | deprecated
updated: YYYY-MM-DD
---
```
- `status: snapshot` = point-in-time; may go stale — verify against the repo.
- `status: living` = updated continuously ([[51-Timeline]], [[31-Current-Priorities]], [[53-Known-Issues]]).

## Linking & tags
- `[[Note-Title]]` wikilinks by basename; keep basenames unique across the vault.
- Every leaf links up to its MOC; every MOC lists its leaves.
- Tags: `#moc #module #architecture #ops #roadmap #audit #reference #meta` + domain tags (`#dsp #faust #juce`).

## Update rules
- Surgical, factual edits — never rewrite whole notes, never add noise.
- After meaningful work: ALWAYS append a dated bullet (newest at top) to [[51-Timeline]]; add decisions to [[52-Decision-Log]]; record bugs/risks in [[53-Known-Issues]]; when code changes a documented fact, update the specific note and bump `updated:`.
- Config/env: record secret **names** only, never values.

Up: [[00-Home]]
