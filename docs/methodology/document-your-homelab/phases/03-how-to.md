---
type: Phase
phase: 3 — How to
title: Build / Run Docs
status: draft
---

# Phase 3 — How to (Build / Run Docs)

## Purpose
Produce the actual documentation you'd need to build, rebuild, or onboard someone to the lab: architecture, decision records, inventory, and runbooks. This is the "real" documentation most people mean — but here it's grounded in the Goal and Achievable phases, so it has a reason to exist.

## When to use
- After Phase 2 for a plan you're about to execute.
- Existing lab entering the loop: start here to capture what already exists.

## Inputs
- Phase 1 Ideal State + Phase 2 Scoped plan (what to document and why).
- The actual lab (inventory, configs, running services).

## Output artifacts
A topic-grouped doc set. The reference layout (from a real lab) that works:

```
<lab>/
  README.md            # index + at-a-glance
  overview/            # map + interactive visual
  platform/            # architecture, environments, hardware
  network/             # connectivity, remote access
  planning/            # decisions, ADRs, roadmap, scratchpad
  <service>/           # per-service architecture + provisioning
```

Key doc types:
- **Environment map** — environments, subnets, services, dependencies (topology).
- **ADRs** — Architecture Decision Records: one file per significant decision, with context + choice + consequences.
- **Inventory** — machine list, assignments, specs.
- **Runbooks** — how to rebuild / recover a component.

See `../../homelab/` for a complete worked example of every one of these.

## Visualization
Architecture + service map with real subnets/hardware. This is the "current/plan state" half of the eventual Fit view.

## AI-assist role
Runs [prompts/how-to.session.md](../prompts/how-to.session.md): helps structure the doc tree, drafts ADRs from your described decisions, and extracts an inventory from what you describe. It should not invent configs you didn't provide — flag gaps instead.

## Handoff
→ Phase 4 (Fit). The docs produced here are the "Current/Plan State" column.
