---
type: README-Note
status: draft
lab: {{LAB_NAME}}
---

# {{LAB_NAME}} — Homelab Documentation

A four-phase, interactive method for documenting this infrastructure *with* an AI assistant. It works for three distinct jobs, all using the same loop:

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

## How the AI assistant helps (and doesn't)

The assistant's highest-value role is **structured interviewer**: it runs the per-phase session scripts, asks the questions you'd skip alone, and drafts the artifact skeleton. It is *not* a doc ghostwriter — you own the decisions. The session scripts in `prompts/` are written so any agent can pick them up and drive the loop without re-deriving the method.

## Using this system

1. Pick your entry phase (Goal for greenfield; How to or Fit for existing).
2. Open the matching `prompts/*.session.md` and run it with your assistant.
3. Fill the template from the matching `phases/*.md`.
4. Render the visual for that phase (start with [overview/lab-map.template.md](overview/lab-map.template.md)).
5. Loop to Fit, then back to Goal as the lab evolves.

## Worked example (optional, external)

For a fully worked example of this method applied to a real, populated lab, see <https://samplayskeys.com/docs/homelab.html>. Optional and external — this documentation set stands on its own.
