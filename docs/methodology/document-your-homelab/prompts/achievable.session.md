---
type: Prompt-Session
phase: 2 — Achievable
title: Realistic scope discovery
status: draft
---

# Session — Phase 2: Achievable (Realistic Scope)

Run after the Ideal-State doc exists. Goal: a scoped plan + an explicit deferred list. The deferred list is the point — most people skip it and silently abandon goals.

## Prerequisites
- Completed Phase 1 Ideal-State doc.

## Conversation flow

1. **Constraints.** "What's the real ceiling — hardware you have or can buy, budget, hours/week, and your comfort level with each technology you listed?"
2. **In scope.** Walk each Ideal-State item: "Can this be in v1, and what's the risk if it goes wrong?" Build the in-scope table.
3. **Deferred (critical).** For every item NOT in v1: "What's blocking it, and when would you revisit?" Force a row for each. No item leaves the Ideal State unaccounted for.
4. **Acceptance.** "What does 'good enough for v1' mean, specifically?" So Phase 4 has a bar.

## Drafting instructions
- Fill [phases/02-achievable.md](../phases/02-achievable.md)'s Scoped-plan template.
- Every Ideal-State item must appear in either In-scope or Deferred. If one is missing, stop and ask.
- Flag high-risk "I'll learn it as I go" items: `<!-- FLAG: high risk for stated comfort level -->`.
- Pattern reminder to surface (not as a lecture): the reason labs drift is deferred items with no "revisit when" — they die silently. Capture the revisit trigger.

## Output
A completed Scoped plan. Confirm before writing to disk.

## Handoff
→ Run [how-to.session.md](../how-to.session.md) for the in-scope items.
