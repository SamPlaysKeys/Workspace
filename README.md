# Workspace

A personal workspace for brainstorming, planning, troubleshooting, and building — with AI collaboration conventions baked in.

## What's Here

- **Project planning and documentation** — Architecture designs, decision logs, technical docs
- **Work-in-progress sessions** — Brainstorming, debugging investigations, scratch work
- **Reusable artifacts** — Scripts, playbooks, and configs that can be applied across projects
- **Persistent context** — Session handoffs and state that survives across working sessions

## Structure

```
.
├── docs/                   # Finished documentation and guides
│   └── homelab/            # Homelab infrastructure planning
├── wip/                    # Active brainstorming and investigations
├── planning/               # Session state and tracking
├── artifacts/              # Reusable scripts and configs
├── workstyle/              # AI collaboration conventions
└── roadmap/                # Strategic planning
```

## AI Collaboration

This workspace includes conventions for working with AI assistants — structured behaviors for brainstorming ("Storm Sessions"), debugging ("Troubleshoot"), and session handoffs ("Dead Drops"). These patterns help maintain continuity across sessions and capture knowledge as work happens.

### Loading the Working Style

To start a session with an AI assistant, ask it to load the conventions:

> "Load `workstyle/working_style.md` as your working style, and validate the behaviors and conventions you now have."

The agent should read the document and confirm:
- The behaviors it can invoke (Storm Session, Pre-Mortem, Dead Drop, etc.)
- The conventions it will follow (Progressive Bookkeeping, Isolation, etc.)
- Any Smooth Brain observations about the current context

This ensures the agent is operating with the right patterns before diving into work.

See `workstyle/working_style.md` for the full conventions.

## Current Work

| Project | Location | Status |
|---------|----------|--------|
| Homelab rebuild | `docs/homelab/` | Architecture defined |

Check `planning/whats-next.md` for recent session activity.

---

<details>
<summary>Development Status</summary>

This workspace is under active development. The collaboration patterns are functional and in daily use, but conventions may evolve.

</details>
