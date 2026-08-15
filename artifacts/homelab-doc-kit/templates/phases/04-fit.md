---
type: Phase
phase: 4 — Fit
title: Gap Analysis
status: draft
lab: {{LAB_NAME}}
---

# Phase 4 — Fit (Gap Analysis)

## Purpose
The most valuable and most-skipped artifact: a side-by-side of **Ideal State** (Phase 1) vs **Current/Plan State** (Phase 3), with a fit flag per item. This turns documentation into a *growth roadmap* and shows how well your plan actually serves your stated goals.

## When to use
- Always, as the close of one loop.
- Existing lab audit: this is the entire point — see where you've drifted from intent.

## Inputs
- Phase 1 Ideal-State doc.
- Phase 3 Current/Plan-State docs (or Phase 2 scoped plan if not yet built).

## Output artifact — Fit-gap matrix
Minimal template:

```markdown
# Fit Analysis — {{LAB_NAME}}

| Ideal-State item | Current/Plan state | Fit | Gap / action |
|------------------|--------------------|-----|--------------|
| Self-hosted photos | Immich running | ✅  | — |
| No monthly SaaS    | Still on Spotify  | ⚠️  | Migrate to Navidrome (deferred) |
| Local-only DNS     | Using Cloudflare  | ❌  | Stand up Pi-hole (Phase 2 deferred) |

## Drift notes
- Where the lab diverged from intent and why.

## Growth roadmap
- Prioritized list of gaps to close, pulled from the ⚠️/❌ rows.
```

## Visualization
The Fit overlay: take the Phase 1/2 topology and the Phase 3 topology, render them stacked, and color each item green/amber/red by fit. This is the visual that makes "areas of growth" obvious at a glance.

## AI-assist role
Runs [../prompts/fit.session.md](../prompts/fit.session.md): diffs the two artifacts, proposes the fit flag per row, and drafts the growth roadmap from the gaps. It should surface *silent drift* — items present in Current but absent from Ideal (scope creep) and items in Ideal but missing from Current (abandoned goals).

## Handoff
→ back to Phase 1. The growth roadmap becomes the next loop's Ideal-State revisions. Documentation is a loop, not a deliverable.
