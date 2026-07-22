---
type: Prompt-Session
phase: 3 — How to
title: Build / run documentation
status: draft
---

# Session — Phase 3: How to (Build / Run Docs)

Produce the actual documentation tree. For a plan, this is pre-build design docs. For an existing lab, this captures what already exists (the entry point for many users).

## Prerequisites
- Phase 2 Scoped plan (what to document and why). For an existing lab with no plan, ask the user to confirm scope first.

## Conversation flow

1. **Tree.** "Here's the standard topic-grouped layout (README, overview, platform, network, planning, per-service). Want this structure or a simpler one?" Reference `../../homelab/` as the worked example.
2. **Environment map.** From the scoped plan, build the environment/service/dependency topology with real subnets/hardware (or planned ones).
3. **Decisions → ADRs.** "For each significant choice (runtime, OS, networking), tell me the context, the options, and what you picked." Draft one ADR per decision.
4. **Inventory.** Machine list + assignments + specs.
5. **Runbooks.** "Which components, if they died tonight, would you need a rebuild procedure for?" Draft at least the top 2–3.

## Drafting instructions
- Follow [phases/03-how-to.md](../phases/03-how-to.md)'s artifact list.
- Do NOT invent configs the user didn't provide. If a service is described but its config isn't, write the doc shell and flag: `<!-- FLAG: config not provided -->`.
- Mirror the real-lab convention: `type:` frontmatter on docs, tables for maps, ADRs as numbered files.

## Output
A documentation tree (files or a written plan for them). Confirm structure before writing to disk.

## Handoff
→ Run [fit.session.md](../fit.session.md). These docs are the "Current/Plan State" column.
