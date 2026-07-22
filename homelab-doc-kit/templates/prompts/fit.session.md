---
type: Prompt-Session
phase: 4 — Fit
title: Gap analysis
status: draft
lab: {{LAB_NAME}}
---

# Session — Phase 4: Fit (Gap Analysis) for {{LAB_NAME}}

The close of the loop and the most valuable output. Diff Ideal State against Current/Plan State and produce the fit-gap matrix + growth roadmap.

## Prerequisites
- Phase 1 Ideal-State doc.
- Phase 3 Current/Plan-State docs (or Phase 2 scoped plan if not yet built).

## Conversation flow

1. **Diff.** Walk each Ideal-State item. "Is it in the current/plan state? Fully, partially, or not?" Assign fit: ✅ / ⚠️ / ❌.
2. **Silent drift (both directions).** 
   - *Abandoned goals:* Ideal items missing from Current → why?
   - *Scope creep:* Current items absent from Ideal → intentional or accidental?
3. **Gap actions.** For each ⚠️/❌: "What closes this, and what's the priority?"
4. **Roadmap.** "Order the gaps by impact vs effort. That's your next-loop backlog."

## Drafting instructions
- Fill [../phases/04-fit.md](../phases/04-fit.md)'s Fit-gap matrix template.
- Every Ideal-State row must have a fit flag. No "we'll see."
- Capture drift notes explicitly — this is the artifact most labs never write.
- Growth roadmap = the ⚠️/❌ rows, prioritized.

## Output
A completed Fit analysis + growth roadmap. Confirm before writing to disk.

## Handoff
→ Loop back to Phase 1. The growth roadmap revises the Ideal State for the next pass. Documentation is a loop.
