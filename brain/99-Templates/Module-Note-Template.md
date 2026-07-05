---
title: Module Note Template
type: template
tags: [template, meta]
status: stable
updated: 2026-07-02
---

# Module Note Template

Copy the block below into `20-Modules/2X-<Module>.md` when a subsystem gets real code.

```markdown
---
title: 2X · <Module Name>
type: module
tags: [module, <domain-tag>]
status: stable
updated: YYYY-MM-DD
---

# <Module Name>

## Purpose
<what this subsystem does and why>

## Key files
- `<path>` — <what it does, key exports>

## Data flow
<how data moves through this module>

## Config / params
<Faust param paths / plugin params -> single source of truth>

## Gotchas / open issues
<known bugs, edge cases -> [[53-Known-Issues]]>

## Related
[[...]] · [[...]]

Up: [[20-Modules-MOC]]
```

Up: [[00-Home]]
