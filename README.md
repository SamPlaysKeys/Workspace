# Workspace

A personal workspace for brainstorming, planning, troubleshooting, and building — with AI collaboration conventions baked in.

This repository serves as a centralized hub for my technical projects, documentation, and experimentation. It uses a structured approach to collaboration with AI agents to maintain continuity and capture knowledge.

## Upstream First Philosophy

This workspace operates on an **"upstream first"** paradigm: rather than building custom, isolated solutions directly within client or downstream environments, we author, refine, and publish general-purpose examples, reusable artifacts, modules, and documentation open-source first. These upstream-validated assets are then pulled down into target environments as clean, standard deliverables, reducing technical debt and accelerating delivery across engagements.

## Acknowledgments

Special thanks to [hhellbusch/my-ai-workspace](https://github.com/hhellbusch/my-ai-workspace) for providing much of the inspiration and foundational patterns for the AI collaboration workflows and workspace structure used here.

## What's Here

- **Project planning and documentation** — Architecture designs, decision logs, technical docs (OpenShift, Homelab, OpenBao, etc.)
- **Work-in-progress sessions** — Brainstorming, debugging investigations, and scratch work in `wip/`
- **Reusable artifacts** — Scripts, playbooks, and configs that can be applied across projects
- **Persistent context** — Session handoffs and state that survives across working sessions

## Structure

```
.
├── .agents/                # Installed AI agent skills and behaviors
├── artifacts/              # Reusable scripts, playbooks, and configs
├── docs/                   # Finished documentation and guides
│   ├── homelab/            # Homelab infrastructure planning
│   └── troubleshooting/    # Solutions for common technical issues
├── planning/               # Session state, tracking, and roadmaps
├── research/               # Deep dives and comparative analysis
├── wip/                    # Active brainstorming and investigations
└── workstyle/              # AI collaboration conventions (Working Style)
```

## AI Collaboration

This workspace enforces strict alignment and controlled execution through dedicated **Skills** and **Conventions**. These patterns help maintain continuity across sessions and capture knowledge as work happens.

### Core Behaviors

| Behavior | Purpose |
|----------|---------|
| **Start** | Orient the session based on state files. |
| **Ideate** | Quarantined rapid ideation in `wip/`. |
| **Consolidate**| Tame a messy `wip/` folder with a BRIEF. |
| **Troubleshoot**| Phased investigation and fix for technical issues. |
| **Document** | Graduate findings into standardized docs/guides. |
| **Handoff** | Capture session state for the next session. |

### Loading the Working Style

To start a session with an AI assistant, ask it to load the conventions:

> "Load `workstyle/working_style.md` as your working style, and validate the behaviors and conventions you now have."

See `workstyle/working_style.md` and `AGENTS.md` for the full conventions.

---

> [!WARNING]
> This workspace is under active development. The collaboration patterns are functional and in daily use, but conventions may evolve.

