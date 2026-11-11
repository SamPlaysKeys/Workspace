---
type: ADR
Status: Accepted
Date: 2026-11-11
---

# ADR 0002: Review with a context-free congress of six lenses

## Context

A single reviewer, human or model, anchors on whatever it was told and on its own habits. A reviewer who has just been in a conversation about the code also carries that conversation's beliefs ("this was fixed", "this is unused"). The skill exists to avoid exactly that.

Different failure modes need different questions. Logic bugs, failure paths, security boundaries, hidden dependencies and maintainability are only partly covered by any one pass.

## Decision

Phase 1 always uses **fresh subagents with no conversation context**, run in parallel. Each gets the same neutral brief plus one lens charter:

| Delegate | Focus |
|---|---|
| **Smooth Brain** | Plain-language blind read: what does the code do, step by step? No judgment, no intent, no fixes. |
| **Skeptic** | Logic and correctness, edge cases, wrong conditions. |
| **Saboteur** | Failure paths: partial failure, retries, idempotency, "runs twice, never, or half-way". |
| **Gatekeeper** | Security and trust boundaries. |
| **Operator** | Hidden dependencies: environment, ordering, shared state, config. |
| **Maintainer** | Newcomer's view: readability, naming, omissions such as missing tests or docs. |

- `full` mode (default) runs all six. `quick` runs Smooth Brain, Skeptic and Saboteur.
- **Smooth Brain is mandatory in every mode.** It is the blind read that makes reconciliation possible ([ADR 0003](0003-withhold-intent-then-reconcile.md)).
- Every finding follows one schema: `severity | file:line | claim | evidence | confidence (verified/inferred/assumed) | could-not-verify`. Delegates must report unverified items and treat apparently deliberate behavior as **undocumented behavior**, not as correct.
- Delegates use a read-only subagent type and never edit, create or delete files.

## Alternatives Considered

| Option | Why not |
|---|---|
| One reviewer with several checklists | Same anchoring and the same blind spots across every checklist. |
| Review in the main conversation | Inherits the conversation's beliefs, which is what the skill is meant to set aside. |
| Many more lenses (performance, accessibility, etc.) | Cost grows linearly. Six cover the failure families seen in practice, and further lenses can be added later. |

## Consequences

- Convergence across lenses is a usable confidence signal, and disagreement is visible.
- A full run costs 6 delegate runs before any verification ([ADR 0004](0004-adjudication-wave-and-severity.md)).
- Delegates start without context, so some say "could not verify X". This was handled by letting them list the repo layout ([ADR 0003](0003-withhold-intent-then-reconcile.md)).
