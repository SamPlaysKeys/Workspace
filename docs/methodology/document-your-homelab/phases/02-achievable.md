---
type: Phase
phase: 2 — Achievable
title: Realistic Scope
status: draft
---

# Phase 2 — Achievable (Realistic Scope)

## Purpose
Constrain the Ideal State (Phase 1) by reality: hardware you have/ can buy, budget, time, skill, and risk tolerance. Output is a *scoped plan* plus an explicit **deferred list** — things true to the goal but intentionally not now.

## When to use
- Always after Phase 1 for a greenfield plan.
- For an existing lab: this is your "what can I realistically improve next" pass.

## Inputs
- The Phase 1 Ideal-State doc.
- Honest inventory of hardware, budget, hours/week, comfort level with each tech.

## Output artifact — Scoped plan
Minimal template:

```markdown
# Scoped Plan — <lab name>

## Constraints (the reality)
- Hardware available / budget ceiling / time per week / skill comfort.

## In scope (now)
| Item | From Ideal State? | Why now | Risk |
|------|-------------------|---------|------|

## Deferred (still true to goal, not now)
| Item | Blocking constraint | Revisit when |
|------|---------------------|--------------|

## Acceptance criteria
- What "good enough for v1" means, so Phase 4 has a bar to measure.
```

## Visualization
Same topology as Phase 1, but dim or strike the deferred items. The visual difference between the bright (in-scope) and dim (deferred) map *is* the scope decision made visible.

## AI-assist role
Runs [prompts/achievable.session.md](../prompts/achievable.session.md): challenges "I'll just learn it as I go" for high-risk items, forces an explicit deferred list (most people skip this and silently abandon goals), and drafts acceptance criteria. Pattern reminder: the IaC omitted the "why we deferred" — capture it here so future-you knows it was a choice, not an oversight.

## Handoff
→ Phase 3 (How to). The scoped plan's "In scope" rows become the build/run docs to produce.
