---
type: README-Note
status: draft
---

# Document Your Homelab — a reusable methodology

A four-phase, interactive method for documenting infrastructure *with* an AI assistant. It works for three distinct jobs, all using the same loop:

- **Plan a future homelab** — you have no gear yet.
- **Document a current homelab** — you have gear, no paper trail.
- **Audit an existing lab for growth** — produce an "Ideal State" and measure the gap.

## The loop

```
        ┌─────────────────────────────────────┐
        │                                     │
        ▼                                     │
   ┌─────────┐    ┌──────────────┐            │
   │ 1 GOAL  │──▶ │ 2 ACHIEVABLE │            │
   │ Ideal   │    │ Realistic    │            │
   │ State   │    │ scope        │            │
   └─────────┘    └──────────────┘            │
        │                │                    │
        │                ▼                    │
        │         ┌──────────────┐            │
        └────────│ 3 HOW TO     │            │
                  │ Build/run   │            │
                  │ docs        │            │
                  └──────┬───────┘            │
                         ▼                    │
                  ┌──────────────┐            │
                  │ 4 FIT        │────────────┘
                  │ Gap analysis │  (loops back
                  └──────────────┘   to Goal)
```

You can enter the loop at any phase. An existing lab usually enters at **3 (How to)** or **4 (Fit)**; a greenfield plan enters at **1 (Goal)**.

### Why Achievable precedes How-to

It is tempting to jump straight from **Goal** to **How-to** (writing detailed architecture specs or runbooks), but placing **Achievable** second is intentional:

- **Prevents Wasted Design Effort:** Filters aspirational goals against real constraints (budget, physical space, power, skill) *before* spending hours detailing technical implementations for unviable specs.
- **Distinguishes Feasibility Spikes from Operational Docs:** Quick technical sanity checks belong in Achievable (*"Can 8GB RAM run Ceph?"* $\rightarrow$ *"No, defer"*). How-to is reserved for execution and runbooks of the surviving scope.
- **Lowers Friction & Prevents Abandonment:** Homelabs often stall when overwhelmed by oversized designs. Scoping reality first keeps How-to focused, manageable, and gratifying to complete.

## Phase summary

| Phase | Question | Primary artifact | Phase doc | AI session |
|-------|----------|------------------|-----------|------------|
| 1 Goal | What do I *want*? | Ideal-State doc | [phases/01-goal.md](phases/01-goal.md) | [prompts/goal.session.md](prompts/goal.session.md) |
| 2 Achievable | What's realistic? | Scoped plan + deferred list | [phases/02-achievable.md](phases/02-achievable.md) | [prompts/achievable.session.md](prompts/achievable.session.md) |
| 3 How to | How do I build/run it? | Architecture, ADRs, inventory, runbooks | [phases/03-how-to.md](phases/03-how-to.md) | [prompts/how-to.session.md](prompts/how-to.session.md) |
| 4 Fit | How well does it match? | Fit-gap matrix | [phases/04-fit.md](phases/04-fit.md) | [prompts/fit.session.md](prompts/fit.session.md) |

## The "interactive / visualize" mechanism

Each phase produces a structured artifact. The visualization step renders those artifacts so you can *see* the lab and the gaps:

- **Goal & Achievable** render as an environment/intent map (the aspirational topology).
- **How to** renders as an architecture + service map.
- **Fit** renders as a side-by-side matrix: Ideal-State row vs Current/Plan-State row, with a gap flag per item.

The proven precedent for the visual is the interactive HTML overview used in the reference homelab (`../../homelab/overview/homelab-graphic.html`) — the same idea, generalized to any lab, plus the Fit overlay that the reference lab never had.

## How the AI assistant helps (and doesn't)

The assistant's highest-value role is **structured interviewer**: it runs the per-phase session scripts, asks the questions you'd skip alone, and drafts the artifact skeleton. It is *not* a doc ghostwriter — you own the decisions. The session scripts in `prompts/` are written so any agent (Hermes, etc.) can pick them up and drive the loop without re-deriving the method.

## Using this system

1. Pick your entry phase (Goal for greenfield; How to or Fit for existing).
2. Open the matching `prompts/*.session.md` and run it with your assistant.
3. Fill the template from the matching `phases/*.md`.
4. Render the visual for that phase.
5. Loop to Fit, then back to Goal as the lab evolves.

## Reference implementation

The method was extracted from a real, populated lab's docs (`../../homelab/`), which already used ADRs, a roadmap, an environment map, and an interactive overview. This directory generalizes that practice so it applies with or without hardware.

## Homelab doc kit artifact

This methodology is paired with the [homelab-doc-kit](@artifacts/homelab-doc-kit/), a portable, dependency-free scaffold that generates a complete four-phase documentation set from a single command. Use the kit to:

- Quickly scaffold a new documentation directory with `bootstrap.sh`
- Produce self-referential READMEs, phase templates, and AI session prompts
- Generate the `overview/lab-map.template.md` for visualizing your lab topology
- Iterate through the Goal → Achievable → How to → Fit loop without manual file creation

The kit outputs relative-path-linked markdown that works standalone once scaffolded, making it easy to adopt this methodology in any repository.
