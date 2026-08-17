---
type: Phase
phase: 1 — Goal
title: Ideal State
status: draft
---

# Phase 1 — Goal (Ideal State)

## Purpose
Define what you *want* from the lab before constraining it by reality. This is the vision document. It is deliberately unconstrained by budget, hardware, or current skill — those are Phase 2's job.

## When to use
- Greenfield: you have no lab yet and want to plan before buying.
- Existing lab: produce the "Ideal State" to later measure against (Phase 4 Fit).
- Audit: define the target the current lab should grow toward.

## Inputs
- A wishlist (even one line counts).
- Any constraints you already know you care about (privacy, self-hosting ethos, "no cloud").

## Output artifact — Ideal-State doc
Minimal template:

```markdown
# Ideal State — <lab name>

## Intent
- Why this lab exists (1–3 sentences).
- Non-negotiables (e.g. "everything local", "no monthly SaaS").

## Environments (logical)
| Environment | Purpose | Example services |
|-------------|---------|------------------|
| Prod        | ...     | ...              |
| Dev         | ...     | ...              |

## Services (desired)
- Service → what problem it solves → why it's wanted.

## Boundaries / ethics
- What this lab will NOT do.

## Success looks like
- 2–4 statements that mean "this lab is doing its job."
```

## Visualization
Render the environments + services as an aspirational topology (boxes for environments, lists for services). No IPs, no hardware — this is the dream map.

## AI-assist role
The assistant runs [prompts/goal.session.md](../prompts/goal.session.md): it interviews you for intent and services, pushes back on vague "I want it to be cool" answers, and drafts the template. It should refuse to add implementation detail here — that's Phase 3.

## Handoff
→ Phase 2 (Achievable). The Ideal-State doc becomes the baseline the scoped plan is measured against.
