# Working Style

This directory defines how AI agents should collaborate in this workspace. The conventions are agent and model agnostic — any AI assistant working here should follow them.

## Core File

**`working_style.md`** — The complete behavior and convention definitions. AI agents should read this file to understand how to operate in this workspace.

## Behaviors

| Behavior | Trigger | Purpose |
|----------|---------|---------|
| **Storm Session** | "Storm session for X" | Brainstorm new ideas in `wip/` |
| **Pre-Mortem** | "Pre-mortem" or agent-initiated | Challenge decisions before committing |
| **Smooth Brain** | Session start or "smooth brain" | Surface and question assumptions |
| **Progressive Bookkeeping** | Always-on | Keep state files current as work happens |
| **Dead Drop** | "Dead drop" / "bread crumb" | Structured session handoff |
| **Troubleshoot** | "Troubleshoot" / "debug" | Structured debugging with knowledge capture |
| **Sneaky** | "Be sneaky" / "hide your actions" | Hide working dirs under `.workspace/` |

## Conventions

- **Isolation** — No writes to external workspaces without confirmation
- **Append, Don't Replace** — State files accumulate history
- **Documentation Structure** — Graduated docs live in `docs/<project>/`

## Directory Contents

```
workstyle/
├── working_style.md    # Full behavior definitions (the source of truth)
├── rules/              # Cursor-specific rule files
└── skills/             # Custom skill definitions
```

## Quick Reference

### Starting a session
AI agents should do a quick "Smooth Brain" check — what assumptions are being carried forward? What might have changed?

### During work
Keep `planning/whats-next.md` and relevant `wip/` files current (Progressive Bookkeeping).

### Ending a session
Prompt for a Dead Drop if work is in progress. Write structured handoff to `planning/whats-next.md`.

### In external repos
Use Sneaky mode to hide working files under `.workspace/`.
